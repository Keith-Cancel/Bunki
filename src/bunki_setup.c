#include "bunki.h"
#include "bunki_ctx.h"
#include <string.h>
#include <limits.h>

#define ALIGN_MASK(val) (~((size_t)val - 1))
#define MULT_OF_ALIGN(x, align) (((x) + ((align) - 1)) & (-(align)))

#if defined(BUNKI_STACK_CONST)
    static const uint32_t global_stack_size = (BUNKI_STACK_CONST);
#else
    static uint32_t global_stack_size = 0;
#endif

static unsigned is_power2(uint32_t number) {
    #if defined(__GNUC__)
        #if UINT_MAX >= UINT32_MAX
            #define BUNKI_USED_POP
            return __builtin_popcount(number) == 1;
        #elif ULONG_MAX >= UINT32_MAX
            #define BUNKI_USED_POP
            return __builtin_popcountl(number) == 1;
        #endif
    #endif
    #if !defined(BUNKI_USED_POP) && defined(_MSC_VER)
        #define BUNKI_USED_POP
        return __popcnt(number) == 0;
    #endif

    #if !defined(BUNKI_USED_POP)
        if(number == 0) {
            return 0;
        }
        return (number & (number - 1)) == 0;
    #endif
}

void* bunki_push_stack(bunki_t* ctx, size_t allocation_length) {
    uintptr_t stk = (uintptr_t)(*ctx);
    stk -= allocation_length;
    stk &= ALIGN_MASK(ARCH_STK_ALIGN);
    *ctx = (bunki_t)stk;
    return (void*)stk;
}

// Push data onto a stack before finalizing a ctx with this. Handy in
// conjuction with the arg parameter for make_ctx.
void* bunki_push_stack_data(bunki_t* ctx, size_t data_length, void* data) {
    void* stk = bunki_push_stack(ctx, data_length);
    memcpy(stk, data, data_length);
    return stk;
}

bunki_t bunki_make_stack_call_ctx(void* stack_mem) {
    uintptr_t stk = (uintptr_t)stack_mem;
    stk += global_stack_size;
    bunki_t ctx = (bunki_t)stk;
    bunki_push_stack(&ctx, 16);
    return ctx;
}

bunki_t bunki_make_stack_swap_ctx(void* stack_mem, size_t stack_length) {
    uintptr_t stk = (uintptr_t)stack_mem;
    stk += stack_length;
    return (bunki_t)stk;
}

uint32_t bunki_min_swap_stack_size(void) {
    return MULT_OF_ALIGN(sizeof(struct stack_ctx_s) + sizeof(void (*)(void)), ARCH_STK_ALIGN);
}

uint32_t bunki_min_call_stack_size(void) {
    uint32_t size = 2 * sizeof(bunki_t);
    size += sizeof(struct stack_ctx_s);
    // round up to nearest power of 2
    size--;
    size |= size >> 1;
    size |= size >> 2;
    size |= size >> 4;
    size |= size >> 8;
    size |= size >> 16;
    size++;
    return size;
}

unsigned bunki_call_init(uint32_t stack_size) {
    #if defined(BUNKI_STACK_CONST)
        return 0;
    #else
        if(!is_power2(stack_size) || stack_size <= bunki_min_call_stack_size()) {
            return 1;
        }
        global_stack_size = stack_size;
        return (bunk_patch_call_yield(stack_size) << 1);
    #endif
}
