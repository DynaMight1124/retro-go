/*
    VeMUlator - A Dreamcast Visual Memory Unit emulator for libretro
    Copyright (C) 2018  Mahmoud Jaoune

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
#include <streams/file_stream.h>
#include <string.h>
#include <time/rtime.h>

#include "vmu.h"

/* Forward declarations */
extern "C" {
RFILE* rfopen(const char *path, const char *mode);
int64_t rfseek(RFILE* stream, int64_t offset, int origin);
int64_t rftell(RFILE* stream);
int rfgetc(RFILE* stream);
int rfclose(RFILE* stream);
}

VMU::VMU(uint16_t *_frameBuffer)
{
   //Initialize system
   ram          = new VE_VMS_RAM();
   rom          = new VE_VMS_ROM();
   flash        = new VE_VMS_FLASH(ram);
   intHandler   = new VE_VMS_INTERRUPTS();

   cpu          = new VE_VMS_CPU(ram, rom, flash, intHandler, true);
   audio        = new VE_VMS_AUDIO(cpu, ram);
   t0           = new VE_VMS_TIMER0(ram, intHandler, cpu, &prescaler);
   t1           = new VE_VMS_TIMER1(ram, intHandler, audio);
   baseTimer    = new VE_VMS_BASETIMER(ram, intHandler, cpu);
   video        = new VE_VMS_VIDEO(ram);
   serial       = new VE_VMS_SERIAL(ram, intHandler);
   frameBuffer  = _frameBuffer;


   //Initialize variables
   ccount       = 0;  //Cycle count
   cycle_count  = 0;
   time_reg     = 0;
   frame_skip   = 0;
   CPS          = 0; //Real cycles per second
   prescaler    = 0;
   pcount       = 0;
   oldPRR       = -1;

   OSC          = 0;
   OCR_old      = -1; //For performance, not to calculate clock each time, unless OCR is changed.
   threadReady  = false;
   inSleepState = false;
   BIOSExists   = false;
   linkConnectorBits = 0;
   fixedClock   = false;
   enableSound  = true;
   useT1ELD     = false; //Some mini-game programmers (Especially homebrew creators) don't use it
   cycles_left  = 0;
}

VMU::~VMU()
{
   delete t0;
   delete t1;
   delete baseTimer;
   delete audio;
   delete serial;
   delete video;
   delete flash;
   delete cpu;
   delete intHandler;
   delete ram;
   delete rom;
}

int VMU::loadBIOS(const char *filePath)
{
   size_t i, fileSize, readSize;
   int result = 0;
   byte *BIOS_Data_Encrypted;
   byte *BIOS_Data;
   RFILE *bios = rfopen(filePath, "rb");

   if(!bios)
      return -1;

   rfseek(bios, 0, SEEK_END);
   fileSize = (size_t)rftell(bios);
   rfseek(bios, 0, SEEK_SET);

   /* A BIOS dump is either the plain 60KB of ROM, a full 64KB chip dump
      (the tail past 0xF000 is unmapped padding) or an encrypted image
      carrying a 4 byte header. Anything shorter than the ROM itself
      cannot boot. */
   if(fileSize != 0xF000 && fileSize != 0xF004 && fileSize != 0x10000)
   {
      rfclose(bios);
      return -2; //Unknown BIOS image type
   }

   BIOS_Data_Encrypted = new byte[0xF004];
   BIOS_Data           = new byte[0xF000];

   readSize = fileSize > 0xF004 ? 0xF004 : fileSize;

   for(i = 0; i < readSize; ++i)
      {
      int value = rfgetc(bios);
      if (value == EOF) { rfclose(bios); delete []BIOS_Data;
         delete []BIOS_Data_Encrypted; return -2; }
      BIOS_Data_Encrypted[i] = (byte)value;
   }

   rfclose(bios);

   if (readSize < 0xF004 && BIOS_Data_Encrypted[0] != 0x2A) {
      delete []BIOS_Data; delete []BIOS_Data_Encrypted;
      return -2;
   }
   //Decrypt BIOS file if encrypted (First opcode is not JMPF)
   if(BIOS_Data_Encrypted[0] != 0x2A)
   {
      //Remove first 4 bytes
      for (i = 0; i < 0xF000; ++i)
         BIOS_Data[i] = BIOS_Data_Encrypted[i + 4];

      //XOR 0x37
      for (i = 0; i < 0xF000; ++i)
         BIOS_Data[i] = (byte) ((BIOS_Data[i] ^ 0x37) & 0xFF);
   }
   else
   {
      //BIOS is not encrypted
      for(i = 0; i < 0xF000; ++i)
         BIOS_Data[i] = BIOS_Data_Encrypted[i];
   }

   //Check BIOS one last time (After decrypting)
   if(BIOS_Data[0] != 0x2A)
      result = -2;
   else
   {
      BIOSExists = true;

      //Real BIOS code runs instead of the emulated system calls
      cpu->setHLE(false);

      //This is loaded in (60KB) of ROM
      for(i = 0; i < 0xF000; ++i)
         rom->writeByte(i, BIOS_Data[i]);
   }

   delete []BIOS_Data;
   delete []BIOS_Data_Encrypted;

   return result;
}

