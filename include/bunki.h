#ifndef BUNKI_FILE_H
#define BUNKI_FILE_H

#include <stddef.h>
#include <stdint.h>

typedef struct stack_ctx_s* bunki_t;

unsigned bunki_init(uint32_t stack_size);
uint32_t bunki_stack_min_size(void);

void*   bunki_stack_push(bunki_t* ctx, size_t allocation_length);
void*   bunki_stack_push_data(bunki_t* ctx, size_t data_length, void* data);

bunki_t bunki_init_stack_ctx(void* stack_mem);
void    bunki_prepare_ctx(bunki_t ctx, uintptr_t (*func)(void*), void* arg);
bunki_t bunki_init_prepare_ctx(void* stack_mem, uintptr_t (*func)(void*), void* arg);

uintptr_t bunki_resume(bunki_t ctx);
void      bunki_yield(uintptr_t ret);

// Functions that can only be called with a coroutine ctx
uintptr_t bunki_ctx_resume(bunki_t ctx);
// Context local Storage functions
void*     bunki_ctx_data_get(void);
void      bunki_ctx_data_set(void* data);
// Call a function on the thread stack
uintptr_t bunki_ctx_call(void* arg, uintptr_t (*func)(void*));
uintptr_t bunki_ctx_call_arg2(void* arg0, void* arg1, uintptr_t (*func)(void*, void*));
uintptr_t bunki_ctx_call_arg3(void* arg0, void* arg1, void* arg2, uintptr_t (*func)(void*, void*, void*));

#endif
