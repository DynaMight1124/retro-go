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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <streams/file_stream.h>
#include <time/rtime.h>

#include <libretro.h>
#include "libretro_link.h"
#include "vmu.h"

#ifdef _WIN32
#define RETRO_PATH_SEPARATOR '\\'
#else
#define RETRO_PATH_SEPARATOR '/'
#endif

/* Forward declarations */
extern "C" {
   RFILE* rfopen(const char *path, const char *mode);
   int64_t rfseek(RFILE* stream, int64_t offset, int origin);
   int64_t rftell(RFILE* stream);
   int rfgetc(RFILE* stream);
   int rfclose(RFILE* stream);
}

retro_environment_t environment_cb;
retro_video_refresh_t video_cb;
retro_audio_sample_t audio_cb;
retro_audio_sample_batch_t audio_batch_cb;
retro_input_poll_t input_poll_cb;
retro_input_state_t input_state_cb;
static void fallback_log(enum retro_log_level level, const char *fmt, ...) { }

static retro_log_printf_t log_cb = fallback_log;

struct retro_variable options[6] = {
   {"enable_flash_write", "Enable flash write (.bin, requires restart); enabled|disabled"},
   {"bios", "BIOS (requires restart); auto|american|japanese|disabled"},
   {"serial_link", "Serial link to another VMU; enabled|disabled"},
   {"icon_row", "Icon row (requires restart); enabled|disabled"},
   {"clock", "Clock at power-on (requires restart); system|fixed"},
   { NULL, NULL }
};

/* Drawing the icon strip makes the frame taller than 48x32, so this is read
   once, when the geometry is settled, and not changed under the frontend. */
static bool iconRow = true;

static unsigned screenHeight(void)
{
   return iconRow ? SCREEN_HEIGHT_ICONS : SCREEN_HEIGHT;
}

static VMU *vmu;
static uint16_t *frameBuffer;
static byte *romData;
static size_t romSize;
static int romType;
static bool flashWrite;
static char romPath[4096];
static char biosPath[4096];

/* The cable to another VMU, hosted by the frontend. A frontend that does not
   offer one leaves link_port NULL and the core runs exactly as it always has:
   a VMU with nothing in its socket. */
static struct retro_link_interface link_iface;
static retro_link_port_t          *link_port;
static uint64_t                    link_tick;
static uint64_t                    link_safe;

/* What this core calls a tick. The VMU's RC oscillator divided by 12, which is
   what OCR selects out of reset and the rate retro_run counts cycles at. A
   guest that reprograms OCR changes the real rate and this declared one does
   not follow it -- harmless while both ends of a cable are VMUs booted the
   same way, since the frontend only uses it to convert between peers' units. */
#define LINK_CLOCK_RATE 50000

/* How far ahead of itself this core promises not to originate a transfer.
   Traded against fidelity: larger means fewer rendezvous and more emulated
   delay before a byte lands. An eighth of a frame is a couple of milliseconds
   of emulated time, far below the millisecond-scale gaps between bytes the
   BIOS leaves. */
#define LINK_HORIZON (LINK_CLOCK_RATE / FPS / 8)

/* Peers whose protocol_id differs are never cabled to each other, which is
   what keeps this core off other machines' cables. */
#define LINK_PROTOCOL_ID "vmu-sio"

/* Known dump names for the two VMU BIOS revisions, plus the plain names a
   user is likely to give them. The first one present in the system
   directory wins. */
static const char *bios_names_american[] = {
   "en1005-19991026-315-6208-05.bin",
   "vmu_bios_en.bin",
   "vmu_bios.bin",
   NULL
};

static const char *bios_names_japanese[] = {
   "jp1004-19980930-315-6208-01.bin",
   "vmu_bios_jp.bin",
   "vmu_bios.bin",
   NULL
};

/* Fills biosPath with the first BIOS the frontend's system directory holds,
   and hands it to the VMU. Returns false when the core should fall back to
   its HLE boot. */
