#ifndef RG_MAME_CORE_H
#define RG_MAME_CORE_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "retrogo/rg_psram.h"
#include "driver.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#include <fcntl.h>
#include <sys/mman.h>
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>

#include "dma.h"

int map_handle = -1;
volatile uint16_t *dma_ptr = NULL;

#define DMA_SIZE ((320 * 480) * 2)

/*
int map_memory(unsigned long addr, unsigned long size)
	{
	map_handle = open("/dev/mem", O_RDWR | O_SYNC);
	if(map_handle < 0)
		{
		printf("failed to open /dev/mem\n");
		return -1;
		}

	dma_pointer = mmap(0, size, PROT_READ | PROT_WRITE, MAP_SHARED, map_handle, addr);
	return 0;
	}

void unmap_memory(unsigned long size)
	{
	if(dma_pointer)
		{
		munmap((void*)dma_pointer, size);
		}
	if(map_handle!=-1)
		{
		close(map_handle);
		map_handle = -1;
		}
	}
*/

void init_dma()
	{
	uint32_t dma_addr = 0;

	map_handle = open("/dev/mem", O_RDWR | O_SYNC);
	if(map_handle < 0)
		{
		printf("failed to open /dev/mem\n");
		return -1;
		}

	volatile unsigned long *dma_pointer = mmap(0, PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, map_handle, LCD_BASE);

	if(dma_pointer)
		{
		dma_addr = dma_pointer[LCDSA0];
		munmap(dma_pointer, PAGE_SIZE);
		}

	if(dma_addr)
		{
		dma_ptr = mmap(0, DMA_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, map_handle, dma_addr);
		}
	}

void close_dma()
	{
	if(dma_ptr)
		{
		munmap(dma_ptr, DMA_SIZE);
		dma_ptr = NULL;
		close(map_handle);
		map_handle = 0;
		}
	}


#ifdef __cplusplus
}
#endif
