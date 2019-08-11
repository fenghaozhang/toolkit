#ifndef _SRC_BASE_CRC32C_H
#define _SRC_BASE_CRC32C_H

#include <stdint.h>

uint32_t docrc32c_intel(uint32_t crc, const void *buf, size_t len);

uint32_t crc32c_combine(uint32_t crc1, uint32_t crc2, size_t len2);

uint32_t crc32c_combine_4KB(uint32_t crc1, uint32_t crc2);

#endif  // _SRC_BASE_CRC32C_H
