/***************************************************************************
    CannonBoard Hardware Interface. 
    
    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#pragma once
#include <stdint.h>
#include "globals.h"
#include <string.h>

#ifdef CANNONBOARD
#include <boost/thread/mutex.hpp>
#endif

#ifdef CANNONBOARD
class CallbackAsyncSerial;
#endif

// Packet Length including CBALL Header
#define PACKET_LENGTH 15

typedef struct
{
    // Message Count
    uint8_t msg_count;

    // Last message received
    uint8_t msg_received;

    uint8_t status;
    uint8_t di1;
    uint8_t di2;
    uint8_t mci;     // Moving Cabinet: Motor Limit Values
    uint8_t ai0;     // Acceleration
    uint8_t ai1;     // Moving Cabinet: Motor Position
    uint8_t ai2;     // Steering
    uint8_t ai3;     // Braking
} Packet;

#define INCOMINGSTATUS_FOUND     0x01 // Packet Found
#define INCOMINGSTATUS_CSUM      0x02 // Last Packet Checksum was OK
#define INCOMINGSTATUS_PAST_FAIL 0x04 // Checksum has failed at some point in the past
#define INCOMINGSTATUS_RESET     0x08 // Interface is resetting
#define INCOMINGSTATUS_MISSED    0x10 // Interface missed a packet (using message counter)

// Incoming Packets Found
extern uint32_t Interface_stats_found_in;
// Incoming Packets Not Found
extern uint32_t Interface_stats_notfound_in;
// Incoming Packets With Error
extern uint32_t Interface_stats_error_in;
// Outbound Packets Found
extern uint32_t Interface_stats_found_out;
// Outbound Packets Missed
extern uint32_t Interface_stats_missed_out;
// Outbound Packets With Error
extern uint32_t Interface_stats_error_out;


void Interface_init(const char* port, unsigned int baud);
void Interface_reset_stats();
void Interface_close();
void Interface_start();
void Interface_stop();
void Interface_write(uint8_t dig_out, uint8_t mc_out);
uint8_t Interface_started();
Packet* Interface_get_packet();



