cmake_minimum_required(VERSION 3.5)

set(CONFIG_SOC_MEM_NON_CONTIGUOUS_SRAM 0 CACHE INTERNAL "" FORCE)

set(COMPONENTS "main retro-go mame4all")
include(../base.cmake)
project(mame4all)
