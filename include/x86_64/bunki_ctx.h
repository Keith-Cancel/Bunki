#ifndef BUNKI_x64_86_CTX_FILE_H
#define BUNKI_x64_86_CTX_FILE_H

#if !defined(__x86_64__) && !defined(_M_X64)
    #error "This is not an x86_64 architecture"
#endif

#include <stdint.h>

struct stack_ctx_s {
    #if !defined(BUNKI_SHARE_FCW_MXCSR)
        union {
            struct {
                uint32_t mxcsr;
                uint16_t fcw;
                uint16_t pad;
            };
            uint64_t both_csrs;
        };
    #endif
    #ifdef _WIN64
        #if !defined(BUNKI_NO_WIN64_XMM)
        uint8_t xmm8_15[128];
        #endif
        uint64_t stk_base;
        uint64_t stk_limit;
        uint64_t stk_dealloc;
        uint64_t rsi;
        uint64_t rdi;
    #endif
    // System V and Windows x64
    uint64_t r15;
    uint64_t r14;
    uint64_t r13;
    uint64_t r12;
    uint64_t rbp;
    uint64_t rbx;
    uint64_t rip; // The rip or return ptr
    // Windows shadow/home space
    #ifdef _WIN64
        uint8_t home[32]; // Also store xmm6 and xmm7 here
    #endif
};

#define ARCH_STK_ALIGN 0x10U

unsigned  bunki_patch_call_yield(uint32_t stack_size);
bunki_t   bunki_native_finalize_ctx(bunki_t ctx, uintptr_t (*func)(void*), void* arg, uintptr_t stack_end);
uintptr_t bunki_ctx_stack_start(void);

#endif
