#include "bunki.h"
#include "bunki_ctx.h"
#include "bunki_common.h"

#include <string.h>

void bunki_init_ctx(void);
extern uint32_t __bunki_patch0_r0_r0__;
extern uint32_t __bunki_patch1_r3_sp__;
extern uint32_t __bunki_patch2_r0_r0__;
extern uint32_t __bunki_patch3_r3_sp__;
extern uint32_t __bunki_patch4_r0_sp__;
extern uint32_t __bunki_patch5_r11_sp__;
extern uint32_t __bunki_patch6_r4_sp__;

static uint32_t* patch_ptrs[] = {
    &__bunki_patch0_r0_r0__,
    &__bunki_patch1_r3_sp__,
    &__bunki_patch2_r0_r0__,
    &__bunki_patch3_r3_sp__,
    &__bunki_patch4_r0_sp__,
    &__bunki_patch5_r11_sp__,
    &__bunki_patch6_r4_sp__
};

// stack_size must be greater than 1 and a power of 2
static uint32_t gen_orr(uint32_t dest, uint32_t src, uint32_t stack_size) {
    uint32_t ins = 0xe3800000; // S = 0
    ins |= (stack_size - 1) & 0x0fff;
    ins |= (src & 0x000f) << 16;
    ins |= (dest & 0x000f) << 12;
    #if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
        return BUNKI_BSWAP32(ins);
    #else
        return ins;
    #endif
}

//

bunki_t bunki_native_finalize_ctx(bunki_t ctx, uintptr_t (*func)(void*), void* arg, uintptr_t stack_end) {
    (void) stack_end;
    uintptr_t stk = (uintptr_t)ctx;
    // If stack alignment does not match use a little space to fix that.
    stk &= BUNKI_ALIGN_MASK(ARCH_STK_ALIGN);
    struct stack_ctx_s* new_ctx = (struct stack_ctx_s*)(stk - sizeof(struct stack_ctx_s));
    // init_ctx moves arg where needed for calling argument then jumps to func.
    new_ctx->lr  = (uintptr_t)bunki_init_ctx;
    new_ctx->r4 = (uintptr_t)func;
    new_ctx->r5 = (uintptr_t)arg;
    return new_ctx;
}

void* bunki_ctx_data_get(void) {
    uintptr_t ptr = bunki_ctx_stack_start() - 0x10;
    void* ret = *((void**)ptr);
    return ret;
}

void bunki_ctx_data_set(void* data) {
    uintptr_t ptr = bunki_ctx_stack_start() - 0x10;
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
    uint32_t r0_r0  = gen_orr(0,  0,  stack_size);
    uint32_t r3_sp  = gen_orr(3,  13,  stack_size);
    uint32_t r0_sp  = gen_orr(0,  13,  stack_size);
    uint32_t r11_sp  = gen_orr(11,  13,  stack_size);
    uint32_t r4_sp  = gen_orr(4,  13,  stack_size);
    memcpy(&__bunki_patch0_r0_r0__,  &r0_r0,  sizeof(uint32_t));
    memcpy(&__bunki_patch1_r3_sp__,  &r3_sp,  sizeof(uint32_t));
    memcpy(&__bunki_patch2_r0_r0__,  &r0_r0,  sizeof(uint32_t));
    memcpy(&__bunki_patch3_r3_sp__,  &r3_sp,  sizeof(uint32_t));
    memcpy(&__bunki_patch4_r0_sp__,  &r0_sp,  sizeof(uint32_t));
    memcpy(&__bunki_patch5_r11_sp__,  &r11_sp,  sizeof(uint32_t));
    memcpy(&__bunki_patch6_r4_sp__,  &r4_sp,  sizeof(uint32_t));
    
done: ;
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
    // important since ARM has an icache
    // mprotect does flush caches on linux for quite sometime
    // but that is not a given for other *nix systems
    __builtin___clear_cache((char*)lowest, (char*)highest);
    return ret;
}
#endif