static bool loadBIOS(void)
{
   const char *system_dir = NULL;
   const char **lists[2];
   unsigned l, i;
   struct retro_variable var = {0};

   biosPath[0] = '\0';

   var.key = "bios";
   if(environment_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if(!strcmp(var.value, "disabled"))
         return false;
      if(!strcmp(var.value, "american"))
      {
         lists[0] = bios_names_american;
         lists[1] = NULL;
      }
      else if(!strcmp(var.value, "japanese"))
      {
         lists[0] = bios_names_japanese;
         lists[1] = NULL;
      }
      else
      {
         lists[0] = bios_names_american;
         lists[1] = bios_names_japanese;
      }
   }
   else
   {
      lists[0] = bios_names_american;
      lists[1] = bios_names_japanese;
   }

   if(!environment_cb(RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY, &system_dir)
         || !system_dir)
   {
      log_cb(RETRO_LOG_WARN, "[VeMUlator] no system directory, booting with HLE\n");
      return false;
   }

   for(l = 0; l < 2; l++)
   {
      if(!lists[l])
         break;

      for(i = 0; lists[l][i]; i++)
      {
         char path[4096];
         snprintf(path, sizeof(path), "%s%c%s",
               system_dir, RETRO_PATH_SEPARATOR, lists[l][i]);

         int status = vmu->loadBIOS(path);

         log_cb(RETRO_LOG_INFO, "[VeMUlator] BIOS %s: %s\n", path,
               status == 0 ? "loaded" :
               status == -1 ? "not found" : "not a usable image");

         if(status == 0)
         {
            strncpy(biosPath, path, sizeof(biosPath) - 1);
            biosPath[sizeof(biosPath) - 1] = '\0';
            return true;
         }
      }
   }

   log_cb(RETRO_LOG_WARN,
         "[VeMUlator] no BIOS in %s, booting with HLE\n", system_dir);

   return false;
}

/* Join the frontend's link bus, if it has one and the user wants it.
 *
 * The experimental number is probed first and the final one second, so a core
 * built today keeps working against a frontend that later adopts the
 * unflagged 94. */
static void linkAttach(void)
{
   struct retro_variable var = {0};

   link_port = NULL;
   link_tick = 0;
   link_safe = 0;

   var.key = "serial_link";
   if(environment_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var)
         && var.value && !strcmp(var.value, "disabled"))
      return;

   memset(&link_iface, 0, sizeof(link_iface));

   if(!environment_cb(RETRO_ENVIRONMENT_GET_LINK_INTERFACE, &link_iface)
         && !environment_cb(RETRO_ENVIRONMENT_GET_LINK_INTERFACE_FINAL, &link_iface))
   {
      log_cb(RETRO_LOG_INFO,
            "[VeMUlator] frontend hosts no link bus, serial port left empty\n");
      return;
   }

   link_port = link_iface.attach(0, LINK_PROTOCOL_ID, LINK_CLOCK_RATE);

   if(!link_port)
   {
      log_cb(RETRO_LOG_WARN, "[VeMUlator] could not attach to the link bus\n");
      return;
   }

   log_cb(RETRO_LOG_INFO, "[VeMUlator] attached to the link bus as \"%s\"\n",
         LINK_PROTOCOL_ID);
}

static void linkDetach(void)
{
   if(!link_port)
      return;

   link_iface.detach(link_port);
   link_port = NULL;
}

/* Hand the bus anything the VMU clocked out since the last look. Stamped at
   the published horizon rather than the cycle the eighth edge fell on, which
   is what makes the promise in safe_tick true, at the cost of up to a horizon
   of emulated delay on a byte. */
