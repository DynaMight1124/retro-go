#include <linux/kernel.h>

void warm_cop_clean_inval_d(void);
void warm_cop_clean_d(void);
void warm_cop_inval_d(void);
void warm_cop_inval_i(void);
void warm_cop_drain_wb(void);

void warm_cop_r_clean_d_inval_di(u32 addr, u32 size);
void warm_cop_r_clean_d_inval_d(u32 addr, u32 size);
void warm_cop_r_clean_d_inval_i(u32 addr, u32 size);
void warm_cop_r_clean_d(u32 addr, u32 size);
void warm_cop_r_inval_di(u32 addr, u32 size);
void warm_cop_r_inval_d(u32 addr, u32 size);
void warm_cop_r_inval_i(u32 addr, u32 size);

void warm_drain_wb_inval_tlb(void);
