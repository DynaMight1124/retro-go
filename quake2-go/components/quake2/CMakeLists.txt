cmake_minimum_required(VERSION 3.5)

set(QUAKE_SOURCE_DIR source)

# Keep private callbacks visible to their own generated stable-ID table.
if(NOT CMAKE_SCRIPT_MODE_FILE)
find_package(Python3 REQUIRED COMPONENTS Interpreter)
file(GLOB Q2_SAVE_GAME_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/source/game/[gmp]_*.c")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
    ${Q2_SAVE_GAME_SOURCES} "${CMAKE_CURRENT_SOURCE_DIR}/scripts/generate_save_symbols.py")
execute_process(COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/scripts/generate_save_symbols.py"
    --game "${CMAKE_CURRENT_SOURCE_DIR}/source/game"
    --output "${CMAKE_CURRENT_BINARY_DIR}"
    RESULT_VARIABLE Q2_SAVE_GENERATION_RESULT)
if(NOT Q2_SAVE_GENERATION_RESULT EQUAL 0)
    message(FATAL_ERROR "Failed to generate Quake II save symbol registry")
endif()
endif()

# ref_soft renderer is the frame hot path. Boost it past the global -O2
# (CONFIG_COMPILER_OPTIMIZATION_PERF). Applied per-source so the rest of the
# engine keeps the default flags.
set(QUAKE2_REF_SOFT_SOURCES
	${QUAKE_SOURCE_DIR}/ref_soft/r_aclip.c
	${QUAKE_SOURCE_DIR}/ref_soft/r_alias.c
	${QUAKE_SOURCE_DIR}/ref_soft/r_bsp.c
	${QUAKE_SOURCE_DIR}/ref_soft/r_draw.c
	${QUAKE_SOURCE_DIR}/ref_soft/r_edge.c
	${QUAKE_SOURCE_DIR}/ref_soft/r_image.c
	${QUAKE_SOURCE_DIR}/ref_soft/r_light.c
	${QUAKE_SOURCE_DIR}/ref_soft/r_main.c
	${QUAKE_SOURCE_DIR}/ref_soft/r_misc.c
	${QUAKE_SOURCE_DIR}/ref_soft/r_model.c
	${QUAKE_SOURCE_DIR}/ref_soft/r_part.c
	${QUAKE_SOURCE_DIR}/ref_soft/r_poly.c
	${QUAKE_SOURCE_DIR}/ref_soft/r_polyse.c
	${QUAKE_SOURCE_DIR}/ref_soft/r_rast.c
	${QUAKE_SOURCE_DIR}/ref_soft/r_scan.c
	${QUAKE_SOURCE_DIR}/ref_soft/r_sprite.c
	${QUAKE_SOURCE_DIR}/ref_soft/r_surf.c
)

