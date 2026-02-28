/*
 * wARM - exporting ARM processor specific privileged services to userspace
 * kernelspace part
 *
 * Author: Gražvydas "notaz" Ignotas
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/seq_file.h>

#include <linux/version.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,11)
#include <linux/uaccess.h>
#else
#include <linux/init.h>
#include <linux/moduleparam.h>
#include <asm/uaccess.h>
#define __user
#define unlocked_ioctl ioctl
#endif

#define WARM_CODE
#include "../warm.h"
#include "warm_ops.h"

#ifndef CONFIG_PROC_FS
#error need proc_fs
#endif
	
#define WARM_VER "r3"
#define PFX "wARM: "

#define WARM_INFO(fmt, ...) \
	if (verbose) \
		pr_info(PFX fmt, ##__VA_ARGS__)

#define SECTION_SIZE		0x100000
#define MAX_CACHEOP_RANGE	16384

/* assume RAM starts at phys addr 0 (this is really machine specific) */
#define RAM_PHYS_START	0
#define RAM_MAX_SIZE	0x10000000	/* 256M, try to be future proof */

/* expected CPU id */
#if   defined(CONFIG_CPU_ARM926T)
#define EXPECTED_ID	0x069260
#elif defined(CONFIG_CPU_ARM920T)
#define EXPECTED_ID	0x029200
#else
#error "unsupported CPU"
#endif

extern unsigned long max_mapnr;

/* "upper" physical memory, not seen by Linux and to be mmap'ed */
static u32 uppermem_start;
static u32 uppermem_end;
static spinlock_t lock;

static int verbose;

static u32 *get_pgtable(void)
{
	u32 ttb;

	/* get the pointer to the translation table base... */
	asm ("mrc p15, 0, %0, c2, c0, 0" : "=r"(ttb));

	return __va((ttb) & 0xffffc000);
}

static int do_set_cb_uppermem(int in_cb, int is_set)
{
	unsigned long flags;
	u32 *pgtable, *cpt;
	int i, j, count = 0;
	int bits = 0;

	if (uppermem_end <= uppermem_start)
		return -ENODEV;

	if (in_cb & WCB_C_BIT)
		bits |= 8;
	if (in_cb & WCB_B_BIT)
		bits |= 4;

	spin_lock_irqsave(&lock, flags);

	pgtable = get_pgtable();

	for (i = 0; i < 4096; i++)
	{
		if ((pgtable[i] & 3) != 1)
			/* must be coarse page table */
			continue;

		cpt = __va(pgtable[i] & 0xfffffc00);

		for (j = 0; j < 256; j++)
		{
			u32 addr, entry;

			entry = cpt[j];
			if (!(entry & 3))
				/* fault */
				continue;

			addr = entry & 0xfffff000;
			if (uppermem_start <= addr && addr < uppermem_end)
			{
				pr_debug(PFX "%s C&B bits %08x\n",
					is_set ? "set" : "clear", entry);

				if (is_set)
					entry |= bits;
				else
					entry &= ~bits;

				/* need to also set AP bits (so that no faults
				 * happen and kernel doesn't touch this after us) */
				if ((entry & 3) == 3)
					entry |= 0x030;	/* tiny page */
				else
					entry |= 0xff0;

				cpt[j] = entry;
				count++;
			}
		}
	}

	warm_cop_clean_d();
	warm_drain_wb_inval_tlb();

	spin_unlock_irqrestore(&lock, flags);

	WARM_INFO("%c%c bit(s) %s for phys %08x-%08x (%d pages)\n",
		bits & 8 ? 'c' : ' ', bits & 4 ? 'b' : ' ',
		is_set ? "set" : "cleared",
		uppermem_start, uppermem_end - 1, count);

	return 0;
}