static void linkPumpOutgoing(void)
{
   int  iface;
   byte value;

   while(vmu->serial->takeOutgoing(&iface, &value))
   {
      byte payload[2];

      /* The cable crosses. Two VMUs clip together face to face, so one unit's
         connector is the mirror of the other's and SIO0 lands on the peer's
         SIO1. Both sides drive SO0/SCK0 and listen on SIO1, which only works
         because of that crossing -- wire it straight and both ends are
         talking into each other's outputs. */
      payload[0] = (byte)(iface ^ 1);
      payload[1] = value;

      /* One horizon ahead of where this core stands, which is exactly what it
         will publish as safe_tick on the next rendezvous. A peer cannot have
         run past it, so the byte is never delivered into its past. */
      link_iface.send(link_port, link_tick + LINK_HORIZON,
            RETRO_LINK_BROADCAST, payload, sizeof(payload));
   }
}

/* Take everything waiting for us. A bit-accurate model would hold each byte
   until the clock reached the tick it carries; this one delivers on arrival,
   which can be slightly early but is never late. */
static void linkDrainIncoming(void)
{
   uint64_t tick;
   unsigned from;
   byte     payload[8];
   size_t   len = sizeof(payload);

   while(link_iface.recv(link_port, &tick, &from, payload, &len))
   {
      if(len >= 2)
         vmu->serial->deliver((int)payload[0], payload[1]);

      len = sizeof(payload);
   }
}

/* Run one frame's worth of cycles, rendezvousing with the peer as often as the
   bus makes us. Falls back to a straight run when nothing is cabled. */
static void linkRunFrame(unsigned cyclesPassed)
{
   uint64_t target = link_tick + cyclesPassed;
   unsigned peers  = 0;
   int      stalls = 0;

   if(link_iface.peers(link_port, &peers) < 0 || peers < 2)
   {
      /* Nothing on the other end. Still advance the tick so a peer that
         arrives later does not find us in the distant past. */
      unsigned i;

      vmu->serial->setCabled(false);
      vmu->linkConnectorBits = 0;

      for(i = 0; i < cyclesPassed; i++)
         vmu->runCycle();

      link_tick = target;
      link_safe = target;
      return;
   }

   vmu->serial->setCabled(true);

   /* Connector pin 6, which is how software notices a peer is there at all,
      and what it checks before going near the serial port. */
   vmu->linkConnectorBits = 0x08;

   while(link_tick < target)
   {
      uint32_t wake  = RETRO_LINK_WAKE_NONE;
      uint64_t step  = link_tick + LINK_HORIZON;
      uint64_t grant;

      /* One horizon, never the whole frame. advance() blocks until the whole
         request can be granted and a peer only promises a horizon past where
         it stands, so two cores that each asked for a frame would deadlock,
         each waiting for the other to promise a frame it cannot promise
         without first running. A horizon at a time makes them leapfrog. */
      if(step > target)
         step = target;

      linkDrainIncoming();

      link_safe = step;
      grant     = link_iface.advance(link_port, link_tick, link_safe,
            step, &wake);

      if(grant == RETRO_LINK_UNBOUNDED || grant > step)
         grant = step;

      if(grant <= link_tick)
      {
         /* No ground given. That is normal when a peer woke us to hand over a
            message, and the next pass will pick it up -- but a frontend that
            keeps saying no must not wedge the emulation thread. */
         if(wake & RETRO_LINK_WAKE_DETACHED)
            break;

         if(wake == RETRO_LINK_WAKE_NONE && ++stalls > 64)
            break;

         continue;
      }

      stalls = 0;

      while(link_tick < grant)
      {
         vmu->runCycle();
         link_tick++;
      }

      linkPumpOutgoing();
   }

   /* A grant may have fallen short of the frame. Let the machine finish it
      anyway so video and audio keep their cadence; the tick stays honest. */
   while(link_tick < target)
   {
      vmu->runCycle();
      link_tick++;
   }
}

