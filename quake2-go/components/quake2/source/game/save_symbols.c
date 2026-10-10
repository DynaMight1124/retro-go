#include "save_symbols.h"

qboolean Q2_SaveEncodePointer(uintptr_t pointer, qboolean movement, uint32_t *id)
{
    if (!pointer) { *id = 0; return true; }
    for (const q2_save_table_t *table = q2_save_tables; table->symbols; ++table) {
        if (table->movement != movement) continue;
        for (const q2_save_symbol_t *symbol = table->symbols; symbol->id; ++symbol) {
            if (symbol->address == pointer) { *id = symbol->id; return true; }
        }
    }
    return false;
}

qboolean Q2_SaveDecodePointer(uint32_t id, qboolean movement, uintptr_t *pointer)
{
    if (!id) { *pointer = 0; return true; }
    for (const q2_save_table_t *table = q2_save_tables; table->symbols; ++table) {
        if (table->movement != movement) continue;
        for (const q2_save_symbol_t *symbol = table->symbols; symbol->id; ++symbol) {
            if (symbol->id == id) { *pointer = symbol->address; return true; }
        }
    }
    return false;
}