static int do_set_cb_virt(int in_cb, int is_set, u32 addr, u32 size)
{
	int count = 0, bits = 0;
	unsigned long flags;
	u32 desc1, desc2 = 0;
	u32 *pgtable, *cpt = NULL;
	u32 start, end;
	u32 mask;

	if (in_cb & WCB_C_BIT)
		bits |= 8;
	if (in_cb & WCB_B_BIT)
		bits |= 4;

	mask = PAGE_SIZE - 1;
	size += addr & mask;
	size = (size + mask) & ~mask;

	addr &= ~(PAGE_SIZE - 1);
	start = addr;
	end = addr + size;

	spin_lock_irqsave(&lock, flags);

	pgtable = get_pgtable();

	while (addr < end)
	{
		desc1 = pgtable[addr >> 20];

		switch (desc1 & 3) {
		case 0:
			spin_unlock_irqrestore(&lock, flags);
			printk(KERN_WARNING PFX "address %08x not mapped.\n", addr);
			return -EINVAL;
		case 1:
			/* coarse table */
			cpt = __va(desc1 & 0xfffffc00);
			desc2 = cpt[(addr >> 12) & 0xff];
			break;
		case 2:
			/* section */
			if (is_set)
				desc1 |= bits;
			else
				desc1 &= ~bits;
			pgtable[addr >> 20] = desc1;
			addr += SECTION_SIZE;
			count++;
			continue;
		case 3:
			cpt = __va(desc1 & 0xfffff000);
			desc2 = cpt[(addr >> 10) & 0x3ff];
			break;
		}

		if ((desc2 & 3) == 0) {
			spin_unlock_irqrestore(&lock, flags);
			printk(KERN_WARNING PFX "address %08x not mapped (%08x)\n",
				addr, desc2);
			return -EINVAL;
		}

		if (is_set)
			desc2 |= bits;
		else
			desc2 &= ~bits;

		/* this might be bad idea, better let it fault so that Linux does
		 * it's accounting, but that will drop CB bits, so keep this
		 * for compatibility */
		if ((desc2 & 3) == 2)
			desc2 |= 0xff0;

		switch (desc1 & 3) {
		case 1:
			cpt[(addr >> 12) & 0xff] = desc2;
			break;
		case 3:
			cpt[(addr >> 10) & 0x3ff] = desc2;
			break;
		}

		addr += PAGE_SIZE;
		count++;
	}

	warm_cop_clean_d();
	warm_drain_wb_inval_tlb();

	spin_unlock_irqrestore(&lock, flags);

	WARM_INFO("%c%c bit(s) %s virt %08x-%08x (%d pages)\n",
		bits & 8 ? 'c' : ' ', bits & 4 ? 'b' : ' ',
		is_set ? "set" : "cleared", start, end - 1, count);

	return 0;
}

