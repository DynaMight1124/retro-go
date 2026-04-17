/***************************************************************************
    Animation Sequences.
    
    Used in three areas of the game:
    - The sequence at the start with the Ferrari driving in from the side
    - Flag Waving Man
    - 5 x End Sequences
    
    See "oanimsprite.h" for the specific format used by animated sprites.
    It is essentially a deviation from the normal sprites in the game.
    
    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#pragma once

#include "oanimsprite.h"


// Man at line with start flag
extern oanimsprite OAnimSeq_anim_flag;

// Ferrari Animation Sequence
extern oanimsprite OAnimSeq_anim_ferrari;               // 1

// Passenger Animation Sequences
extern oanimsprite OAnimSeq_anim_pass1;                 // 2
extern oanimsprite OAnimSeq_anim_pass2;                 // 3

// End Sequence Stuff
extern oanimsprite OAnimSeq_anim_obj1;                  // 4
extern oanimsprite OAnimSeq_anim_obj2;                  // 5
extern oanimsprite OAnimSeq_anim_obj3;                  // 6
extern oanimsprite OAnimSeq_anim_obj4;                  // 7
extern oanimsprite OAnimSeq_anim_obj5;                  // 8
extern oanimsprite OAnimSeq_anim_obj6;                  // 9
extern oanimsprite OAnimSeq_anim_obj7;                  // 10
extern oanimsprite OAnimSeq_anim_obj8;                  // 10

// End sequence to display (0-4)
extern uint8_t OAnimSeq_end_seq;


//void init(oentry*, oentry*, oentry*, oentry*);
void OAnimSeq_init(oentry*);
void OAnimSeq_flag_seq();
void OAnimSeq_ferrari_seq();
void OAnimSeq_anim_seq_intro(oanimsprite*);
void OAnimSeq_init_end_seq();
void OAnimSeq_tick_end_seq();