bool VMU::hasBIOS()
{
   return BIOSExists;
}

void VMU::halt()
{
	cpu->state = 0;
}

void VMU::setDate()
{
	/* Set time and date. Through rtime_localtime rather than localtime, which
	   hands back a pointer to one static struct shared by the whole process
	   and can return NULL: this runs on the emulation thread while the
	   frontend is free to be formatting timestamps of its own. */
	time_t    rawTime;
	struct tm timeBuf;
	struct tm *currentTime;

	if (fixedClock)
	{
		/* Saturday 1 January 2000, midnight. Built by hand rather than through
		   localtime, which would put two peers in different time zones on
		   different dates again. */
		memset(&timeBuf, 0, sizeof(timeBuf));
		timeBuf.tm_year = 100;
		timeBuf.tm_mday = 1;
		timeBuf.tm_wday = 6;
		currentTime     = &timeBuf;
	}
	else
	{
		time(&rawTime);
		currentTime = rtime_localtime(&rawTime, &timeBuf);
	}

	if(!currentTime)
		return;

	/* The BIOS keeps the clock in two forms at once, and its own half-second
	   handler increments the binary set. Seeding both is all this has to do;
	   the BIOS counts from here.
	 *
	 *   0x10-0x16  century, year-in-century, month, day, hour, min, sec, BCD
	 *              (the copies the set-clock UI and Maple show)
	 *   0x17-0x18  full year, binary, high byte first
	 *   0x19-0x1D  month, day, hour, min, sec, binary
	 *   0x1E       half-second toggle, 0x1F leap year
	 */
	int  fullYear = currentTime->tm_year + 1900;
	byte day      = currentTime->tm_mday & 0xFF;
	byte month    = (currentTime->tm_mon + 1) & 0xFF;   //tm_mon counts from 0
	byte century  = (byte)(fullYear / 100);
	byte yearIn   = (byte)(fullYear % 100);
	byte hour     = currentTime->tm_hour & 0xFF;
	byte min      = currentTime->tm_min & 0xFF;
	byte sec      = currentTime->tm_sec & 0xFF;
	bool leap     = (fullYear % 4 == 0)
	             && (fullYear % 100 != 0 || fullYear % 400 == 0);

	//BCD copies
	ram->writeByte_RAW(0x10, int2BCD(century));
	ram->writeByte_RAW(0x11, int2BCD(yearIn));
	ram->writeByte_RAW(0x12, int2BCD(month));
	ram->writeByte_RAW(0x13, int2BCD(day));
	ram->writeByte_RAW(0x14, int2BCD(hour));
	ram->writeByte_RAW(0x15, int2BCD(min));
	ram->writeByte_RAW(0x16, int2BCD(sec));

	//Binary copies, which are the ones the BIOS counts
	ram->writeByte_RAW(0x17, (byte)((fullYear >> 8) & 0xFF));
	ram->writeByte_RAW(0x18, (byte)(fullYear & 0xFF));
	ram->writeByte_RAW(0x19, month);
	ram->writeByte_RAW(0x1A, day);
	ram->writeByte_RAW(0x1B, hour);
	ram->writeByte_RAW(0x1C, min);
	ram->writeByte_RAW(0x1D, sec);
	ram->writeByte_RAW(0x1E, 0);
	ram->writeByte_RAW(0x1F, leap ? 1 : 0);
}

//Sets system variables in RAM
void VMU::initBIOS()
{
	setDate();

	ram->writeByte_RAW(0x31, 0xFF);

	//No buttons clicked
	ram->writeByte_RAW(P3, 0xFF);
}

void VMU::startCPU()
{
	if(!BIOSExists)
	{
		//Enable HLE
		ram->writeByte_RAW(EXT, 1);
		cpu->EXTOld = 1;
		initializeHLE();
	} else initBIOS();

	cpu->state = 1;
}

