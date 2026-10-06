/* Generated from tools/psp/recipes.json; contains no game data. */
#pragma once
#include <stdint.h>
#define MK64_ASSET_REGION_SIZE 13231904u
#define MK64_COURSE_WINDOW_BEGIN 324568u
#define MK64_COURSE_WINDOW_END 3434631u
typedef struct { uint32_t offset, size; } mk64_course_range_t;
static const mk64_course_range_t mk64_course_ranges[20] = {
    {1962168u, 146709u}, /* mario_raceway */
    {723432u, 131463u}, /* choco_mountain */
    {512272u, 211153u}, /* bowsers_castle */
    {324568u, 145701u}, /* banshee_boardwalk */
    {3249624u, 185007u}, /* yoshi_valley */
    {1068080u, 135117u}, /* frappe_snowland */
    {1479936u, 281415u}, /* koopa_troopa_beach */
    {2474560u, 219483u}, /* royal_raceway */
    {1761352u, 200815u}, /* luigi_raceway */
    {2108880u, 222151u}, /* moo_moo_farm */
    {2806624u, 262905u}, /* toads_turnpike */
    {1203200u, 276733u}, /* kalimari_desert */
    {2694048u, 92651u}, /* sherbet_land */
    {2331032u, 143521u}, /* rainbow_road */
    {3069536u, 180081u}, /* wario_stadium */
    {491096u, 21174u}, /* block_fort */
    {2786704u, 19915u}, /* skyscraper */
    {1058048u, 10025u}, /* double_deck */
    {854896u, 203145u}, /* dks_jungle_parkway */
    {470272u, 20821u}, /* big_donut */
};
