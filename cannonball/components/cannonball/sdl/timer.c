#include "sdl/timer.h"
#include <stdint.h>
/***************************************************************************
    SDL Based Timer.
    
    Will need to be replaced if SDL library is replaced.

    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

uint32_t getMilliseconds(){
	/*struct timeval endTime;

	GetSysTime(&endTime);
	SubTime(&endTime,&startTime);

	return (endTime.tv_secs * 1000 + endTime.tv_micro / 1000);*/
	return SDL_GetTicks();
}

void initTimer()
{}

void Timer_init(Timer* timer)
{
    //Initialize the variables
    timer->startTicks = 0;
    timer->pausedTicks = 0;
    timer->paused = 0;
    timer->started = 0;
}

void Timer_start(Timer* timer)
{
    //Start the timer
    timer->started = 1;

    //Unpause the timer
    timer->paused = 0;

    //Get the current clock time
    timer->startTicks = getMilliseconds();
}

void Timer_stop(Timer* timer)
{
    //Stop the timer
    timer->started = 0;

    //Unpause the timer
    timer->paused = 0;
}

void Timer_pause(Timer* timer)
{
    //If the timer is running and isn't already paused
    if( ( timer->started == 1 ) && ( timer->paused == 0 ) )
    {
        //Pause the timer
        timer->paused = 1;

        //Calculate the paused ticks
        timer->pausedTicks = getMilliseconds() - timer->startTicks;
    }
}

void Timer_unpause(Timer* timer)
{
    //If the timer is paused
    if( timer->paused == 1 )
    {
        //Unpause the timer
        timer->paused = 0;

        //Reset the starting ticks
        timer->startTicks = getMilliseconds() - timer->pausedTicks;

        //Reset the paused ticks
        timer->pausedTicks = 0;
    }
}

int Timer_get_ticks(Timer* timer)
{
    //If the timer is running
    if( timer->started == 1 )
    {
        //If the timer is paused
        if( timer->paused == 1 )
        {
            //Return the number of ticks when the timer was paused
            return timer->pausedTicks;
        }
        else
        {
            //Return the current time minus the start time
            return getMilliseconds() - timer->startTicks;
        }
    }

    //If the timer isn't running
    return 0;
}

uint8_t Timer_is_started(Timer* timer)
{
    return timer->started;
}

uint8_t Timer_is_paused(Timer* timer)
{
    return timer->paused;
}
