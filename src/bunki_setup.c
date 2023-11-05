#include "bunki.h"
#include "bunki_ctx.h"
#include "bunki_common.h"

#include <string.h>
#include <limits.h>

#define MULT_OF_ALIGN(x, align) (((x) + ((align) - 1)) & (-(align)))

#if defined(BUNKI_STACK_CONST)
static const uint32_t global_stack_size = (BUNKI_STACK_CONST);
#else
static uint32_t global_stack_size = 0;
#endif

static uintptr_t get_stack_start(bunki_t ctx) {
    uintptr_t ret = (uintptr_t)ctx;
    ret |= (uintptr_t)(global_stack_size - 1);
    ret += 1;
    return ret;
}

static uintptr_t get_stack_base(bunki_t ctx) {
    uintptr_t ret = (uintptr_t)ctx;
    ret &= -((uintptr_t)global_stack_size);
    return ret;
}

uint32_t bunki_stack_min_size(void) {
    uint32_t size = 5 * sizeof(void*) + sizeof(struct stack_ctx_s);
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
    (void) stack_size;
    return 0;
#else
    if(!bunki_is_power2(stack_size) || stack_size < bunki_stack_min_size()) {
        return 1;
    }
    global_stack_size = stack_size;
    return (bunki_patch_call_yield(stack_size) << 1);
#endif
}

void* bunki_stack_ptr(bunki_t ctx) {
    return (void*)get_stack_base(ctx);
}

void* bunki_stack_push(bunki_t* ctx, size_t allocation_length) {
    uintptr_t stk = (uintptr_t)(*ctx);
    stk -= allocation_length;
    stk &= BUNKI_ALIGN_MASK(ARCH_STK_ALIGN);
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

void* bunki_data_get(bunki_t ctx) {
    uintptr_t ptr = get_stack_start(ctx) - 4 * sizeof(void*);
    void* ret;
    memcpy(&ret, (void*)ptr, sizeof(void*));
    return ret;
}

void bunki_data_set(bunki_t ctx, void* data) {
    uintptr_t ptr = get_stack_start(ctx) - 4 * sizeof(void*);
    memcpy((void*)ptr, &data, sizeof(void*));
}

bunki_t bunki_init_stack_ctx(void* stack_mem) {
    uintptr_t stk = (uintptr_t)stack_mem;
    stk += global_stack_size;
    bunki_t ctx   = (bunki_t)stk;
    bunki_stack_push(&ctx, 4 * sizeof(void*));
    //void** ptrs   = bunki_stack_push(&ctx, 4 * sizeof(void*));
    // ptrs[0] user data
    // ptrs[1] the thread stack pointer
    // ptrs[2] caller ctx
    // ptrs[3] callee ctx
    return ctx;
}

void bunki_prepare_ctx(bunki_t ctx, uintptr_t (*func)(void*), void* arg) {
    uintptr_t ptr = get_stack_base(ctx);
    bunki_t ret   = bunki_native_finalize_ctx(ctx, func, arg, ptr);
    ptr  = get_stack_start(ret) - sizeof(void*);
    memcpy((void*)ptr, &ret, sizeof(void*));
}

bunki_t bunki_init_prepare_ctx(void* stack, uintptr_t (*func)(void*), void* arg) {
    bunki_t ctx = bunki_init_stack_ctx(stack);
    bunki_prepare_ctx(ctx, func, arg);
    return ctx;
}
