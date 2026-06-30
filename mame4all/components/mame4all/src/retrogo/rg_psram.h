#ifdef __cplusplus
extern "C" {
#endif

#ifndef RG_PSRAM_H
#define RG_PSRAM_H

#include "osd_cpu.h"
#include <time.h>
#include <string.h>

#include "rg_system.h"

static inline void *rg_alloc_malloc(size_t size) {
    return malloc(size);
}

/* Forward declarations */
struct osd_bitmap;
struct GfxElement;
struct rectangle;
struct tilemap;
struct ipd;

/* Subsystem Init functions */
void rg_psram_init(void);
void check_yiear_psram(void);
void check_mikie_psram(void);
void check_pooyan_psram(void);
void check_btime_psram(void);
void check_kchamp_psram(void);
void check_gberet_psram(void);
void check_shaolins_psram(void);
void check_battlane_psram(void);
void check_ccastles_psram(void);
void check_combatsc_psram(void);
void check_fastlane_psram(void);
void check_hcastle_psram(void);
void check_ironhors_psram(void);
void check_jackal_psram(void);
void check_labyrunr_psram(void);
void check_matmania_psram(void);
void check_taitoic_psram(void);
void check_ym_deltat_psram(void);
void check_pia_psram(void);
void check_atarigen_psram(void);
void check_wardner_psram(void);
void check_input_psram(void);

/* Shared structures */
struct taitoic_psram_struct {
    unsigned char L_TC0100SCN_ctrl[2][16];
    struct tilemap *L_TC0100SCN_tilemap[2][3];
    int L_TC0100SCN_bg_gfx[2];
    unsigned char *L_TC0100SCN_ram[2];
    unsigned char L_TC0220IOC_regs[16];
    int L_TC0110PCR_addr;
    UINT16 *L_TC0110PCR_ram;
    unsigned char L_TC0280GRD_ctrl[16];
    unsigned char *L_TC0280GRD_ram;
    struct tilemap *L_TC0280GRD_tilemap;
    int L_TC0280GRD_gfxnum, L_TC0280GRD_base_color;
};

struct code_info {
	int memory;
	unsigned oscode;
	unsigned type;
};

struct ui_info {
	int memory;
};

struct input_psram_struct {
	struct code_info* code_map;
	unsigned code_mac;
	int record_count;
	clock_t record_last;
	int ui_counter;
	int ui_inputdelay;
	int code_counter;
	int code_keydelay;
	struct ui_info *ui_map;
    int *L_impulsecount;
    int *L_waspressed;
    struct ipd *L_inputport_defaults_backup;
};

struct ym_deltat_psram_struct {
    unsigned char *L_ym_deltat_memory;
};

/* Master PSRAM structure */
struct rg_psram_master {
    struct fm_psram_struct *ptr_fm;
    struct fmopl_psram_struct *ptr_fmopl;
    struct atarigen_psram_data *ptr_atarigen;
    struct taitoic_psram_struct *ptr_taitoic;
    struct slapstic_psram_struct *ptr_slapstic;
    struct eeprom_psram_struct *ptr_eeprom;
    struct mathbox_psram_struct *ptr_mathbox;
    struct avgdvg_vars_struct *ptr_avgdvg;
    struct crtc6845_vars_struct *ptr_crtc6845;
    struct input_psram_struct *ptr_input;
    struct hiscore_psram_struct *ptr_hiscore;
    struct cheat_psram_struct *ptr_cheat;
    struct profile_psram_struct *ptr_profile;
    struct artwork_psram_struct *ptr_artwork;
    struct datafile_psram_struct *ptr_datafile;
    struct polepos_psram_struct *ptr_polepos;
    struct polepos_vid_psram_struct *ptr_polepos_vid;
    struct polepos_snd_psram_struct *ptr_polepos_snd;
    struct ccastles_psram_struct *ptr_ccastles;
    struct wardner_psram_struct *ptr_wardner;
    struct ym_deltat_psram_struct *ptr_ym_deltat;
    struct atarijsa_psram_struct *ptr_atarijsa;
    struct atari_vg_psram_struct *ptr_atari_vg;
    struct rastan_psram_struct *ptr_rastan;
    struct bwidow_psram_struct *ptr_bwidow;
    struct tempest_psram_struct *ptr_tempest;
    struct nemesis_psram_struct *ptr_nemesis;
    struct hyperspt_psram_struct *ptr_hyperspt;
    struct tp84_psram_struct *ptr_tp84;
    struct circusc_psram_struct *ptr_circusc;
    struct thepit_psram_struct *ptr_thepit;
    struct twincobr_psram_struct *ptr_twincobr;
    struct combatsc_psram_struct *ptr_combatsc;
    struct hcastle_psram_struct *ptr_hcastle;
    struct fastlane_psram_struct *ptr_fastlane;
    struct labyrunr_psram_struct *ptr_labyrunr;
    struct ironhors_psram_struct *ptr_ironhors;
    struct jackal_psram_struct *ptr_jackal;
    struct matmania_psram_struct *ptr_matmania;
    struct battlane_psram_struct *ptr_battlane;
    struct kchamp_psram_struct *ptr_kchamp;
    struct gberet_psram_struct *ptr_gberet;
    struct shaolins_psram_struct *ptr_shaolins;
    struct btime_psram_struct *ptr_btime;
    struct pia_psram_struct *ptr_pia;
    struct yiear_psram_struct *ptr_yiear;
    struct mikie_psram_struct *ptr_mikie;
    struct pooyan_psram_struct *ptr_pooyan;
    
    /* System globals */
    int gUnzipQuiet;
    int use_profiler;
    unsigned long (*crc_table)[256];
};

extern struct rg_psram_master *rg_psram;

#endif

#ifdef __cplusplus
}
#endif