void VMU::initializeHLE()
{
	//Initialize system variables
	setDate();
	ram->writeByte_RAW(0x31, 0xFF);
	ram->writeByte_RAW(0x6E, 0xFF);

	//Initialize SFR
	ram->writeByte_RAW(P3, 0xFF);
	ram->writeByte_RAW(SP, 0x7F);
	ram->writeByte_RAW(PSW, 0x02);
	ram->writeByte_RAW(IE, 0x80);
	ram->writeByte_RAW(MCR, 0x08);
	ram->writeByte_RAW(P7, 0x02);
	ram->writeByte_RAW(OCR, 0xA3);
	ram->writeByte_RAW(BTCR, 0x41);

	/* Port 1 as the BIOS leaves it: P17 an output carrying timer 1's pulse,
	   which is the buzzer. Games never set it themselves, and timer 1 only
	   reaches the speaker through it, so without these the HLE boot is mute.
	   Measured from the real BIOS as a game starts the timer. */
	ram->writeByte_RAW(P1FCR, 0xBF);
	ram->writeByte_RAW(P1DDR, 0x80);
}

void VMU::runCycle() 
{
	//Calculate cpu clock frequency (Only when OCR is changed)
	byte OCR_data = ram->readByte_RAW(OCR);
	if (OCR_data != OCR_old) 
	{
		int freqDiv = 12;
		if ((OCR_data & 128) != 0) freqDiv = 6;
		OSC = 0;    //Main clock by default is RC
		if ((OCR_data & 32) != 0) OSC = 1; //Quartz
		//double freq;
		if (OSC == 0) 
		{
			audio->setAudioFrequency(600000.0 / freqDiv);   //What the real frequency should be
			cpu->setFrequency(600000.0 / freqDiv);
		} 
		else    //RC
		{
			audio->setAudioFrequency(32768.0 / freqDiv);   //What the real frequency should be
			cpu->setFrequency(32768.0 / freqDiv);
		}
	}
	OCR_old = OCR_data;

	/* 0x31 is the BIOS's "the clock has been set" flag. Holding it means the
	   BIOS takes the host time seeded in setDate() instead of stopping at its
	   own set-the-clock screen.
	 *
	 * P7 is the external connector: bit0 high means plugged into a controller,
	 * which sends the BIOS into Dreamcast mode, and bit1 low means the battery
	 * is flat, which gets a "change battery" screen. Standalone with a good
	 * battery is bit0 clear, bit1 set. */
	ram->writeByte_RAW(0x31, 0xFF);

	/* Bits 2 and 3 are the states of connector pins 13 and 6, which is how a
	   VMU notices another one has been clipped onto it. Software looks here
	   before it touches the serial port at all. */
	ram->writeByte_RAW(P7, (byte)(0x02 | linkConnectorBits));



	//Execute
	if (cycles_left <= 0 && cpu->state != 0)
	{
		cpu->processInterrupts();
		if (ram->readByte_RAW(PCON) == 0)
			cycles_left = cpu->processInstruction(false);
	}
	if (cycles_left > 0) --cycles_left;

	//Set prescaler
	byte PRR = ram->readByte_RAW(T0PRR);

	//This is important since Base Timer interrupts sometimes manipulates PRR, so we reset it so the modulo would be 0 in case number came.
	if (PRR != oldPRR) 
		pcount = PRR;
		
	/* An 8 bit up-counter reloaded from T0PRR when it overflows, so it hands
	   timer 0 one tick every (256 - T0PRR) cycles. Counting first and testing
	   after is what makes that exact: testing first spends an extra cycle in
	   the period, which is 0.4% at T0PRR=0 and a factor of two at 0xFF. */
	pcount++;

	if (pcount > 255)
	{
		prescaler = 1;
		pcount = PRR;
	}
	else
		prescaler = 0;

	oldPRR = PRR;


	//Run timers (t0 and t1)
	serial->runCycle();

	t0->runTimer();
	t1->runTimer();
	baseTimer->runTimer();

	/* Hand the BIOS the host clock once, after it has finished zeroing its own
	   system variables, and then leave it alone: from here the BIOS counts the
	   half-second base timer interrupts itself. The clearing is measured to be
	   done by cycle 20000, so this leaves a wide margin and still lands within
	   a few seconds of emulated time. */
	if (ccount <= 60000)
	{
		if (ccount == 60000 && BIOSExists)
			setDate();

		ccount++;
	}

	cycle_count++;
}

