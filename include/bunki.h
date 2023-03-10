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
void    bunki_finalize_ctx(bunki_t ctx, uintptr_t (*func)(void*), void* arg);

void*   bunki_large_stack_get(bunki_t ctx);
void    bunki_large_stack_set(bunki_t ctx, void* stack_start);

uintptr_t bunki_resume(bunki_t ctx);
void      bunki_yield(uintptr_t ret);

// Context local Storage functions
void* bunki_ctx_data_get(void);
void  bunki_ctx_data_set(void* data);
void* bunki_ctx_caller_stack(void);

#endif