static int do_virt2phys(unsigned long *_addr)
{
	u32 addr = *_addr;
	u32 desc1, desc2;
	u32 *pgtable, *cpt;

	pgtable = get_pgtable();
	desc1 = pgtable[addr >> 20];

	switch (desc1 & 3) {
	case 1:	/* coarse table */
		cpt = __va(desc1 & 0xfffffc00);
		desc2 = cpt[(addr >> 12) & 0xff];
		break;
	case 2: /* 1MB section */
		*_addr = (desc1 & 0xfff00000) | (addr & 0xfffff);
		return 0;
	case 3:	/* fine table */
		cpt = __va(desc1 & 0xfffff000);
		desc2 = cpt[(addr >> 10) & 0x3ff];
		break;
	default:
		return -EINVAL;
	}

	switch (desc2 & 3) {
	case 1:	/* large page */
		*_addr = (desc2 & ~0xffff) | (addr & 0xffff);
		break;
	case 2:	/* small page */
		*_addr = (desc2 & ~0x0fff) | (addr & 0x0fff);
		break;
	case 3:	/* tiny page */
		*_addr = (desc2 & ~0x03ff) | (addr & 0x03ff);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int do_cache_ops_whole(int ops)
{
	if ((ops & (WOP_D_CLEAN|WOP_D_INVALIDATE)) == (WOP_D_CLEAN|WOP_D_INVALIDATE))
		warm_cop_clean_inval_d();

	else if (ops & WOP_D_CLEAN)
		warm_cop_clean_d();

	else if (ops & WOP_D_INVALIDATE) {
		printk(KERN_WARNING PFX "invalidate without clean is dangerous!\n");
		warm_cop_inval_d();
	}

	if (ops & WOP_I_INVALIDATE)
		warm_cop_inval_i();
	
	warm_cop_drain_wb();
	return 0;
}

static int do_cache_ops(int ops, u32 addr, u32 size)
{
	if (addr & 31) {
		size += addr & 31;
		addr &= ~31;
	}

	switch (ops) {
	case WOP_D_CLEAN|WOP_D_INVALIDATE|WOP_I_INVALIDATE:
		warm_cop_r_clean_d_inval_di(addr, size);
		break;
	case WOP_D_CLEAN|WOP_D_INVALIDATE:
		warm_cop_r_clean_d_inval_d(addr, size);
		break;
	case WOP_D_CLEAN|WOP_I_INVALIDATE:
		warm_cop_r_clean_d_inval_i(addr, size);
		break;
	case WOP_D_CLEAN:
		warm_cop_r_clean_d(addr, size);
		break;
	case WOP_D_INVALIDATE|WOP_I_INVALIDATE:
		warm_cop_r_inval_di(addr, size);
		break;
	case WOP_D_INVALIDATE:
		warm_cop_r_inval_d(addr, size);
		break;
	case WOP_I_INVALIDATE:
		warm_cop_r_inval_i(addr, size);
		break;
	default:
		/* only drain wb */
		break;
	}

	warm_cop_drain_wb();
	return 0;
}

static int do_map_op(u32 vaddr, u32 paddr, u32 size, int cb, int is_unmap)
{
	int count = 0, retval = 0;
	unsigned long flags;
	u32 pstart, start, end;
	u32 desc1, apcb_bits;
	u32 *pgtable;
	u32 v, mask;

	apcb_bits = (3 << 10) | (1 << 5); /* r/w, dom 1 */
	if (cb & WCB_C_BIT)
		apcb_bits |= 8;
	if (cb & WCB_B_BIT)
		apcb_bits |= 4;

	mask = SECTION_SIZE - 1;
	size = (size + mask) & ~mask;

	pstart = paddr;
	start = vaddr;
	end = start + size;

	/* check for overflows */
	if (end - 1 < start)
		return -EINVAL;
	if (pstart + size - 1 < pstart)
		return -EINVAL;

	spin_lock_irqsave(&lock, flags);

	pgtable = get_pgtable();

	for (; vaddr < end; vaddr += SECTION_SIZE, paddr += SECTION_SIZE)
	{
		desc1 = pgtable[vaddr >> 20];

		if (is_unmap) {
			if ((desc1 & 3) != 2) {
				spin_unlock_irqrestore(&lock, flags);
				printk(KERN_WARNING PFX "vaddr %08x is not a section? (%08x)\n",
						vaddr, desc1);
				return -EINVAL;
			}
			v = 0;
		} else {
			if ((desc1 & 3) != 0) {
				printk(KERN_WARNING PFX "vaddr %08x already mapped? (%08x)\n",
						vaddr, desc1);
				retval = -EINVAL;
				break;
			}
			v = (paddr & ~mask) | apcb_bits | 0x12;
		}

		pgtable[vaddr >> 20] = v;
		count++;
	}

	if (retval != 0) {
		/* undo mappings */
		vaddr = start;

		for (; vaddr < end && count > 0; vaddr += SECTION_SIZE, count--)
			pgtable[vaddr >> 20] = 0;
	}

	warm_cop_clean_d();
	warm_drain_wb_inval_tlb();

	spin_unlock_irqrestore(&lock, flags);

	if (retval == 0 && !is_unmap) {
		WARM_INFO("mapped %08x to %08x with %c%c bit(s) (%d section(s))\n",
			start, pstart, apcb_bits & 8 ? 'c' : ' ',
			apcb_bits & 4 ? 'b' : ' ', count);
	}

	return retval;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,11)
static long warm_ioctl(struct file *file, unsigned int cmd, unsigned long __arg)
#else
static int warm_ioctl(struct inode *inode, struct file *file,
			unsigned int cmd, unsigned long __arg)
#endif
{
	void __user *arg = (void __user *) __arg;
	union {
		struct warm_cache_op wcop;
		struct warm_change_cb ccb;
		struct warm_map_op mop;
		unsigned long addr;
	} u;
	long ret;

	switch (cmd) {
	case WARMC_CACHE_OP:
		if (copy_from_user(&u.wcop, arg, sizeof(u.wcop)))
			return -EFAULT;
		if (u.wcop.ops & ~(WOP_D_CLEAN|WOP_D_INVALIDATE|WOP_I_INVALIDATE))
			return -EINVAL;
		if (u.wcop.size == (unsigned long)-1 ||
				(u.wcop.size > MAX_CACHEOP_RANGE && !(u.wcop.ops & WOP_D_INVALIDATE)))
			ret = do_cache_ops_whole(u.wcop.ops);
		else
			ret = do_cache_ops(u.wcop.ops, u.wcop.addr, u.wcop.size);
		break;
	case WARMC_CHANGE_CB:
		if (copy_from_user(&u.ccb, arg, sizeof(u.ccb)))
			return -EFAULT;
		if (u.ccb.cb & ~(WCB_C_BIT|WCB_B_BIT))
			return -EINVAL;
		if (u.ccb.addr == 0 && u.ccb.size == 0)
			ret = do_set_cb_uppermem(u.ccb.cb, u.ccb.is_set);
		else
			ret = do_set_cb_virt(u.ccb.cb, u.ccb.is_set, u.ccb.addr, u.ccb.size);
		break;
	case WARMC_VIRT2PHYS:
		if (copy_from_user(&u.addr, arg, sizeof(u.addr)))
			return -EFAULT;
		ret = do_virt2phys(&u.addr);
		if (copy_to_user(arg, &u.addr, sizeof(u.addr)))
			return -EFAULT;
		break;
	case WARMC_MMAP:
		if (copy_from_user(&u.mop, arg, sizeof(u.mop)))
			return -EFAULT;
		if (u.mop.cb & ~(WCB_C_BIT|WCB_B_BIT))
			return -EINVAL;
		ret = do_map_op(u.mop.virt_addr, u.mop.phys_addr, u.mop.size,
			u.mop.cb, u.mop.is_unmap);
		break;
	default:
		ret = -ENOTTY;
		break;
	}

	return ret;
}

static const char *warm_implementor_name(char code)
{
	switch (code) {
	case 'A':
		return "ARM";
	case 'D':
		return "DEC";
	case 'i':
		return "Intel";
	case 'M':
		return "Motorola - Freescale";
	case 'V':
		return "Marvell";
	}
	return "???";
}

static const char *warm_arch_name(int code)
{
	switch (code) {
	case 1:
		return "4";
	case 2:
		return "4T";
	case 3:
		return "5";
	case 4:
		return "5T";
	case 5:
		return "5TE";
	case 6:
		return "5TEJ";
	case 7:
		return "6";
	}
	return "?";
}

static int warm_cache_size(int code, int m)
{
	int base = 512;
	if (m)
		base = 768;
	return base << code;
}

static int warm_cache_assoc(int code, int m)
{
	int base = 2;
	if (code == 0)
		return m ? 0 : 1;
	if (m)
		base = 3;
	return base << (code - 1); 
}

static int warm_cache_line(int code)
{
	return 8 << code;
}

static int warm_seq_show(struct seq_file *seq, void *offset)
{
	u32 tmp;

	seq_printf(seq, "wARM: " WARM_VER "\n");

	/* ID codes */
	asm ("mrc p15, 0, %0, c0, c0, 0" : "=r"(tmp));
	seq_printf(seq, "ID: %08x\n", tmp);
	if (tmp & 0x80000) {
		/* revised format, not yet documented */
	} else if ((tmp & 0xf000) == 0) {
		/* pre-ARM7 */
		seq_printf(seq, "Architecture: %d\n",
				(tmp & 0xf00) == 0x600 ? 3 : 2);
		seq_printf(seq, "Variant: %d%d0\n", (tmp & 0xf00) >> 8,
				(tmp & 0xf0) >> 4);
	} else {
		seq_printf(seq, "Implementor: %c (%s)\n", tmp >> 24,
				warm_implementor_name(tmp >> 24));
		if ((tmp & 0xf000) == 7) {
			seq_printf(seq, "Architecture: %s\n",
				tmp & 0x800000 ? "4T" : "3");
			seq_printf(seq, "Variant: 0x%x\n", (tmp & 0x7f0000) >> 16);
		} else {
			seq_printf(seq, "Architecture: %s\n",
				warm_arch_name((tmp & 0x0f0000) >> 16));
			seq_printf(seq, "Variant: 0x%x\n", (tmp & 0xf00000) >> 20);
		}
		seq_printf(seq, "Part number: 0x%x\n", (tmp & 0xfff0) >> 4);
	}
	seq_printf(seq, "Revision: 0x%x\n", tmp & 0xf);

	/* cache type */
	asm ("mrc p15, 0, %0, c0, c0, 1" : "=r"(tmp));
	seq_printf(seq, "Cache ctype: 0x%x\n", (tmp & 0x1e000000) >> 25);
	seq_printf(seq, "Cache unified: %s\n", (tmp & 0x01000000) ? "no" : "yes");
	seq_printf(seq, "DCache size: %d\n",
			warm_cache_size((tmp >> (6+12)) & 0xf, (tmp >> (2+12)) & 1));
	seq_printf(seq, "DCache associativity: %d\n",
			warm_cache_assoc((tmp >> (3+12)) & 7, (tmp >> (2+12)) & 1));
	seq_printf(seq, "DCache line size: %d\n",
			warm_cache_line((tmp >> (0+12)) & 3));
	seq_printf(seq, "ICache size: %d\n",
			warm_cache_size((tmp >> 6) & 0xf, (tmp >> 2) & 1));
	seq_printf(seq, "ICache associativity: %d\n",
			warm_cache_assoc((tmp >> 3) & 7, (tmp >> 2) & 1));
	seq_printf(seq, "ICache line size: %d\n",
			warm_cache_line((tmp >> 0) & 3));

	return 0;
}

static int warm_open(struct inode *inode, struct file *file)
{
	return single_open(file, warm_seq_show, NULL);
}

static int warm_close(struct inode *ino, struct file *file)
{
	return single_release(ino, file);
}

static const struct file_operations warm_fops = {
	.owner	= THIS_MODULE,
	.open	= warm_open,
	.read	= seq_read,
	.llseek	= seq_lseek,
	.unlocked_ioctl = warm_ioctl,
	.release = warm_close,
};

static int __init warm_module_init(void)
{
	struct proc_dir_entry *pret;
	u32 cpuid;

	asm ("mrc p15, 0, %0, c0, c0, 0" : "=r"(cpuid));
	if ((cpuid & 0x0ffff0) != EXPECTED_ID) {
		printk(KERN_ERR PFX "module was compiled for different CPU, aborting\n");
		return -1;
	}

	pret = create_proc_entry("warm", S_IWUGO | S_IRUGO, NULL);
	if (!pret) {
		printk(KERN_ERR PFX "can't create proc entry\n");
		return -1;
	}

	pret->owner = THIS_MODULE;
	pret->proc_fops = &warm_fops;

	spin_lock_init(&lock);

	uppermem_start = RAM_PHYS_START + (max_mapnr << PAGE_SHIFT);
	uppermem_end = RAM_PHYS_START + RAM_MAX_SIZE;

	pr_info(PFX WARM_VER " loaded, ");
	if (uppermem_end <= uppermem_start)
		printk("no upper mem");
	else
		printk("upper mem %08x-%08x", uppermem_start, uppermem_end - 1);
	printk("\n");

	/* give time for /proc node to appear */
	mdelay(200);

	return 0;
}

static void __exit warm_module_exit(void)
{
	remove_proc_entry("warm", NULL);

	pr_info(PFX "unloaded.\n");
}

module_init(warm_module_init);
module_exit(warm_module_exit);

module_param(verbose, int, 0644);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("ARM processor services");
MODULE_AUTHOR("Grazvydas Ignotas");
