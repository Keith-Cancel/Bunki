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

uint32_t bunki_stack_min_size(void) {
    uint32_t size = 4 * sizeof(void*) + sizeof(struct stack_ctx_s);
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

unsigned bunki_init(uint32_t stack_size) {
    #if defined(BUNKI_STACK_CONST)
        return 0;
    #else
        if(!is_power2(stack_size) || stack_size <=  bunki_stack_min_size()) {
            return 1;
        }
        global_stack_size = stack_size;
        return (bunki_patch_call_yield(stack_size) << 1);
    #endif
}

void* bunki_stack_push(bunki_t* ctx, size_t allocation_length) {
    uintptr_t stk = (uintptr_t)(*ctx);
    stk -= allocation_length;
    stk &= ALIGN_MASK(ARCH_STK_ALIGN);
    *ctx = (bunki_t)stk;
    return (void*)stk;
}

// Push data onto a stack before finalizing a ctx with this. Handy in
// conjuction with the arg parameter for a make ctx function.
void* bunki_stack_push_data(bunki_t* ctx, size_t data_length, void* data) {
    void* stk = bunki_stack_push(ctx, data_length);
    memcpy(stk, data, data_length);
    return stk;
}

bunki_t bunki_init_stack_ctx(void* stack_mem) {
    uintptr_t stk = (uintptr_t)stack_mem;
    stk += global_stack_size;
    bunki_t ctx   = (bunki_t)stk;
    void** ptrs = bunki_stack_push(&ctx, 4 * sizeof(void*));
    // ptrs[0] user data
    ptrs[1] = stack_mem; // The stack base.
    // ptrs[2] caller ctx
    // ptrs[3] callee ctx
    return ctx;
}

void bunki_finalize_ctx(bunki_t ctx, uintptr_t (*func)(void*), void* arg) {
    uintptr_t ptr = (uintptr_t)ctx;
    ptr &= -((uintptr_t)global_stack_size);
    bunki_t ret = bunki_native_finalize_ctx(ctx, func, arg, ptr);
    ptr = (uintptr_t)ret;
    ptr |= global_stack_size - 1;
    ptr -= 0x7;
    memcpy((void*)ptr, &ret, sizeof(void*));
}
