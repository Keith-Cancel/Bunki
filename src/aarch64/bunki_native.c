#include "bunki.h"
#include "bunki_ctx.h"
#include "bunki_common.h"

#include <string.h>

void bunki_init_ctx(void);

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
