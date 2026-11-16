# GTA Vice City J2ME -> C port for SF2000 / GB300
# Host build + MIPS soft-float cross build (no FPU on the target).

MIPS_ELF  ?= /opt/frog-toolchain/mipsel-mti-elf_stable/bin/mipsel-mti-elf-
HOST_CC   ?= gcc
MULTICORE ?= /home/Sajnaps/gb300/sf2000_multicore
FROGGY_TYPE ?= GB300V2

PORT_SRC  := $(wildcard src/port/*.c)
PLAT_SRC  := src/platform/j2me_core.c src/platform/j2me_gfx.c
CORE_SRC  := libretro/gtavc_libretro.c

HOST_CFLAGS := -std=gnu11 -O3 -g -Wall -Wextra -Wno-unused-parameter -fwrapv \
               -I src/platform -I src/port -I third_party -I libretro
HOST_BIN    := build/gtavc_runner

SFLAGS := -EL -march=mips32 -mtune=mips32r2 -msoft-float -O3 -G0 -MMD -MP \
          -mno-abicalls -fno-pic -ffreestanding -fno-strict-aliasing -fwrapv \
          -ffunction-sections -fdata-sections -fomit-frame-pointer \
          -fno-unwind-tables -fno-asynchronous-unwind-tables \
          -DSF2000 -DGB300 -std=gnu11 -I src/platform -I src/port \
          -I third_party -I libretro
SF_BUILD := build_sf2000
SF_OBJS  := $(patsubst src/%.c,$(SF_BUILD)/%.o,$(PORT_SRC) $(PLAT_SRC))
SF_OBJS  += $(SF_BUILD)/gtavc_libretro.o

.PHONY: all host run mips-test core device clean

# The multicore frontend invokes this Makefile with platform=sf2000 and then
# copies the resulting .a.  Build the cross archive in that case.
ifeq ($(platform),sf2000)
all: gtavc_libretro_sf2000.a
else
all: host
endif

build:
	@mkdir -p build

$(HOST_BIN): $(PORT_SRC) $(PLAT_SRC) runner/test_runner.c $(wildcard src/port/*.h) src/platform/j2me.h third_party/stb_image.h | build
	$(HOST_CC) $(HOST_CFLAGS) $(PORT_SRC) $(PLAT_SRC) runner/test_runner.c -o $@ -lm

host: $(HOST_BIN)

run: host
	GTAVC_SEED=1234567 ./$(HOST_BIN) assets screenshots

$(SF_BUILD)/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(MIPS_ELF)gcc $(SFLAGS) -c $< -o $@

$(SF_BUILD)/gtavc_libretro.o: libretro/gtavc_libretro.c
	@mkdir -p $(SF_BUILD)
	$(MIPS_ELF)gcc $(SFLAGS) -c $< -o $@

gtavc_libretro_sf2000.a: $(SF_OBJS)
	$(MIPS_ELF)ar rcs $@ $^
	@echo "Built $@"

core: gtavc_libretro_sf2000.a

# The multicore frontend builds $(CORE) with platform=sf2000 and copies its .a
device: core
	$(MAKE) -C $(MULTICORE) CORE=$(CURDIR) CONSOLE=gtavc FROGGY_TYPE=$(FROGGY_TYPE)
	@mkdir -p dist
	@EXT=gb3k; [ "$(FROGGY_TYPE)" = "SF2000" ] && EXT=sf2k; \
	 cp -f $(MULTICORE)/build/core_87000000 dist/gtavc_$(FROGGY_TYPE).$$EXT; \
	 ls -lh dist/gtavc_$(FROGGY_TYPE).$$EXT

# Run the same integration scenario on a MIPS32 soft-float binary under qemu.
mips-test:
	./tools/test_mips.sh

-include $(SF_OBJS:.o=.d)

clean:
	rm -rf build build_sf2000 gtavc_libretro_sf2000.a
