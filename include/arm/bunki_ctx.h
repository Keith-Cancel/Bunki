#ifndef BUNKI_ARM_CTX_FILE_H
#define BUNKI_ARM_CTX_FILE_H

#if !defined(__arm__)
    #error "This is not an ARM architecture"
#endif

#include <stdint.h>

struct stack_ctx_s {
    uint32_t r4;
    uint32_t r5;
    uint32_t r6;
    uint32_t r7;
    uint32_t r8;
    uint32_t r9;
    uint32_t r10;
    uint32_t fp;
    uint32_t lr;
};

#define ARCH_STK_ALIGN 0x08U

unsigned  bunki_patch_call_yield(uint32_t stack_size);
bunki_t   bunki_native_finalize_ctx(bunki_t ctx, uintptr_t (*func)(void*), void* arg, uintptr_t stack_end);
uintptr_t bunki_ctx_stack_start(void);

#endif