void VMU::reset()
{
   delete t0;
   delete t1;
   delete baseTimer;
   delete audio;
   delete serial;
   delete video;
   delete flash;
   delete cpu;
   delete intHandler;
   delete ram;
   delete rom;

   //Re-initialize system
   ram          = new VE_VMS_RAM();
   rom          = new VE_VMS_ROM();
   flash        = new VE_VMS_FLASH(ram);
   intHandler   = new VE_VMS_INTERRUPTS();

   cpu          = new VE_VMS_CPU(ram, rom, flash, intHandler, true);

   audio        = new VE_VMS_AUDIO(cpu, ram);

   t0           = new VE_VMS_TIMER0(ram, intHandler, cpu, &prescaler);
   t1           = new VE_VMS_TIMER1(ram, intHandler, audio);
   baseTimer    = new VE_VMS_BASETIMER(ram, intHandler, cpu);

   video        = new VE_VMS_VIDEO(ram);
   serial       = new VE_VMS_SERIAL(ram, intHandler);

   //Re-nitialize variables
   ccount       = 0;  //Cycle count
   cycle_count  = 0;
   time_reg     = 0;
   frame_skip   = 0;
   CPS          = 0; //Real cycles per second
   prescaler    = 0;
   pcount       = 0;
   oldPRR       = -1;

   OSC          = 0;
   OCR_old      = -1; //For performance, not to calculate clock each time, unless OCR is changed.
   threadReady  = false;
   inSleepState = false;
   BIOSExists   = false;
   linkConnectorBits = 0;
   enableSound  = true;
   useT1ELD     = false; //Some mini-game programmers (Especially homebrew creators) don't use it
   cycles_left  = 0;
}

/* A stamp so a state from a different build of the core is refused rather
   than read as though it matched. */
#define VMU_STATE_MAGIC   0x564D5531u   /* "VMU1" */
#define VMU_STATE_VERSION 2u

void VMU::serialize(VE_STATE &s)
{
   unsigned magic   = VMU_STATE_MAGIC;
   unsigned version = VMU_STATE_VERSION;

   s.u32(magic);
   s.u32(version);

   /* Not ours, or not this version of ours. Say so rather than reading the
      rest of it into the machine. */
   if(magic != VMU_STATE_MAGIC || version != VMU_STATE_VERSION)
   {
      s.fail();
      return;
   }

   ram->serialize(s);
   flash->serialize(s);
   cpu->serialize(s);
   t0->serialize(s);
   t1->serialize(s);
   baseTimer->serialize(s);
   intHandler->serialize(s);
   audio->serialize(s);
   serial->serialize(s);

   /* What the machine itself carries: the cycle the CPU is part way through,
      the prescaler, the clock the oscillator control register settled on, and
      the connector. The frame buffer is left out -- it is redrawn from XRAM
      every frame -- and so is ROM, which the BIOS fills at load and nothing
      writes afterwards. */
   s.u8(linkConnectorBits);
   s.i32(ccount);
   s.i64(cycle_count);
   s.i64(time_reg);
   s.i64(frame_skip);
   s.f64(CPS);
   s.u8(prescaler);
   s.i32(pcount);
   s.i32(oldPRR);
   s.i32(OSC);
   s.i32(OCR_old);
   s.b(threadReady);
   s.b(inSleepState);
   s.b(BIOSExists);
   s.b(enableSound);
   s.b(useT1ELD);
   s.i32(cycles_left);
}

size_t VMU::serializeSize()
{
   VE_STATE s(VE_STATE::COUNT, NULL, 0);
   serialize(s);
   return s.used();
}

bool VMU::saveState(void *buffer, size_t size)
{
   VE_STATE s(VE_STATE::SAVE, (byte*)buffer, size);
   serialize(s);
   return s.ok();
}

bool VMU::loadState(const void *buffer, size_t size)
{
   if (!buffer || size != serializeSize()) return false;
   const byte *header = (const byte *)buffer;
   if (header[0] != 0x31 || header[1] != 0x55 || header[2] != 0x4D ||
       header[3] != 0x56 || header[4] != VMU_STATE_VERSION ||
       header[5] || header[6] || header[7]) return false;
   // Keep rejection transactional even when a subsystem rejects a field after
   // earlier subsystems have already consumed their bytes.
   byte *rollback = new byte[size];
   if (!saveState(rollback, size)) { delete []rollback; return false; }
   VE_STATE s(VE_STATE::LOAD, (byte*)buffer, size);
   serialize(s);
   bool ok = s.ok();
   if (!ok) {
      VE_STATE restore(VE_STATE::LOAD, rollback, size);
      serialize(restore);
   }
   delete []rollback;
   return ok;
}
