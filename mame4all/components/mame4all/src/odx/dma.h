#ifdef __cplusplus
extern "C" {
#endif
#ifndef DMA_H
#define DMA_H

#define DMA_BASE	0x13420000
#define DSA0 			(0x00 >> 2)
#define DTA0 			(0x04 >> 2)
#define DTC0 			(0x08 >> 2)
#define DRT0 			(0x0C >> 2)
#define DCS0 			(0x10 >> 2)
#define DCM0 			(0x14 >> 2)
#define DDA0 			(0x18 >> 2)
#define DSD0 			(0x1C >> 2)
#define DSA1 			(0x20 >> 2)
#define DTA1 			(0x24 >> 2)
#define DTC1 			(0x28 >> 2)
#define DRT1 			(0x2C >> 2)
#define DCS1 			(0x30 >> 2)
#define DCM1 			(0x34 >> 2)
#define DDA1 			(0x38 >> 2)
#define DSD1 			(0x3C >> 2)

#define LCD_BASE 	0x13050000
#define LCDDA0 		(0x40 >> 2)
#define LCDSA0 		(0x44 >> 2)
#define LCDFID0 	(0x48 >> 2)
#define LCDCMD0 	(0x4C >> 2)
#define LCDDA1		(0x50 >> 2)
#define LCDSA1		(0x54 >> 2)
#define LCDFID1 	(0x58 >> 2)
#define LCDCMD1 	(0x5C >> 2)

#define PAGE_SIZE	1024

void init_dma();
void close_dma();

#endif
#ifdef __cplusplus
}
#endif
