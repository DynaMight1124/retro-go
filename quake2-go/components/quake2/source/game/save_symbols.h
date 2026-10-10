#ifndef Q2_SAVE_SYMBOLS_H
#define Q2_SAVE_SYMBOLS_H
#include <stdint.h>
#include "q_shared.h"
typedef struct { uint32_t id; uintptr_t address; } q2_save_symbol_t;
typedef struct { const q2_save_symbol_t *symbols; qboolean movement; } q2_save_table_t;
extern const q2_save_table_t q2_save_tables[];
qboolean Q2_SaveEncodePointer(uintptr_t pointer, qboolean movement, uint32_t *id);
qboolean Q2_SaveDecodePointer(uint32_t id, qboolean movement, uintptr_t *pointer);
#endif
