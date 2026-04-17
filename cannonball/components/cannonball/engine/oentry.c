#include "oentry.h"


// Initalize to default values
void oentry_init(oentry* entry, uint8_t i)
{
    entry->control = 0;
    entry->jump_index = i;
	entry->function_holder = -1;
	entry->id = 0;
	entry->shadow = 3;
	entry->zoom = 0;
	entry->draw_props = 0;
	entry->pal_src = 0;
	entry->pal_dst = 0;
	entry->x = 0;
	entry->y = 0;
	entry->width = 0;
	entry->priority = 0;
	entry->dst_index = 0;
	entry->addr = 0;
	entry->road_priority = 0;
	entry->reload = 0;
	entry->counter = 0;
    entry->xw1 = 0;
    entry->z = 0;
    entry->traffic_speed = 0;
    entry->type = 0;
    entry->xw2 = 0;
    entry->traffic_proximity = 0;
    entry->traffic_fx = 0;
    entry->traffic_orig_speed = 0;
    entry->traffic_near_speed = 0;
    entry->yw = 0;
    entry->pass_props = 0;
}
