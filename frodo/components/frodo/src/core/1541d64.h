#pragma once

// GCRDisk only needs the canonical D64 geometry constants from the
// DOS-level image-drive implementation. Keeping this header narrow avoids
// pulling the unused virtual filesystem/IEC drive hierarchy into firmware.
constexpr unsigned NUM_SECTORS_35 = 683;
constexpr unsigned NUM_SECTORS_40 = 768;