RETRO_API void retro_set_environment(retro_environment_t env)
{
   struct retro_vfs_interface_info vfs_iface_info;

   environment_cb = env;

   env(RETRO_ENVIRONMENT_SET_VARIABLES, options);

   {
      struct retro_log_callback log;
      log.log = NULL;
      if (env(RETRO_ENVIRONMENT_GET_LOG_INTERFACE, &log) && log.log)
         log_cb = log.log;
      else
         log_cb = fallback_log;
   }

   /* Take the frontend's VFS when it offers one. Every file this core touches
      already goes through RFILE - the ROM in retro_load_game, the flash save
      file, VMU::loadBIOS - so this hand-off is all that is needed for paths
      stdio cannot open on its own. Android's SAF hands out content:// URIs,
      and this core is need_fullpath, so it opens those paths itself. */
   vfs_iface_info.required_interface_version = 1;
   vfs_iface_info.iface                      = NULL;
   if (env(RETRO_ENVIRONMENT_GET_VFS_INTERFACE, &vfs_iface_info))
      filestream_vfs_init(&vfs_iface_info);
}

RETRO_API void retro_set_video_refresh(retro_video_refresh_t vr)
{
	video_cb = vr;
}

RETRO_API void retro_set_audio_sample(retro_audio_sample_t sample)
{
	audio_cb = sample;
}

RETRO_API void retro_set_audio_sample_batch(retro_audio_sample_batch_t batch)
{
	audio_batch_cb = batch;
}

RETRO_API void retro_set_input_poll(retro_input_poll_t ipoll)
{
	input_poll_cb = ipoll;
}

RETRO_API void retro_set_input_state(retro_input_state_t istate)
{
	input_state_cb = istate;
}

RETRO_API void retro_init(void)
{
	/* rtime_localtime, which VMU::setDate uses, needs this. */
	rtime_init();

	frameBuffer = (uint16_t*)calloc(SCREEN_WIDTH*SCREEN_HEIGHT_ICONS,
	                                sizeof(uint16_t));
	vmu         = new VMU(frameBuffer);
}

RETRO_API void retro_deinit(void)
{
	delete vmu;
	if(frameBuffer)
      free(frameBuffer);
	if(romData)
      free(romData);
   frameBuffer = NULL;
   romData     = NULL;

   rtime_deinit();
}

RETRO_API unsigned retro_api_version(void)
{
	return RETRO_API_VERSION;
}

RETRO_API void retro_get_system_info(struct retro_system_info *info)
{
	info->library_name     = "VeMUlator";
	info->library_version  = "0.1";
	info->valid_extensions = "vms|bin|dci";
	info->need_fullpath    = true;
	info->block_extract    = false;
}

RETRO_API void retro_get_system_av_info(struct retro_system_av_info *info)
{
	{
	   struct retro_variable var = {0};
	   var.key = "icon_row";
	   if(environment_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
	      iconRow = strcmp(var.value, "disabled") != 0;
	}

	info->geometry.base_width   = SCREEN_WIDTH;
	info->geometry.base_height  = screenHeight();
	info->geometry.max_width    = SCREEN_WIDTH;
	info->geometry.max_height   = SCREEN_HEIGHT_ICONS;
	info->geometry.aspect_ratio = 0;
	
	info->timing.fps            = FPS;
	info->timing.sample_rate    = SAMPLE_RATE;
}

RETRO_API void retro_set_controller_port_device(
      unsigned port, unsigned device)
{
	
}
 
void processInput()
{
   byte P3_reg, P3_int;
   int pressFlag = 0;

   input_poll_cb();

   if(!vmu->cpu->P3_taken)
      return;	//Don't accept new input until previous is processed

   P3_reg = vmu->ram->readByte_RAW(P3);
   P3_int = vmu->ram->readByte_RAW(P3INT);
   P3_reg = ~P3_reg;	//Active low

   //Up
   if(input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP))
   {
      P3_reg |= 1;
      pressFlag++;
   }
   else P3_reg &= 0xFE;

   //Down
   if(input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN))
   {
      P3_reg |= 2;
      pressFlag++;
   }
   else P3_reg &= 0xFD;

   //Left
   if(input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT))
   {
      P3_reg |= 4;
      pressFlag++;
   }
   else P3_reg &= 0xFB;

   //Right
   if(input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT))
   {
      P3_reg |= 8;
      pressFlag++;
   }
   else P3_reg &= 0xF7;

   //A
   if(input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A))
   {
      P3_reg |= 16;
      pressFlag++;
   }
   else P3_reg &= 0xEF;

   //B
   if(input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B))
   {
      P3_reg |= 32;
      pressFlag++;
   }
   else P3_reg &= 0xDF;

   /* MODE and SLEEP are only meaningful to the BIOS; without one the HLE
      boot hangs on them, so they stay dead in that case. */
   if(vmu->hasBIOS())
   {
      //Mode
      if(input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START))
      {
         P3_reg |= 64;
         pressFlag++;
      }
      else P3_reg &= 0xBF;

      //Sleep
      if(input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_SELECT))
      {
         P3_reg |= 128;
         pressFlag++;
      }
      else P3_reg &= 0x7F;
   }

   P3_reg = ~P3_reg;

   vmu->ram->writeByte_RAW(P3, P3_reg);

   if(pressFlag)
   {
      vmu->ram->writeByte_RAW(P3INT, P3_int | 2);
      vmu->intHandler->setP3();
      vmu->cpu->P3_taken = false;
   }

}

