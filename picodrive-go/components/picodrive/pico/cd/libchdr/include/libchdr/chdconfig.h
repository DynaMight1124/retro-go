#ifndef __CHDCONFIG_H__
#define __CHDCONFIG_H__

/* Configure CHDR features here */
#define WANT_RAW_DATA_SECTOR    1
#define WANT_SUBCODE            1
/*
 * PicoDrive's pm_file CHD adapter owns the active one-hunk cache.  The
 * libchdr cache/compare buffers are for APIs this read-only port does not
 * call, and otherwise reserve two additional full hunks in PSRAM.
 */
#define VERIFY_BLOCK_CRC        1

#endif
