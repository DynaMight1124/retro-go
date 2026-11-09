#pragma once
// Exact sum of the existing loaders' 32-byte-aligned Hunk_Alloc calls.
// Returns zero for an invalid/truncated header, range or size overflow.
int Mod_ReservationSize(const void *data, int filelen);