RETRO_API void retro_reset(void)
{
	vmu->reset();

	/* reset() throws away ROM and flash alike, so put both back. */
	if(biosPath[0])
		vmu->loadBIOS(biosPath);

	if(romData)
		vmu->flash->loadROM(romData, romSize, romType, romPath, flashWrite);

	vmu->startCPU();
}

RETRO_API void retro_run(void)
{
   unsigned i;
   unsigned int cyclesPassed;
	processInput();
	
	//Cycles passed since last screen refresh
	cyclesPassed = vmu->cpu->getCurrentFrequency() / FPS;
	
#ifdef VMU_LINK_SELFTEST
	/* Compiled out of every normal build. Stands in for guest code that drives
	   the serial port, so the link transport can be exercised end to end
	   without a mini-game that does it. */
	{
		static unsigned selftestFrame;

		/* CTRL has to be seen going 0 -> 1 by a cycle-sampling model, so the
		   clear and the set are put in different frames. Guest code gets this
		   for free: instructions take cycles. */
		switch(++selftestFrame % 40)
		{
			case 0:   /* clock a byte out */
				vmu->ram->writeByte_RAW(P1DDR, 0x05);
				vmu->ram->writeByte_RAW(P1FCR, 0x05);
				vmu->ram->writeByte_RAW(SBR,   0xF0);
				vmu->ram->writeByte_RAW(SBUF0, (byte)(selftestFrame / 40));
				vmu->ram->writeByte_RAW(SCON0, 0x08);
				break;
			case 10:  /* back to idle */
				vmu->ram->writeByte_RAW(SCON0, 0x00);
				break;
			case 20:  /* listen for one */
				vmu->ram->writeByte_RAW(P1DDR, 0x00);
				vmu->ram->writeByte_RAW(SCON0, 0x08);
				break;
			case 35:
				vmu->ram->writeByte_RAW(SCON0, 0x00);
				break;
		}
	}
#endif

	if(link_port)
		linkRunFrame(cyclesPassed);
	else
		for(i = 0; i < cyclesPassed; i++)
			vmu->runCycle();

	//Video
	vmu->video->drawFrame(frameBuffer);
	if(iconRow)
		vmu->video->drawIcons(frameBuffer);
	if(vmu->ram->readByte_RAW(MCR) & 8)
      video_cb(frameBuffer, SCREEN_WIDTH, screenHeight(), SCREEN_WIDTH * 2);
	
	//Audio
	vmu->audio->generateSignal(audio_batch_cb);
}

/* Save states. The flash is most of the size -- it is 128KB of the machine and
   a game writes to it -- so a state is a little over that. It is a fixed size
   for a given build, which is what the frontend asks for once and then keeps
   handing back. */
