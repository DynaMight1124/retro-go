/***************************************************************************
    Traffic Routines.

    - Traffic spawning.
    - Traffic logic, lane changing & movement.
    - Collisions.
    - Traffic panning and volume control to pass to sound program.

    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#pragma once

#include "outrun.h"


// AI: Set to denote enemy traffic is close to car.
extern uint8_t OTraffic_ai_traffic;

// Set to denote we should go to LHS of road on bonus
extern uint8_t OTraffic_bonus_lhs;

// Logic for traffic based on road split
extern int8_t OTraffic_traffic_split;

// Denotes Collision With Other Traffic
//
// 0 = No Collision
// 1 = Init Collision Sequence
// 2 = Collision Sequence In Progress
extern uint16_t OTraffic_collision_traffic;

extern uint16_t OTraffic_collision_mask;

void OTraffic_init();
void OTraffic_init_stage1_traffic();
void OTraffic_tick();
void OTraffic_disable_traffic();
void OTraffic_set_max_traffic();
void OTraffic_traffic_logic();
void OTraffic_traffic_sound();