set(QUAKE2_SOURCES
	# Client
	${QUAKE_SOURCE_DIR}/client/cl_cin.c
	${QUAKE_SOURCE_DIR}/client/cl_ents.c
	${QUAKE_SOURCE_DIR}/client/cl_fx.c
	${QUAKE_SOURCE_DIR}/client/cl_input.c
	${QUAKE_SOURCE_DIR}/client/cl_inv.c
	${QUAKE_SOURCE_DIR}/client/cl_main.c
	${QUAKE_SOURCE_DIR}/client/cl_newfx.c
	${QUAKE_SOURCE_DIR}/client/cl_parse.c
	${QUAKE_SOURCE_DIR}/client/cl_pred.c
	${QUAKE_SOURCE_DIR}/client/cl_scrn.c
	${QUAKE_SOURCE_DIR}/client/cl_tent.c
	${QUAKE_SOURCE_DIR}/client/cl_view.c
	${QUAKE_SOURCE_DIR}/client/console.c
	${QUAKE_SOURCE_DIR}/client/keys.c
	${QUAKE_SOURCE_DIR}/client/menu.c
	${QUAKE_SOURCE_DIR}/client/qmenu.c
	${QUAKE_SOURCE_DIR}/client/snd_dma.c
	${QUAKE_SOURCE_DIR}/client/snd_mem.c
	${QUAKE_SOURCE_DIR}/client/snd_mix.c

	# Server
	${QUAKE_SOURCE_DIR}/server/sv_ccmds.c
	${QUAKE_SOURCE_DIR}/server/sv_ents.c
	${QUAKE_SOURCE_DIR}/server/sv_game.c
	${QUAKE_SOURCE_DIR}/server/sv_init.c
	${QUAKE_SOURCE_DIR}/server/sv_main.c
	${QUAKE_SOURCE_DIR}/server/sv_send.c
	${QUAKE_SOURCE_DIR}/server/sv_user.c
	${QUAKE_SOURCE_DIR}/server/sv_world.c

	# Qcommon
	${QUAKE_SOURCE_DIR}/qcommon/cmd.c
	${QUAKE_SOURCE_DIR}/qcommon/cmodel.c
	${QUAKE_SOURCE_DIR}/qcommon/common.c
	${QUAKE_SOURCE_DIR}/qcommon/crc.c
	${QUAKE_SOURCE_DIR}/qcommon/cvar.c
	${QUAKE_SOURCE_DIR}/qcommon/files.c
	${QUAKE_SOURCE_DIR}/qcommon/md4.c
	${QUAKE_SOURCE_DIR}/qcommon/net_chan.c
	${QUAKE_SOURCE_DIR}/qcommon/pmove.c

	# Other
	${QUAKE_SOURCE_DIR}/other/glob.c
	${QUAKE_SOURCE_DIR}/other/q_arena.c
	${QUAKE_SOURCE_DIR}/other/q_hunk.c
	${QUAKE_SOURCE_DIR}/other/q_system.c
	${QUAKE_SOURCE_DIR}/other/vid_lib.c
	${QUAKE_SOURCE_DIR}/other/vid_menu.c

	# Net (loopback + IPv4 UDP via lwIP BSD sockets)
	${QUAKE_SOURCE_DIR}/net/net_udp.c

	# CD music is implemented by the application's WAV streaming adapter.
	# Software renderer
	${QUAKE2_REF_SOFT_SOURCES}

	# Game (embedded)
	${QUAKE_SOURCE_DIR}/game/g_ai.c
	${QUAKE_SOURCE_DIR}/game/g_chase.c
	${QUAKE_SOURCE_DIR}/game/g_cmds.c
	${QUAKE_SOURCE_DIR}/game/g_combat.c
	${QUAKE_SOURCE_DIR}/game/g_func.c
	${QUAKE_SOURCE_DIR}/game/g_items.c
	${QUAKE_SOURCE_DIR}/game/g_main.c
	${QUAKE_SOURCE_DIR}/game/g_misc.c
	${QUAKE_SOURCE_DIR}/game/g_monster.c
	${QUAKE_SOURCE_DIR}/game/g_phys.c
	${QUAKE_SOURCE_DIR}/game/g_save.c
	${QUAKE_SOURCE_DIR}/game/save_build.c
	${QUAKE_SOURCE_DIR}/game/save_symbols.c
	${CMAKE_CURRENT_BINARY_DIR}/save_registry.c
	${QUAKE_SOURCE_DIR}/game/g_spawn.c
	${QUAKE_SOURCE_DIR}/game/g_svcmds.c
	${QUAKE_SOURCE_DIR}/game/g_target.c
	${QUAKE_SOURCE_DIR}/game/g_trigger.c
	${QUAKE_SOURCE_DIR}/game/g_turret.c
	${QUAKE_SOURCE_DIR}/game/g_utils.c
	${QUAKE_SOURCE_DIR}/game/g_weapon.c
	${QUAKE_SOURCE_DIR}/game/m_actor.c
	${QUAKE_SOURCE_DIR}/game/m_berserk.c
	${QUAKE_SOURCE_DIR}/game/m_boss2.c
	${QUAKE_SOURCE_DIR}/game/m_boss3.c
	${QUAKE_SOURCE_DIR}/game/m_boss31.c
	${QUAKE_SOURCE_DIR}/game/m_boss32.c
	${QUAKE_SOURCE_DIR}/game/m_brain.c
	${QUAKE_SOURCE_DIR}/game/m_chick.c
	${QUAKE_SOURCE_DIR}/game/m_flash.c
	${QUAKE_SOURCE_DIR}/game/m_flipper.c
	${QUAKE_SOURCE_DIR}/game/m_float.c
	${QUAKE_SOURCE_DIR}/game/m_flyer.c
	${QUAKE_SOURCE_DIR}/game/m_gladiator.c
	${QUAKE_SOURCE_DIR}/game/m_gunner.c
	${QUAKE_SOURCE_DIR}/game/m_hover.c
	${QUAKE_SOURCE_DIR}/game/m_infantry.c
	${QUAKE_SOURCE_DIR}/game/m_insane.c
	${QUAKE_SOURCE_DIR}/game/m_medic.c
	${QUAKE_SOURCE_DIR}/game/m_move.c
	${QUAKE_SOURCE_DIR}/game/m_mutant.c
	${QUAKE_SOURCE_DIR}/game/m_parasite.c
	${QUAKE_SOURCE_DIR}/game/m_soldier.c
	${QUAKE_SOURCE_DIR}/game/m_supertank.c
	${QUAKE_SOURCE_DIR}/game/m_tank.c
	${QUAKE_SOURCE_DIR}/game/p_client.c
	${QUAKE_SOURCE_DIR}/game/p_hud.c
	${QUAKE_SOURCE_DIR}/game/p_trail.c
	${QUAKE_SOURCE_DIR}/game/p_view.c
	${QUAKE_SOURCE_DIR}/game/p_weapon.c
	${QUAKE_SOURCE_DIR}/game/q_shared.c

	# ESP32 platform layer
	${QUAKE_SOURCE_DIR}/port_esp32.c
)

idf_component_register(
	SRCS ${QUAKE2_SOURCES}
	INCLUDE_DIRS ${QUAKE_SOURCE_DIR}
	PRIV_INCLUDE_DIRS ${QUAKE_SOURCE_DIR} ${QUAKE_SOURCE_DIR}/game ${CMAKE_CURRENT_BINARY_DIR}
	PRIV_REQUIRES p4_memcpy lwip retro-go
)
rg_setup_compile_options()

# Keep the upstream engine's warning policy local; board adapters and IDF
# drivers must still report their diagnostics.
target_compile_options(${COMPONENT_LIB} PRIVATE "-w")

# Keep measured stack usage for the helper used on the internal UI stack.
set_source_files_properties(${QUAKE_SOURCE_DIR}/game/q_shared.c PROPERTIES
	COMPILE_OPTIONS "-fstack-usage")

set_source_files_properties(${QUAKE2_REF_SOFT_SOURCES} PROPERTIES
	COMPILE_OPTIONS "-O3;-funroll-loops;-fno-math-errno;-fno-stack-protector"
)
