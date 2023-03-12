#ifndef BUNKI_COMMON_FILE_H
#define BUNKI_COMMON_FILE_H

#include <stdint.h>
#include <limits.h>

#if defined(__GNUC__)
    #if UINT_MAX >= UINT32_MAX
        #define BUNKI_NATIVE_POPCNT(x) __builtin_popcount(x)
    #elif ULONG_MAX >= UINT32_MAX
        #define BUNKI_NATIVE_POPCNT(x) __builtin_popcountl(x)
    #endif
#elif defined(_MSC_VER)
    #define BUNKI_NATIVE_POPCNT(x) __popcnt(x)
#endif

static inline uint32_t bunki_pop_cnt(uint32_t number) {
    #if defined(BUNKI_NATIVE_POPCNT)
        return BUNKI_NATIVE_POPCNT(number);
    #else
        uint32_t tmp = number & 0x55555555;
        tmp += (number >> 1) & 0x55555555;
        tmp  = (tmp & 0x33333333) + ((tmp >> 2) & 0x33333333);
        // No need to mask here we have a zero to carry into
        tmp += (tmp + (tmp >> 4));
        // Clean up any junk in the upper nibbles of each byte
        tmp &= 0x0f0f0f0f;
        tmp += (tmp >> 8);
        // Still have zeros to carry into
        tmp += (tmp >> 16);
        // Cleanup the junk
        return tmp & 0x3f;
    #endif
}

static inline unsigned bunki_is_power2(uint32_t number) {
    #if defined(BUNKI_NATIVE_POPCNT)
        return BUNKI_NATIVE_POPCNT(number) == 1;
    #else
        if(number == 0) {
            return 0;
        }
        return (number & (number - 1)) == 0;
    #endif
}

#if defined(_MSC_VER) && !defined(__clang__)
    #define BUNKI_BSWAP32(val) _byteswap_ulong(val)
#elif defined(__GNUC__)
    #define BUNKI_BSWAP32(val) __builtin_bswap32(val)
#else
    #define BUNKI_BSWAP32(val) ((val >> 24) | ((val >> 8) & 0xff00) | ((val << 8) & 0xff0000) |  (val << 24))
#endif

#define BUNKI_ALIGN_MASK(val) (~((size_t)val - 1))

// Function prototypes
unsigned bunki_patch_obj_mprotect_exec(void* ptr, uint8_t obj_size, unsigned write);

#endif
