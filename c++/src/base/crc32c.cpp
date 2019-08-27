// Copyright 2016 Ferry Toth, Exalon Delft BV, The Netherlands
/*
  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the author be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
    claim that you wrote the original software. If you use this software
    in a product, an acknowledgment in the product documentation would be
    appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
    misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.

  Ferry Toth
  ftoth@exalondelft.nl

  See https://github.com/htot/crc32c for details
*/

/* Use hardware CRC instruction on Intel SSE 4.2 processors.  This computes a
  CRC-32C, *not* the CRC-32 used by Ethernet and zip, gzip, etc. Where efficient
  3 crc32q instructions are used which a single core can execute in parallel.
  This compensates for the latency of a single crc32q instruction. Combining the
  3 CRC-32C bytes is done using the pclmulqdq instruction, which has overhead of
  its own, and makes this code path only efficient for buffer sizes above 216 bytes.
  All code requiring a crc32q instruction is done inside a macro, for which alternative
  code is generated in case of a 32 bit platform.
*/

// GLOBAL_NOLINT

#include <stdint.h>
#include <string.h>
#include <x86intrin.h>

#include "src/common/macros.h"

#define CRCtriplet(crc, buf, offset)                                     \
    crc ## 0 = __builtin_ia32_crc32di(crc ## 0, *(buf ## 0 + offset));   \
    crc ## 1 = __builtin_ia32_crc32di(crc ## 1, *(buf ## 1 + offset));   \
    crc ## 2 = __builtin_ia32_crc32di(crc ## 2, *(buf ## 2 + offset));

#define CRCduplet(crc, buf, offset)                                      \
    crc ## 0 = __builtin_ia32_crc32di(crc ## 0, *(buf ## 0 + offset));   \
    crc ## 1 = __builtin_ia32_crc32di(crc ## 1, *(buf ## 1 + offset));

#define CRCsinglet(crc, buf, offset)                                     \
    crc = __builtin_ia32_crc32di(crc, *(uint64_t*)(buf + offset));

#define CombineCRC()                                                     \
    asm volatile (                                                       \
                     "movdqa       (%3),   %%xmm0\n\t"                   \
                     "movq          %0,    %%xmm1\n\t"                   \
                     "pclmullqlqdq %%xmm0, %%xmm1\n\t"                   \
                     "movq          %2,    %%xmm2\n\t"                   \
                     "pclmullqhqdq %%xmm0, %%xmm2\n\t"                   \
                     "pxor         %%xmm2, %%xmm1\n\t"                   \
                     "movq         %%xmm1, %0"                           \
                     : "=r" (crc0)                                       \
                     : "0" (crc0), "r" (crc1), "r" (K + block_size - 1)  \
                     : "%xmm0", "%xmm1", "%xmm2"                         \
                 );                                                      \
    crc0 = crc0 ^ * ((uint64_t*)next2 - 1);                              \
    crc2 = __builtin_ia32_crc32di(crc2, crc0);                           \
    crc0 = crc2;

