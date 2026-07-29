cmake_minimum_required(VERSION 3.5)

# App-local profiling level. This does not affect the launcher:
#   0 = off (normal/default build)
#   1 = basic adapter timing
#   2 = level 1 + Genesis CPU, VDP and sound profiling
#   3 = levels 1/2 + Sega CD and CHD profiling
set(PICODRIVE_GO_PROFILE_LEVEL 0)

set(COMPONENTS "main retro-go picodrive")
set(RG_ENABLE_NETWORKING 0)
include(../base.cmake)
project(picodrive-go)
