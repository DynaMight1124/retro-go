# mk64k.prx: kernel-mode Media Engine boot (see mek.c).  Linked without crt0:
# its startup chain (newlib glue) makes ForUser syscalls, which fail in
# kernel mode.  libpspmodinfo provides the .lib.ent/.lib.stub markers the
# module info references.
PSPSDK := $(shell psp-config --pspsdk-path)
CFLAGS := -O2 -G0 -Wall -I$(PSPSDK)/include -I../../../src/port/psp -D_PSP_FW_VERSION=600
LDFLAGS := -specs=$(PSPSDK)/lib/prxspecs -Wl,-q,-T$(PSPSDK)/lib/linkfile.prx -Wl,-zmax-page-size=128 -nostdlib -nostartfiles
LIBS := -lpspmodinfo -lpspkernel -lgcc
all: mk64k.prx
mek.o: mek.c ../../../src/port/psp/me_share.h
	psp-gcc $(CFLAGS) -c $< -o $@
me_stub.o: me_stub.S
	psp-gcc $(CFLAGS) -c $< -o $@
mk64k.elf: mek.o me_stub.o
	psp-gcc $(LDFLAGS) $(PSPSDK)/lib/prxexports.o mek.o me_stub.o -L$(PSPSDK)/lib $(LIBS) -o $@
	psp-fixup-imports $@
mk64k.prx: mk64k.elf
	psp-prxgen $< $@
clean:
	rm -f mek.o me_stub.o mk64k.elf mk64k.prx
