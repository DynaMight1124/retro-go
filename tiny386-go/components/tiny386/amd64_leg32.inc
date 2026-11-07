#define I386_IN_AMD64
#include "i386.h"

static int relay_pic_read_irq(void *o)
{
	CPU_CB *cb = o;
	return cb->pic_read_irq(cb->pic);
}

static u8 relay_io_read8(void *o, int port)
{
	CPU_CB *cb = o;
	return cb->io_read8(cb->io, port);
}

static void relay_io_write8(void *o, int port, u8 val)
{
	CPU_CB *cb = o;
	cb->io_write8(cb->io, port, val);
}

static u16 relay_io_read16(void *o, int port)
{
	CPU_CB *cb = o;
	return cb->io_read16(cb->io, port);
}

static void relay_io_write16(void *o, int port, u16 val)
{
	CPU_CB *cb = o;
	cb->io_write16(cb->io, port, val);
}

static u32 relay_io_read32(void *o, int port)
{
	CPU_CB *cb = o;
	return cb->io_read32(cb->io, port);
}

static void relay_io_write32(void *o, int port, u32 val)
{
	CPU_CB *cb = o;
	cb->io_write32(cb->io, port, val);
}

static int relay_io_read_string(void *o, int port, uint8_t *buf, int size, int count)
{
	CPU_CB *cb = o;
	return cb->io_read_string(cb->io, port, buf, size, count);
}

static int relay_io_write_string(void *o, int port, uint8_t *buf, int size, int count)
{
	CPU_CB *cb = o;
	return cb->io_write_string(cb->io, port, buf, size, count);
}

static u8 relay_iomem_read8(void *o, u32 port)
{
	CPU_CB *cb = o;
	return cb->iomem_read8(cb->iomem, port);
}

static void relay_iomem_write8(void *o, u32 port, u8 val)
{
	CPU_CB *cb = o;
	cb->iomem_write8(cb->iomem, port, val);
}

static u16 relay_iomem_read16(void *o, u32 port)
{
	CPU_CB *cb = o;
	return cb->iomem_read16(cb->iomem, port);
}

static void relay_iomem_write16(void *o, u32 port, u16 val)
{
	CPU_CB *cb = o;
	cb->iomem_write16(cb->iomem, port, val);
}

static u32 relay_iomem_read32(void *o, u32 port)
{
	CPU_CB *cb = o;
	return cb->iomem_read32(cb->iomem, port);
}

static void relay_iomem_write32(void *o, u32 port, u32 val)
{
	CPU_CB *cb = o;
	cb->iomem_write32(cb->iomem, port, val);
}

static bool relay_iomem_write_string(void *o, u32 port, uint8_t *buf, int len)
{
	CPU_CB *cb = o;
	return cb->iomem_write_string(cb->iomem, port, buf, len);
}

static void leg32_init(CPUAMD64 *cpu)
{
	CPUI386_CB *cbi386;
	cpu->cpui386 = cpui386_new(6, (char *)cpu->phys_mem,
				   cpu->phys_mem_size, &cbi386);
	cpui386_enable_fpu(cpu->cpui386);
	cbi386->pic = &(cpu->cb);
	cbi386->pic_read_irq = relay_pic_read_irq;
	cbi386->io = &(cpu->cb);
	cbi386->io_read8 = relay_io_read8;
	cbi386->io_write8 = relay_io_write8;
	cbi386->io_read16 = relay_io_read16;
	cbi386->io_write16 = relay_io_write16;
	cbi386->io_read32 = relay_io_read32;
	cbi386->io_write32 = relay_io_write32;
	cbi386->io_read_string = relay_io_read_string;
	cbi386->io_write_string = relay_io_write_string;
	cbi386->iomem = &(cpu->cb);
	cbi386->iomem_read8 = relay_iomem_read8;
	cbi386->iomem_write8 = relay_iomem_write8;
	cbi386->iomem_read16 = relay_iomem_read16;
	cbi386->iomem_write16 = relay_iomem_write16;
	cbi386->iomem_read32 = relay_iomem_read32;
	cbi386->iomem_write32 = relay_iomem_write32;
	cbi386->iomem_write_string = relay_iomem_write_string;
}

static void leg32_free(CPUAMD64 *cpu)
{
	if (cpu->cpui386) {
		cpui386_delete(cpu->cpui386);
		cpu->cpui386 = NULL;
	}
}

static void leg32_reset(CPUAMD64 *cpu)
{
	if (!cpu->cpui386) {
		leg32_init(cpu);
	}
	cpui386_reset(cpu->cpui386);
}

static bool IRAM_ATTR leg32_raise_irq(CPUAMD64 *cpu)
{
	if (cpu->cpui386) {
		cpui386_raise_irq(cpu->cpui386);
		return true;
	}
	return false;
}

static bool leg32_set_gpr(CPUAMD64 *cpu, int i, u32 val)
{
	if (cpu->cpui386) {
		if (i < 8)
			cpui386_set_gpr(cpu->cpui386, i, val);
		return true;
	}
	return false;
}

static bool leg32_step(CPUAMD64 *cpu, int stepcount)
{
	if (cpu->cpui386) {
		cpui386_step(cpu->cpui386, stepcount);
		if (cpui386_get_excno(cpu->cpui386) == 0x8664) {
			CPUI386_State state;
			cpui386_get_state(cpu->cpui386, &state);
			leg32_free(cpu);
			for (int i = 0; i < 8; i++)
				REGi(i) = state.gpr[i];
			for (int i = 0; i < 8; i++)
				cpu->seg[i].sel = state.seg[i];
			cpu->ip = cpu->next_ip = state.ip;
			cpu->cc.mask = 0;
			cpu->flags = state.flags;
			cpu->gdt.base = state.gdt_base;
			cpu->gdt.limit = state.gdt_limit;
			cpu->idt.base = state.idt_base;
			cpu->idt.limit = state.idt_limit;
			cpu->cr0 = state.cr0;
			cpu->cr2 = state.cr2;
			cpu->cr3 = state.cr3;
			cpu->cr3_valid = true;
			cpu->efer = state.efer;
			tlb_clear(cpu);
		}
		return true;
	}
	return false;
}
