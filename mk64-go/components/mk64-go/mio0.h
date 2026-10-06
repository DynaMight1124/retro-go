#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

bool mk64_mio0_decode(const uint8_t *source, size_t source_size,
                     uint8_t *output, size_t output_size);
