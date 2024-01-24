#include "bunki.h"
#include "bunki_ctx.h"
#include "bunki_common.h"

#include <string.h>

void bunki_init_ctx(void);
extern uint32_t __bunki_patch0_x0__;
extern uint32_t __bunki_patch1_x6__;
extern uint32_t __bunki_patch2_x0__;
extern uint32_t __bunki_patch3_x6__;
extern uint32_t __bunki_patch4_x0__;
extern uint32_t __bunki_patch5_x29__;
extern uint32_t __bunki_patch6_x6__;

static uint32_t* patch_ptrs[7] = {
    &__bunki_patch0_x0__,
    &__bunki_patch1_x6__,
    &__bunki_patch2_x0__,
    &__bunki_patch3_x6__,
    &__bunki_patch4_x0__,
    &__bunki_patch5_x29__,
    &__bunki_patch6_x6__
};

// stack_size must be greater than 1 and a power of 2
static uint32_t gen_orr(uint32_t dest, uint32_t src, uint32_t stack_size) {
    unsigned bits = bunki_pop_cnt(stack_size - 1) - 1;
    uint32_t ins  = 0xb2400000; // sf = 1, OP = orr, N = 1 for 64 bit pattern
    ins |= bits << 10;          // Set how many 1s we want to OR with src
    ins |= (dest & 0x1f);       // Set dest register
    ins |= (src & 0x1f) << 5;   // Set src register
    #if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
        return BUNKI_BSWAP32(ins);
    #else
        return ins;
    #endif
}

bunki_t bunki_native_finalize_ctx(bunki_t ctx, uintptr_t (*func)(void*), void* arg, uintptr_t stack_end) {
    (void)stack_end; // Suppress unused warning
    uintptr_t stk = (uintptr_t)ctx;
    // If stack alignment does not match use a little space to fix that.
    stk &= BUNKI_ALIGN_MASK(ARCH_STK_ALIGN);
    struct stack_ctx_s* new_ctx = (struct stack_ctx_s*)(stk - sizeof(struct stack_ctx_s));
    // init_ctx moves arg where needed for calling argument then jumps to func.
    new_ctx->lr  = (uintptr_t)bunki_init_ctx;
    new_ctx->x19 = (uintptr_t)func;
    new_ctx->x20 = (uintptr_t)arg;
    return new_ctx;
}

void* bunki_ctx_data_get(void) {
    uintptr_t ptr = bunki_ctx_stack_start() - 0x20;
    void* ret = *((void**)ptr);
    return ret;
}

void bunki_ctx_data_set(void* data) {
    uintptr_t ptr = bunki_ctx_stack_start() - 0x20;
    *((void**)ptr) = data;
}


#if !defined(BUNKI_STACK_CONST) || 1
unsigned bunki_patch_call_yield(uint32_t stack_size) {
    // Probably could do with less calls since I know some these patch points
    // are within a single page, but if I refactor any of the assembly at least
    // I do not need to update this.
    unsigned ret      = 0;
    for(unsigned i = 0; i < 7; i++) {
        ret |= bunki_patch_obj_mprotect_exec(patch_ptrs[i], sizeof(uint32_t), 1);
    }
    if(ret) {
        goto done;
    }
    uint32_t x0  = gen_orr(0,  0,  stack_size);
    uint32_t x6  = gen_orr(6,  6,  stack_size);
    uint32_t x29 = gen_orr(29, 29, stack_size);
    memcpy(&__bunki_patch0_x0__,  &x0,  sizeof(uint32_t));
    memcpy(&__bunki_patch1_x6__,  &x6,  sizeof(uint32_t));
    memcpy(&__bunki_patch2_x0__,  &x0,  sizeof(uint32_t));
    memcpy(&__bunki_patch3_x6__,  &x6,  sizeof(uint32_t));
    memcpy(&__bunki_patch4_x0__,  &x0,  sizeof(uint32_t));
    memcpy(&__bunki_patch5_x29__, &x29, sizeof(uint32_t));
    memcpy(&__bunki_patch6_x6__,  &x6,  sizeof(uint32_t));
done:
    uintptr_t lowest  = UINTPTR_MAX;
    uintptr_t highest = 0;
    for(unsigned i = 0; i < 7; i++) {
        uintptr_t ptr = (uintptr_t)(patch_ptrs[i]);
        if(ptr < lowest) {
            lowest = ptr;
        }
        if(ptr > highest) {
            highest = ptr;
        }
        ret |= bunki_patch_obj_mprotect_exec(patch_ptrs[i], sizeof(uint32_t), 0);
    }
    lowest  &= -(uintptr_t)4096;
    highest &= -(uintptr_t)4096;
    highest += 4096;
    // important since ARM64 has an icache
    // mprotect does flush caches on linux for quite sometime
    // but that is not a given for other *nix systems
    __builtin___clear_cache((char*)lowest, (char*)highest);
    return ret;
}
#endif
