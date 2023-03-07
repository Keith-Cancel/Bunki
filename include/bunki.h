#ifndef BUNKI_FILE_H
#define BUNKI_FILE_H

#include <stddef.h>
#include <stdint.h>

typedef struct stack_ctx_s* bunki_t;

void*   bunki_push_stack(bunki_t* ctx, size_t allocation_length);
void*   bunki_push_stack_data(bunki_t* ctx, size_t data_length, void* data);

bunki_t bunki_make_stack_call_ctx(void* stack_mem);
bunki_t bunki_make_stack_swap_ctx(void* stack_mem, size_t stack_length);

#endif
