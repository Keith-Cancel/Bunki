#ifndef BUNKI_AARCH64_CTX_FILE_H
#define BUNKI_AARCH64_CTX_FILE_H

#if !defined(__aarch64__)
    #error "This is not an ARM64 architecture"
#endif

#include <stdint.h>

struct stack_ctx_s {
    uint64_t lr;
    uint64_t x29;
    uint64_t x28;
    uint64_t x27;
    uint64_t x26;
    uint64_t x25;
    uint64_t x24;
    uint64_t x23;
    uint64_t x22;
    uint64_t x21;
    uint64_t x20;
    uint64_t x19;
    #if !defined(BUNKI_AARCH64_NO_VEC_FLOAT)
        uint64_t d15;
        uint64_t d14;
        uint64_t d13;
        uint64_t d12;
        uint64_t d11;
        uint64_t d10;
        uint64_t d9;
        uint64_t d8;
    #endif
};

#define ARCH_STK_ALIGN 0x10U

unsigned  bunki_patch_call_yield(uint32_t stack_size);
bunki_t   bunki_native_finalize_ctx(bunki_t ctx, uintptr_t (*func)(void*), void* arg, uintptr_t stack_end);
uintptr_t bunki_ctx_stack_start(void);

#endif
