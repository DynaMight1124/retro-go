#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include <devices/timer.h>
#include <proto/exec.h>
 
 
static ULONG basetime = 0;
struct MsgPort *timer_msgport;
struct timerequest *timer_ioreq;
struct Library *TimerBase;

static int opentimer(ULONG unit){
	timer_msgport = CreateMsgPort();
	timer_ioreq = CreateIORequest(timer_msgport, sizeof(*timer_ioreq));
	if (timer_ioreq){
		if (OpenDevice(TIMERNAME, unit, (APTR) timer_ioreq, 0) == 0){
			TimerBase = (APTR) timer_ioreq->tr_node.io_Device;
			return 1;
		}
	}
	return 0;
}
static void closetimer(void){
	if (TimerBase){
		CloseDevice((APTR) timer_ioreq);
	}
	DeleteIORequest(timer_ioreq);
	DeleteMsgPort(timer_msgport);
	TimerBase = 0;
	timer_ioreq = 0;
	timer_msgport = 0;
}

static struct timeval startTime;

void timerStartup(){
	GetSysTime(&startTime);
}

ULONG getMilliseconds(){
	struct timeval endTime;

	GetSysTime(&endTime);
	SubTime(&endTime,&startTime);

	return (endTime.tv_secs * 1000 + endTime.tv_micro / 1000);
}
 

//
// Same as I_GetTime, but returns time in milliseconds
//

int getTimeMS(void)
{
    ULONG ticks;

    ticks = getMilliseconds();

    if (basetime == 0)
        basetime = ticks;

    return ticks - basetime;
}

// Sleep for a specified number of ms


void exitTimer()
{
    closetimer();
}

void sleep(int ms)
{
    usleep(ms);
}

void waitVBL(int count)
{
    sleep((count * 1000) / 70);
}
 

void initTimer(void)
{
    // initialize timer

   opentimer(UNIT_VBLANK);
   timerStartup();
}