RETRO_API size_t retro_serialize_size(void)
{
   if(!vmu)
      return 0;

   return vmu->serializeSize();
}

RETRO_API bool retro_serialize(void *data, size_t size)
{
   if(!vmu || !data)
      return false;

   return vmu->saveState(data, size);
}

RETRO_API bool retro_unserialize(const void *data, size_t size)
{
   if(!vmu || !data)
      return false;

   return vmu->loadState(data, size);
}

RETRO_API void retro_cheat_reset(void) { }

RETRO_API void retro_cheat_set(unsigned index, bool enabled, const char *code)
{
}

RETRO_API bool retro_load_game(const struct retro_game_info *game)
{
   size_t i;
   //Set environment variables
   enum retro_pixel_format format = RETRO_PIXEL_FORMAT_RGB565;
   environment_cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &format);

   //Opening file
   RFILE *rom = rfopen(game->path, "rb");
   if(!rom)
      return false;
   rfseek(rom, 0, SEEK_END);
   romSize = rftell(rom);
   rfseek(rom, 0 , SEEK_SET);

   romData = (byte *)malloc(romSize);
   for(i = 0; i < romSize; i++)
      romData[i] = rfgetc(rom);

   rfclose(rom);

   //Check extension
   char *path = (char *)malloc(strlen(game->path) + 1);
   strcpy(path, game->path);
   // Last dot: directory names may contain dots too.
   char *ext = strrchr(path, '.');
   if(!ext)
   {
      free(path);
      return false;
   }

   //Check needed variables
   struct retro_variable var = {0};
   var.key = "enable_flash_write";
   environment_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var);

   //Loading ROM
   romType    = 0;
   flashWrite = false;
   if(!strcmp(ext, ".bin") || !strcmp(ext, ".BIN"))
   {
      //Check if user wants core to be able to write to flash
      flashWrite = (var.value && !strcmp(var.value, "enabled"));
   }
   else if(!strcmp(ext, ".vms") || !strcmp(ext, ".VMS")) romType = 1;
   else if(!strcmp(ext, ".dci") || !strcmp(ext, ".DCI")) romType = 2;

   strncpy(romPath, game->path, sizeof(romPath) - 1);
   romPath[sizeof(romPath) - 1] = 0;

   vmu->flash->loadROM(romData, romSize, romType, romPath, flashWrite);

   free(path);

   //Loading BIOS from the system directory, if the user provided one
   loadBIOS();

   //Joining the link bus, if the frontend hosts one
   linkAttach();

   /* "fixed" powers on at the same moment on every machine, which is what a
      netplay session needs: two peers seeding from their own wall clocks
      differ from the first frame. */
   var.key   = "clock";
   var.value = NULL;
   vmu->fixedClock = environment_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var)
         && var.value && !strcmp(var.value, "fixed");

   //Initializing system
   vmu->startCPU();

   return true;
}

RETRO_API bool retro_load_game_special(unsigned game_type, const struct retro_game_info *info, size_t num_info)
{
	return false;
}

RETRO_API void retro_unload_game(void)
{
	linkDetach();
	vmu->reset();
}

RETRO_API unsigned retro_get_region(void) { return RETRO_REGION_NTSC; }
/* The whole 128KB of flash is the save. A VMU has no separate save area: the
   mini-games live in the same flash the saves do, and a game writes its state
   back into its own file through the BIOS firmware call. Handing the frontend
   the flash rather than a private buffer also means the .srm is a VMU image,
   usable as a .bin. */
RETRO_API void *retro_get_memory_data(unsigned id)
{
   if(id != RETRO_MEMORY_SAVE_RAM || !vmu)
      return NULL;

   return vmu->flash->getDataPointer();
}

RETRO_API size_t retro_get_memory_size(unsigned id)
{
   if(id != RETRO_MEMORY_SAVE_RAM || !vmu)
      return 0;

   return VE_VMS_FLASH::DATA_SIZE;
}