// The best prefetch horizon is not same on differenct CPUs, sometimes it
// may have no effect if we set the horizon too small. Here we do prefetch
// twice each time with two different horizon to fit most CPUs.
#define CRCprefetch(buf, offset)                                         \
    __builtin_prefetch(buf ## 0 + offset + kPrefetchLowHorizon, 0, 1);   \
    __builtin_prefetch(buf ## 1 + offset + kPrefetchLowHorizon, 0, 1);   \
    __builtin_prefetch(buf ## 2 + offset + kPrefetchLowHorizon, 0, 1);   \
    __builtin_prefetch(buf ## 0 + offset + kPrefetchHighHorizon, 0, 1);  \
    __builtin_prefetch(buf ## 1 + offset + kPrefetchHighHorizon, 0, 1);  \
    __builtin_prefetch(buf ## 2 + offset + kPrefetchHighHorizon, 0, 1);

static const uint32_t kPrefetchLowHorizon = 8;  // 8 * 8 = 64 bytes
static const uint32_t kPrefetchHighHorizon = 24;  // 24 * 8 = 192 bytes

static __v2di K[] = {
        {0x14cd00bd6, 0x105ec76f0}, {0x0ba4fc28e, 0x14cd00bd6}, {0x1d82c63da, 0x0f20c0dfe}, {0x09e4addf8, 0x0ba4fc28e},
        {0x039d3b296, 0x1384aa63a}, {0x102f9b8a2, 0x1d82c63da}, {0x14237f5e6, 0x01c291d04}, {0x00d3b6092, 0x09e4addf8},
        {0x0c96cfdc0, 0x0740eef02}, {0x18266e456, 0x039d3b296}, {0x0daece73e, 0x0083a6eec}, {0x0ab7aff2a, 0x102f9b8a2},
        {0x1248ea574, 0x1c1733996}, {0x083348832, 0x14237f5e6}, {0x12c743124, 0x02ad91c30}, {0x0b9e02b86, 0x00d3b6092},
        {0x018b33a4e, 0x06992cea2}, {0x1b331e26a, 0x0c96cfdc0}, {0x17d35ba46, 0x07e908048}, {0x1bf2e8b8a, 0x18266e456},
        {0x1a3e0968a, 0x11ed1f9d8}, {0x0ce7f39f4, 0x0daece73e}, {0x061d82e56, 0x0f1d0f55e}, {0x0d270f1a2, 0x0ab7aff2a},
        {0x1c3f5f66c, 0x0a87ab8a8}, {0x12ed0daac, 0x1248ea574}, {0x065863b64, 0x08462d800}, {0x11eef4f8e, 0x083348832},
        {0x1ee54f54c, 0x071d111a8}, {0x0b3e32c28, 0x12c743124}, {0x0064f7f26, 0x0ffd852c6}, {0x0dd7e3b0c, 0x0b9e02b86},
        {0x0f285651c, 0x0dcb17aa4}, {0x010746f3c, 0x018b33a4e}, {0x1c24afea4, 0x0f37c5aee}, {0x0271d9844, 0x1b331e26a},
        {0x08e766a0c, 0x06051d5a2}, {0x093a5f730, 0x17d35ba46}, {0x06cb08e5c, 0x11d5ca20e}, {0x06b749fb2, 0x1bf2e8b8a},
        {0x1167f94f2, 0x021f3d99c}, {0x0cec3662e, 0x1a3e0968a}, {0x19329634a, 0x08f158014}, {0x0e6fc4e6a, 0x0ce7f39f4},
        {0x08227bb8a, 0x1a5e82106}, {0x0b0cd4768, 0x061d82e56}, {0x13c2b89c4, 0x188815ab2}, {0x0d7a4825c, 0x0d270f1a2},
        {0x10f5ff2ba, 0x105405f3e}, {0x00167d312, 0x1c3f5f66c}, {0x0f6076544, 0x0e9adf796}, {0x026f6a60a, 0x12ed0daac},
        {0x1a2adb74e, 0x096638b34}, {0x19d34af3a, 0x065863b64}, {0x049c3cc9c, 0x1e50585a0}, {0x068bce87a, 0x11eef4f8e},
        {0x1524fa6c6, 0x19f1c69dc}, {0x16cba8aca, 0x1ee54f54c}, {0x042d98888, 0x12913343e}, {0x1329d9f7e, 0x0b3e32c28},
        {0x1b1c69528, 0x088f25a3a}, {0x02178513a, 0x0064f7f26}, {0x0e0ac139e, 0x04e36f0b0}, {0x0170076fa, 0x0dd7e3b0c},
        {0x141a1a2e2, 0x0bd6f81f8}, {0x16ad828b4, 0x0f285651c}, {0x041d17b64, 0x19425cbba}, {0x1fae1cc66, 0x010746f3c},
        {0x1a75b4b00, 0x18db37e8a}, {0x0f872e54c, 0x1c24afea4}, {0x01e41e9fc, 0x04c144932}, {0x086d8e4d2, 0x0271d9844},
        {0x160f7af7a, 0x052148f02}, {0x05bb8f1bc, 0x08e766a0c}, {0x0a90fd27a, 0x0a3c6f37a}, {0x0b3af077a, 0x093a5f730},
        {0x04984d782, 0x1d22c238e}, {0x0ca6ef3ac, 0x06cb08e5c}, {0x0234e0b26, 0x063ded06a}, {0x1d88abd4a, 0x06b749fb2},
        {0x04597456a, 0x04d56973c}, {0x0e9e28eb4, 0x1167f94f2}, {0x07b3ff57a, 0x19385bf2e}, {0x0c9c8b782, 0x0cec3662e},
        {0x13a9cba9e, 0x0e417f38a}, {0x093e106a4, 0x19329634a}, {0x167001a9c, 0x14e727980}, {0x1ddffc5d4, 0x0e6fc4e6a},
        {0x00df04680, 0x0d104b8fc}, {0x02342001e, 0x08227bb8a}, {0x00a2a8d7e, 0x05b397730}, {0x168763fa6, 0x0b0cd4768},
        {0x1ed5a407a, 0x0e78eb416}, {0x0d2c3ed1a, 0x13c2b89c4}, {0x0995a5724, 0x1641378f0}, {0x19b1afbc4, 0x0d7a4825c},
        {0x109ffedc0, 0x08d96551c}, {0x0f2271e60, 0x10f5ff2ba}, {0x00b0bf8ca, 0x00bf80dd2}, {0x123888b7a, 0x00167d312},
        {0x1e888f7dc, 0x18dcddd1c}, {0x002ee03b2, 0x0f6076544}, {0x183e8d8fe, 0x06a45d2b2}, {0x133d7a042, 0x026f6a60a},
        {0x116b0f50c, 0x1dd3e10e8}, {0x05fabe670, 0x1a2adb74e}, {0x130004488, 0x0de87806c}, {0x000bcf5f6, 0x19d34af3a},
        {0x18f0c7078, 0x014338754}, {0x017f27698, 0x049c3cc9c}, {0x058ca5f00, 0x15e3e77ee}, {0x1af900c24, 0x068bce87a},
        {0x0b5cfca28, 0x0dd07448e}, {0x0ded288f8, 0x1524fa6c6}, {0x059f229bc, 0x1d8048348}, {0x06d390dec, 0x16cba8aca},
        {0x037170390, 0x0a3e3e02c}, {0x06353c1cc, 0x042d98888}, {0x0c4584f5c, 0x0d73c7bea}, {0x1f16a3418, 0x1329d9f7e},
        {0x0531377e2, 0x185137662}, {0x1d8d9ca7c, 0x1b1c69528}, {0x0b25b29f2, 0x18a08b5bc}, {0x19fb2a8b0, 0x02178513a},
        {0x1a08fe6ac, 0x1da758ae0}, {0x045cddf4e, 0x0e0ac139e}, {0x1a91647f2, 0x169cf9eb0}, {0x1a0f717c4, 0x0170076fa}
};

uint32_t docrc32c_intel(uint32_t crc, const void *buf, size_t len)
{
    const uint8_t *next = (const uint8_t *)buf;
    uint64_t count;
    uint64_t crc0, crc1, crc2;
    crc0 = crc;

    if (len >= 8)
    {
        // if len > 216 then align and use triplets
        if (len > 216)
        {
            uint64_t align = ((uint64_t)next) % 8;           // byte to boundary
            if (__builtin_expect(align != 0, 0))
            {
                align = 8 - align;
                len -= align;
                uint32_t crc32bit = crc0;
                if (align & 0x04)
                {
                    crc32bit = __builtin_ia32_crc32si(crc32bit, *(uint32_t*)next);
                    next += sizeof(uint32_t);
                };
                if (align & 0x02)
                {
                    crc32bit = __builtin_ia32_crc32hi(crc32bit, *(uint16_t*)next);
                    next += sizeof(uint16_t);
                };
                if (align & 0x01)
                {
                    crc32bit = __builtin_ia32_crc32qi(crc32bit, *(next));
                    next++;
                };
                crc0 = crc32bit;
            }

            // use Duff's device, a for() loop inside a switch() statement. This is Legal
            // needs to execute at least once, round len down to nearast triplet multiple
            count = len / 24;       // number of triplets
            len %= 24;              // bytes remaining
            uint64_t n = count / 128;		// #blocks = first block + full blocks
            uint64_t block_size = count % 128;
            if (block_size == 0)
            {
                block_size = 128;
            }
            else
            {
                n++;
            };
            const uint64_t *next0 = (uint64_t*)next + block_size; // points to the first byte of the next block
            const uint64_t *next1 = next0 + block_size;
            const uint64_t *next2 = next1 + block_size;

            crc1 = crc2 = 0;
            switch (block_size)
            {
                case 128:
                    do
                    {
                        CRCprefetch(next, -128);
                        CRCtriplet(crc, next, -128);	// jumps here for a full block of len 128
                        case 127:
                        CRCtriplet(crc, next, -127);	// jumps here or below for the first block smaller
                        case 126:
                        CRCtriplet(crc, next, -126);	// than 128
                        case 125:
                        CRCtriplet(crc, next, -125);
                        case 124:
                        CRCtriplet(crc, next, -124);
                        case 123:
                        CRCtriplet(crc, next, -123);
                        case 122:
                        CRCtriplet(crc, next, -122);
                        case 121:
                        CRCtriplet(crc, next, -121);
                        case 120:
                        CRCprefetch(next, -120);
                        CRCtriplet(crc, next, -120);
                        case 119:
                        CRCtriplet(crc, next, -119);
                        case 118:
                        CRCtriplet(crc, next, -118);
                        case 117:
                        CRCtriplet(crc, next, -117);
                        case 116:
                        CRCtriplet(crc, next, -116);
                        case 115:
                        CRCtriplet(crc, next, -115);
                        case 114:
                        CRCtriplet(crc, next, -114);
                        case 113:
                        CRCtriplet(crc, next, -113);
                        case 112:
                        CRCprefetch(next, -112);
                        CRCtriplet(crc, next, -112);
                        case 111:
                        CRCtriplet(crc, next, -111);
                        case 110:
                        CRCtriplet(crc, next, -110);
                        case 109:
                        CRCtriplet(crc, next, -109);
                        case 108:
                        CRCtriplet(crc, next, -108);
                        case 107:
                        CRCtriplet(crc, next, -107);
                        case 106:
                        CRCtriplet(crc, next, -106);
                        case 105:
                        CRCtriplet(crc, next, -105);
                        case 104:
                        CRCprefetch(next, -104);
                        CRCtriplet(crc, next, -104);
                        case 103:
                        CRCtriplet(crc, next, -103);
                        case 102:
                        CRCtriplet(crc, next, -102);
                        case 101:
                        CRCtriplet(crc, next, -101);
                        case 100:
                        CRCtriplet(crc, next, -100);
                        case 99:
                        CRCtriplet(crc, next, -99);
                        case 98:
                        CRCtriplet(crc, next, -98);
                        case 97:
                        CRCtriplet(crc, next, -97);
                        case 96:
                        CRCprefetch(next, -96);
                        CRCtriplet(crc, next, -96);
                        case 95:
                        CRCtriplet(crc, next, -95);
                        case 94:
                        CRCtriplet(crc, next, -94);
                        case 93:
                        CRCtriplet(crc, next, -93);
                        case 92:
                        CRCtriplet(crc, next, -92);
                        case 91:
                        CRCtriplet(crc, next, -91);
                        case 90:
                        CRCtriplet(crc, next, -90);
                        case 89:
                        CRCtriplet(crc, next, -89);
                        case 88:
                        CRCprefetch(next, -88);
                        CRCtriplet(crc, next, -88);
                        case 87:
                        CRCtriplet(crc, next, -87);
                        case 86:
                        CRCtriplet(crc, next, -86);
                        case 85:
                        CRCtriplet(crc, next, -85);
                        case 84:
                        CRCtriplet(crc, next, -84);
                        case 83:
                        CRCtriplet(crc, next, -83);
                        case 82:
                        CRCtriplet(crc, next, -82);
                        case 81:
                        CRCtriplet(crc, next, -81);
                        case 80:
                        CRCprefetch(next, -80);
                        CRCtriplet(crc, next, -80);
                        case 79:
                        CRCtriplet(crc, next, -79);
                        case 78:
                        CRCtriplet(crc, next, -78);
                        case 77:
                        CRCtriplet(crc, next, -77);
                        case 76:
                        CRCtriplet(crc, next, -76);
                        case 75:
                        CRCtriplet(crc, next, -75);
                        case 74:
                        CRCtriplet(crc, next, -74);
                        case 73:
                        CRCtriplet(crc, next, -73);
                        case 72:
                        CRCprefetch(next, -72);
                        CRCtriplet(crc, next, -72);
                        case 71:
                        CRCtriplet(crc, next, -71);
                        case 70:
                        CRCtriplet(crc, next, -70);
                        case 69:
                        CRCtriplet(crc, next, -69);
                        case 68:
                        CRCtriplet(crc, next, -68);
                        case 67:
                        CRCtriplet(crc, next, -67);
                        case 66:
                        CRCtriplet(crc, next, -66);
                        case 65:
                        CRCtriplet(crc, next, -65);
                        case 64:
                        CRCprefetch(next, -64);
                        CRCtriplet(crc, next, -64);
                        case 63:
                        CRCtriplet(crc, next, -63);
                        case 62:
                        CRCtriplet(crc, next, -62);
                        case 61:
                        CRCtriplet(crc, next, -61);
                        case 60:
                        CRCtriplet(crc, next, -60);
                        case 59:
                        CRCtriplet(crc, next, -59);
                        case 58:
                        CRCtriplet(crc, next, -58);
                        case 57:
                        CRCtriplet(crc, next, -57);
                        case 56:
                        CRCprefetch(next, -56);
                        CRCtriplet(crc, next, -56);
                        case 55:
                        CRCtriplet(crc, next, -55);
                        case 54:
                        CRCtriplet(crc, next, -54);
                        case 53:
                        CRCtriplet(crc, next, -53);
                        case 52:
                        CRCtriplet(crc, next, -52);
                        case 51:
                        CRCtriplet(crc, next, -51);
                        case 50:
                        CRCtriplet(crc, next, -50);
                        case 49:
                        CRCtriplet(crc, next, -49);
                        case 48:
                        CRCprefetch(next, -48);
                        CRCtriplet(crc, next, -48);
                        case 47:
                        CRCtriplet(crc, next, -47);
                        case 46:
                        CRCtriplet(crc, next, -46);
                        case 45:
                        CRCtriplet(crc, next, -45);
                        case 44:
                        CRCtriplet(crc, next, -44);
                        case 43:
                        CRCtriplet(crc, next, -43);
                        case 42:
                        CRCtriplet(crc, next, -42);
                        case 41:
                        CRCtriplet(crc, next, -41);
                        case 40:
                        CRCprefetch(next, -40);
                        CRCtriplet(crc, next, -40);
                        case 39:
                        CRCtriplet(crc, next, -39);
                        case 38:
                        CRCtriplet(crc, next, -38);
                        case 37:
                        CRCtriplet(crc, next, -37);
                        case 36:
                        CRCtriplet(crc, next, -36);
                        case 35:
                        CRCtriplet(crc, next, -35);
                        case 34:
                        CRCtriplet(crc, next, -34);
                        case 33:
                        CRCtriplet(crc, next, -33);
                        case 32:
                        CRCprefetch(next, -32);
                        CRCtriplet(crc, next, -32);
                        case 31:
                        CRCtriplet(crc, next, -31);
                        case 30:
                        CRCtriplet(crc, next, -30);
                        case 29:
                        CRCtriplet(crc, next, -29);
                        case 28:
                        CRCtriplet(crc, next, -28);
                        case 27:
                        CRCtriplet(crc, next, -27);
                        case 26:
                        CRCtriplet(crc, next, -26);
                        case 25:
                        CRCtriplet(crc, next, -25);
                        case 24:
                        CRCprefetch(next, -24);
                        CRCtriplet(crc, next, -24);
                        case 23:
                        CRCtriplet(crc, next, -23);
                        case 22:
                        CRCtriplet(crc, next, -22);
                        case 21:
                        CRCtriplet(crc, next, -21);
                        case 20:
                        CRCtriplet(crc, next, -20);
                        case 19:
                        CRCtriplet(crc, next, -19);
                        case 18:
                        CRCtriplet(crc, next, -18);
                        case 17:
                        CRCtriplet(crc, next, -17);
                        case 16:
                        CRCprefetch(next, -16);
                        CRCtriplet(crc, next, -16);
                        case 15:
                        CRCtriplet(crc, next, -15);
                        case 14:
                        CRCtriplet(crc, next, -14);
                        case 13:
                        CRCtriplet(crc, next, -13);
                        case 12:
                        CRCtriplet(crc, next, -12);
                        case 11:
                        CRCtriplet(crc, next, -11);
                        case 10:
                        CRCtriplet(crc, next, -10);
                        case 9:
                        CRCtriplet(crc, next, -9);
                        case 8:
                        CRCprefetch(next, -8);
                        CRCtriplet(crc, next, -8);
                        case 7:
                        CRCtriplet(crc, next, -7);
                        case 6:
                        CRCtriplet(crc, next, -6);
                        case 5:
                        CRCtriplet(crc, next, -5);
                        case 4:
                        CRCtriplet(crc, next, -4);
                        case 3:
                        CRCtriplet(crc, next, -3);
                        case 2:
                        CRCtriplet(crc, next, -2);
                        case 1:
                        CRCduplet ( crc, next, -1);		        // the final triplet is actually only 2
                        CombineCRC();
                        if (--n > 0)
                        {
                            crc1 = crc2 = 0;
                            block_size = 128;
                            next0 = next2 + 128;			// points to the first byte of the next block
                            next1 = next0 + 128;			// from here on all blocks are 128 long
                            next2 = next1 + 128;
                        };
                        case 0:
                        ;
                    } while ( n > 0);
            };
            next = (const uint8_t*) next2;
        };
        unsigned count = len / 8;                                               // 216 of less bytes is 27 or less singlets
        len %= 8;
        next += ( count * 8);
        switch ( count ) {
            case 27:
                CRCsinglet(crc0, next, -27 * 8);
            case 26:
                CRCsinglet(crc0, next, -26 * 8);
            case 25:
                CRCsinglet(crc0, next, -25 * 8);
            case 24:
                CRCsinglet(crc0, next, -24 * 8);
            case 23:
                CRCsinglet(crc0, next, -23 * 8);
            case 22:
                CRCsinglet(crc0, next, -22 * 8);
            case 21:
                CRCsinglet(crc0, next, -21 * 8);
            case 20:
                CRCsinglet(crc0, next, -20 * 8);
            case 19:
                CRCsinglet(crc0, next, -19 * 8);
            case 18:
                CRCsinglet(crc0, next, -18 * 8);
            case 17:
                CRCsinglet(crc0, next, -17 * 8);
            case 16:
                CRCsinglet(crc0, next, -16 * 8);
            case 15:
                CRCsinglet(crc0, next, -15 * 8);
            case 14:
                CRCsinglet(crc0, next, -14 * 8);
            case 13:
                CRCsinglet(crc0, next, -13 * 8);
            case 12:
                CRCsinglet(crc0, next, -12 * 8);
            case 11:
                CRCsinglet(crc0, next, -11 * 8);
            case 10:
                CRCsinglet(crc0, next, -10 * 8);
            case 9:
                CRCsinglet(crc0, next, -9 * 8);
            case 8:
                CRCsinglet(crc0, next, -8 * 8);
            case 7:
                CRCsinglet(crc0, next, -7 * 8);
            case 6:
                CRCsinglet(crc0, next, -6 * 8);
            case 5:
                CRCsinglet(crc0, next, -5 * 8);
            case 4:
                CRCsinglet(crc0, next, -4 * 8);
            case 3:
                CRCsinglet(crc0, next, -3 * 8);
            case 2:
                CRCsinglet(crc0, next, -2 * 8);
            case 1:
                CRCsinglet(crc0, next, -1 * 8);
            case 0:
                ;
        };

    }
    uint32_t crc32bit = crc0;
    if (__builtin_expect(len != 0, 0))
    {
        // less than 8 bytes remain
        /* compute the crc for up to seven trailing bytes */
        if (len & 0x04)
        {
            crc32bit = __builtin_ia32_crc32si (crc32bit, *(uint32_t*)next);
            next += sizeof(uint32_t);
        };
        if (len & 0x02)
        {
            crc32bit = __builtin_ia32_crc32hi (crc32bit, *(uint16_t*)next);
            next += sizeof(uint16_t);
        };

        if (len & 0x01)
        {
            crc32bit = __builtin_ia32_crc32qi (crc32bit, *(next));
        };
    }
    return crc32bit;
};

#define GF2_DIM 32      /* dimension of GF(2) vectors (length of CRC) */

// These two inital arrays are calculated from PrecomputeInitEvenOdd().
static const uint32_t init_odd[GF2_DIM] = {
    0x105ec76f, 0x20bd8ede, 0x417b1dbc, 0x82f63b78,
    0x00000001, 0x00000002, 0x00000004, 0x00000008,
    0x00000010, 0x00000020, 0x00000040, 0x00000080,
    0x00000100, 0x00000200, 0x00000400, 0x00000800,
    0x00001000, 0x00002000, 0x00004000, 0x00008000,
    0x00010000, 0x00020000, 0x00040000, 0x00080000,
    0x00100000, 0x00200000, 0x00400000, 0x00800000,
    0x01000000, 0x02000000, 0x04000000, 0x08000000
};

static const uint32_t init_even[GF2_DIM] = {
    0x417b1dbc, 0x82f63b78, 0x00000001, 0x00000002,
    0x00000004, 0x00000008, 0x00000010, 0x00000020,
    0x00000040, 0x00000080, 0x00000100, 0x00000200,
    0x00000400, 0x00000800, 0x00001000, 0x00002000,
    0x00004000, 0x00008000, 0x00010000, 0x00020000,
    0x00040000, 0x00080000, 0x00100000, 0x00200000,
    0x00400000, 0x00800000, 0x01000000, 0x02000000,
    0x04000000, 0x08000000, 0x10000000, 0x20000000
};

// These two inital arrays are calculated from PrecomputeInitEvenOdd4KB().
// init_odd_4KB array is not used.
static const uint32_t init_odd_4KB[GF2_DIM] = {
    0xf7506984, 0xeb4ca5f9, 0xd3753d03, 0xa3060cf7,
    0x43e06f1f, 0x87c0de3e, 0x0a6dca8d, 0x14db951a,
    0x29b72a34, 0x536e5468, 0xa6dca8d0, 0x48552751,
    0x90aa4ea2, 0x24b8ebb5, 0x4971d76a, 0x92e3aed4,
    0x202b2b59, 0x405656b2, 0x80acad64, 0x04b52c39,
    0x096a5872, 0x12d4b0e4, 0x25a961c8, 0x4b52c390,
    0x96a58720, 0x28a778b1, 0x514ef162, 0xa29de2c4,
    0x40d7b379, 0x81af66f2, 0x06b2bb15, 0x0d65762a
};

static const uint32_t init_even_4KB[GF2_DIM] = {
    0xc2a5b65e, 0x80a71a4d, 0x04a2426b, 0x094484d6,
    0x128909ac, 0x25121358, 0x4a2426b0, 0x94484d60,
    0x2d7cec31, 0x5af9d862, 0xb5f3b0c4, 0x6e0b1779,
    0xdc162ef2, 0xbdc02b15, 0x7e6c20db, 0xfcd841b6,
    0xfc5cf59d, 0xfd559dcb, 0xff474d67, 0xfb62ec3f,
    0xf329ae8f, 0xe3bf2bef, 0xc292212f, 0x80c834af,
    0x047c1faf, 0x08f83f5e, 0x11f07ebc, 0x23e0fd78,
    0x47c1faf0, 0x8f83f5e0, 0x1aeb9d31, 0x35d73a62
};

// The 8 arrays below are calculated from PrecomputeByteTable4KB().
static const uint32_t byte_table_0_4KB[16] = {
    0x00000000, 0xc2a5b65e, 0x80a71a4d, 0x4202ac13,
    0x04a2426b, 0xc607f435, 0x84055826, 0x46a0ee78,
    0x094484d6, 0xcbe13288, 0x89e39e9b, 0x4b4628c5,
    0x0de6c6bd, 0xcf4370e3, 0x8d41dcf0, 0x4fe46aae,
};

static const uint32_t byte_table_1_4KB[16] = {
    0x00000000, 0x128909ac, 0x25121358, 0x379b1af4,
    0x4a2426b0, 0x58ad2f1c, 0x6f3635e8, 0x7dbf3c44,
    0x94484d60, 0x86c144cc, 0xb15a5e38, 0xa3d35794,
    0xde6c6bd0, 0xcce5627c, 0xfb7e7888, 0xe9f77124,
};

static const uint32_t byte_table_2_4KB[16] = {
    0x00000000, 0x2d7cec31, 0x5af9d862, 0x77853453,
    0xb5f3b0c4, 0x988f5cf5, 0xef0a68a6, 0xc2768497,
    0x6e0b1779, 0x4377fb48, 0x34f2cf1b, 0x198e232a,
    0xdbf8a7bd, 0xf6844b8c, 0x81017fdf, 0xac7d93ee,
};

static const uint32_t byte_table_3_4KB[16] = {
    0x00000000, 0xdc162ef2, 0xbdc02b15, 0x61d605e7,
    0x7e6c20db, 0xa27a0e29, 0xc3ac0bce, 0x1fba253c,
    0xfcd841b6, 0x20ce6f44, 0x41186aa3, 0x9d0e4451,
    0x82b4616d, 0x5ea24f9f, 0x3f744a78, 0xe362648a,
};

static const uint32_t byte_table_4_4KB[16] = {
    0x00000000, 0xfc5cf59d, 0xfd559dcb, 0x01096856,
    0xff474d67, 0x031bb8fa, 0x0212d0ac, 0xfe4e2531,
    0xfb62ec3f, 0x073e19a2, 0x063771f4, 0xfa6b8469,
    0x0425a158, 0xf87954c5, 0xf9703c93, 0x052cc90e,
};

static const uint32_t byte_table_5_4KB[16] = {
    0x00000000, 0xf329ae8f, 0xe3bf2bef, 0x10968560,
    0xc292212f, 0x31bb8fa0, 0x212d0ac0, 0xd204a44f,
    0x80c834af, 0x73e19a20, 0x63771f40, 0x905eb1cf,
    0x425a1580, 0xb173bb0f, 0xa1e53e6f, 0x52cc90e0,
};

static const uint32_t byte_table_6_4KB[16] = {
    0x00000000, 0x047c1faf, 0x08f83f5e, 0x0c8420f1,
    0x11f07ebc, 0x158c6113, 0x190841e2, 0x1d745e4d,
    0x23e0fd78, 0x279ce2d7, 0x2b18c226, 0x2f64dd89,
    0x321083c4, 0x366c9c6b, 0x3ae8bc9a, 0x3e94a335,
};

static const uint32_t byte_table_7_4KB[16] = {
    0x00000000, 0x47c1faf0, 0x8f83f5e0, 0xc8420f10,
    0x1aeb9d31, 0x5d2a67c1, 0x956868d1, 0xd2a99221,
    0x35d73a62, 0x7216c092, 0xba54cf82, 0xfd953572,
    0x2f3ca753, 0x68fd5da3, 0xa0bf52b3, 0xe77ea843,
};

static uint32_t gf2_matrix_times(const uint32_t* mat, uint32_t vec)
{
    uint32_t sum = 0;

    while (vec)
    {
        if (vec & 1)
            sum ^= *mat;
        vec >>= 1;
        mat++;
    }
    return sum;
}

static void gf2_matrix_square(uint32_t* square, uint32_t* mat)
{
    for (int n = 0; n < GF2_DIM; n++)
        square[n] = gf2_matrix_times(mat, mat[n]);
}

static void PrecomputeInitEvenOdd()
{
    uint32_t even[GF2_DIM];    /* even-power-of-two zeros operator */
    uint32_t odd[GF2_DIM];     /* odd-power-of-two zeros operator */

    /* put operator for one zero bit in odd */
    odd[0] = 0x82f63b78;          /* reversed polynomial for CRC32-C */

    uint32_t row = 1;
    for (int n = 1; n < GF2_DIM; n++)
    {
        odd[n] = row;
        row <<= 1;
    }

    /* put operator for two zero bits in even */
    gf2_matrix_square(even, odd);

    /* put operator for four zero bits in odd */
    gf2_matrix_square(odd, even);
}

static void PrecomputeInitEvenOdd4KB()
{
    uint32_t even[GF2_DIM];    /* even-power-of-two zeros operator */
    uint32_t odd[GF2_DIM];     /* odd-power-of-two zeros operator */

    memcpy(even, init_even, GF2_DIM * sizeof(uint32_t));
    memcpy(odd, init_odd, GF2_DIM * sizeof(uint32_t));

    gf2_matrix_square(even, odd);   // len2 4K  => 2K
    gf2_matrix_square(odd, even);   // len2 2K  => 1K
    gf2_matrix_square(even, odd);   // len2 1K  => 512
    gf2_matrix_square(odd, even);   // len2 512 => 256
    gf2_matrix_square(even, odd);   // len2 256 => 128
    gf2_matrix_square(odd, even);   // len2 128 => 64
    gf2_matrix_square(even, odd);   // len2 64  => 32
    gf2_matrix_square(odd, even);   // len2 32  => 16
    gf2_matrix_square(even, odd);   // len2 16  => 8
    gf2_matrix_square(odd, even);   // len2 8   => 4
    gf2_matrix_square(even, odd);   // len2 4   => 2
    gf2_matrix_square(odd, even);   // len2 2   => 1
    gf2_matrix_square(even, odd);   // len2 1
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
static void PrecomputeByteTable4KB()
{
    for (int i = 0; i < 8; ++i)
    {
        for (int j = 0; j <= 0xf; ++j)
        {
            uint32_t sum = 0;
            uint32_t bit_pos = i * 4;
            int t = j;
            while (t > 0)
            {
                if (t & 1)
                {
                    sum ^= init_even_4KB[bit_pos];
                }
                t >>= 1;
                ++bit_pos;
            }
        }
    }
}

// Special case where len2 == 4096.
uint32_t crc32c_combine_4KB(uint32_t crc1, uint32_t crc2)
{
    uint32_t sum = 0;
    sum ^= byte_table_0_4KB[(crc1 & 0x0000000f) >> 0];
    sum ^= byte_table_1_4KB[(crc1 & 0x000000f0) >> 4];
    sum ^= byte_table_2_4KB[(crc1 & 0x00000f00) >> 8];
    sum ^= byte_table_3_4KB[(crc1 & 0x0000f000) >> 12];
    sum ^= byte_table_4_4KB[(crc1 & 0x000f0000) >> 16];
    sum ^= byte_table_5_4KB[(crc1 & 0x00f00000) >> 20];
    sum ^= byte_table_6_4KB[(crc1 & 0x0f000000) >> 24];
    sum ^= byte_table_7_4KB[(crc1 & 0xf0000000) >> 28];
    return sum ^ crc2;
};
#pragma GCC diagnostic pop

uint32_t crc32c_combine_slow_for_test(
        uint32_t crc1, uint32_t crc2, size_t len2)
{
    if (len2 == 4 * 1024)
    {
        return crc32c_combine_4KB(crc1, crc2);
    }

    uint32_t even[GF2_DIM];    /* even-power-of-two zeros operator */
    uint32_t odd[GF2_DIM];     /* odd-power-of-two zeros operator */

    /* degenerate case (also disallow negative lengths) */
    if (len2 <= 0) return crc1;

    memcpy(even, init_even, GF2_DIM * sizeof(uint32_t));
    memcpy(odd, init_odd, GF2_DIM * sizeof(uint32_t));

    /* apply len2 zeros to crc1 (first square will put the operator for one
       zero byte, eight zero bits, in even) */
    do
    {
        /* apply zeros operator for this bit of len2 */
        gf2_matrix_square(even, odd);
        if (len2 & 1)
            crc1 = gf2_matrix_times(even, crc1);
        len2 >>= 1;

        /* if no more bits set, then done */
        if (len2 == 0)
            break;

        /* another iteration of the loop with odd and even swapped */
        gf2_matrix_square(odd, even);
        if (len2 & 1)
            crc1 = gf2_matrix_times(odd, crc1);
        len2 >>= 1;

        /* if no more bits set, then done */
    } while (len2 != 0);

    /* return combined crc */
    crc1 ^= crc2;
    return crc1;
};

// https://github.com/facebook/folly
// https://github.com/facebook/folly/blob/master/folly/hash/detail/Crc32CombineDetail.cpp

// Reduction taken from
// https://www.nicst.de/crc.pdf
//
// This is an intrinsics-based implementation of listing 3.

static const uint32_t crc32c_m = 0x82f63b78;

static const uint32_t crc32c_powers[] = {
    0x82f63b78, 0x6ea2d55c, 0x18b8ea18, 0x510ac59a, 0xb82be955, 0xb8fdb1e7,
    0x88e56f72, 0x74c360a4, 0xe4172b16, 0x0d65762a, 0x35d73a62, 0x28461564,
    0xbf455269, 0xe2ea32dc, 0xfe7740e6, 0xf946610b, 0x3c204f8f, 0x538586e3,
    0x59726915, 0x734d5309, 0xbc1ac763, 0x7d0722cc, 0xd289cabe, 0xe94ca9bc,
    0x05b74f3f, 0xa51e1f42, 0x40000000, 0x20000000, 0x08000000, 0x00800000,
    0x00008000, 0x82f63b78, 0x6ea2d55c, 0x18b8ea18, 0x510ac59a, 0xb82be955,
    0xb8fdb1e7, 0x88e56f72, 0x74c360a4, 0xe4172b16, 0x0d65762a, 0x35d73a62,
    0x28461564, 0xbf455269, 0xe2ea32dc, 0xfe7740e6, 0xf946610b, 0x3c204f8f,
    0x538586e3, 0x59726915, 0x734d5309, 0xbc1ac763, 0x7d0722cc, 0xd289cabe,
    0xe94ca9bc, 0x05b74f3f, 0xa51e1f42, 0x40000000, 0x20000000, 0x08000000,
    0x00800000, 0x00008000
};

static inline uint32_t gf_multiply_crc32c_hw(uint64_t crc1,
        uint64_t crc2, uint32_t m)
{
    __m128i crc1_xmm = _mm_set_epi64x(0, crc1);
    __m128i crc2_xmm = _mm_set_epi64x(0, crc2);
    __m128i count    = _mm_set_epi64x(0, 1);
    __m128i res0     = _mm_clmulepi64_si128(crc2_xmm, crc1_xmm, 0x00);
    __m128i res1     = _mm_sll_epi64(res0, count);

    // Use hardware crc32c to do reduction from 64 -> 32 bytes
    uint64_t res2 = _mm_cvtsi128_si64(res1);
    uint32_t res3 = _mm_crc32_u32(0, res2);
    uint32_t res4 = _mm_extract_epi32(res1, 1);
    return res3 ^ res4;
}

template <typename F>
uint32_t crc32_append_zeroes(F mult, uint32_t crc,
        size_t len, uint32_t polynomial)
{
    const uint32_t* powers = crc32c_powers;
    // Append by multiplying by consecutive powers of two of the zeroes
    // array
    len >>= 2;

    while (len) {
        // Advance directly to next bit set.
        int r = __builtin_ffsl(len) - 1;
        len >>= r;
        powers += r;

        crc = mult(crc, *powers, polynomial);

        len >>= 1;
        powers++;
    }

    return crc;
}

static inline bool issse42_supported()
{
    const uint32_t bit_SSE4_2 = (1 << 20);

    int level = 1;
    uint64_t a, b, c, d;
    asm ("xchg\t%%rbx, %1\n\t"
         "cpuid\n\t"
         "xchg\t%%rbx, %1\n\t"
         :"=a"(a), "=r"(b), "=c"(c), "=d"(d)
         :"0"(level));

    return (c & bit_SSE4_2);
}

static inline uint32_t crc32c_combine_hw(uint32_t crc1,
        uint32_t crc2, size_t crc2len)
{
    return crc2 ^
        crc32_append_zeroes(
                gf_multiply_crc32c_hw, crc1, crc2len, crc32c_m);
}


template <size_t N>
struct Crc32MultiplyHelper
{
    static uint32_t Calc(uint32_t p, uint32_t a, uint32_t b, uint32_t m)
        __attribute__((always_inline))
    {
        return Crc32MultiplyHelper<N + 1>::Calc(p ^ (-((b >> 31) & 1) & a),
                (a >> 1) ^ (-(a & 1) & m), b << 1, m);
    }
};

template <>
struct Crc32MultiplyHelper<32>
{
    static uint32_t Calc(uint32_t p, uint32_t a, uint32_t b, uint32_t m)
        __attribute__((always_inline))
    {
        return p;
    }
};

static inline uint32_t gf_multiply_sw(uint32_t a, uint32_t b, uint32_t m)
{
    return Crc32MultiplyHelper<0>::Calc(0, a, b, m);
}

static inline uint32_t crc32c_combine_sw(uint32_t crc1, uint32_t crc2, size_t crc2len)
{
    return crc2 ^
        crc32_append_zeroes(
                gf_multiply_sw, crc1, crc2len, crc32c_m);
}

uint32_t crc32c_combine(uint32_t crc1, uint32_t crc2, size_t crc2len)
{
    if (crc2len == 4 * 1024)
    {
        return crc32c_combine_4KB(crc1, crc2);
    }
    size_t len = crc2len & 3;
    if (len)
    {
        uint32_t data = 0;
        crc1 = docrc32c_intel(crc1,
                reinterpret_cast<uint8_t*>(&data), len);
    }
    if (LIKELY(issse42_supported()))
    {
        return crc32c_combine_hw(crc1, crc2, crc2len - len);
    }
    return crc32c_combine_sw(crc1, crc2, crc2len - len);
}

// Just for test
uint32_t crc32c_combine_hw_for_test(
        uint32_t crc1, uint32_t crc2, size_t crc2len)
{
    uint8_t data[4] = {0, 0, 0, 0};
    size_t len = crc2len & 3;
    if (len)
    {
        crc1 = docrc32c_intel(crc1, data, len);
    }
    return crc32c_combine_hw(crc1, crc2, crc2len - len);
}

uint32_t crc32c_combine_sw_for_test(
        uint32_t crc1, uint32_t crc2, size_t crc2len)
{
    uint8_t data[4] = {0, 0, 0, 0};
    size_t len = crc2len & 3;
    if (len)
    {
        crc1 = docrc32c_intel(crc1, data, len);
    }
    return crc32c_combine_sw(crc1, crc2, crc2len - len);
}

