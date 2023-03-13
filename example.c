#include "bunki.h"
#include <stdio.h>
#include <stdlib.h>

uintptr_t print_unsigned(void* arg) {
    uintptr_t val = (uintptr_t)arg;
    printf("The value is: 0x%lx\n", val);
    return 0;
}

uintptr_t my_coroutine(void* arg) {
    // Save a value to the thread context storage
    bunki_ctx_data_set((void*)0xcafe);
    // Call a function on the current threads stack
    bunki_ctx_call((void*)0xf00d, print_unsigned);
    bunki_yield(10);

    // print out the value we save before last yield
    bunki_ctx_call((void*)bunki_ctx_data_get(), print_unsigned);
    // print the arg
    bunki_ctx_call(arg, print_unsigned);
    bunki_yield(12);
}

int main() {
    // only call once before using the library
    // The library will now expect stack memory to
    // aligned by 256 bytes and be 256 bytes in size.
    if(bunki_init(256)) {
        fprintf(stderr, "Failed to initialize!\n");
        return 1;
    }
    void* stack_mem = aligned_alloc(256, 256);
    if(stack_mem == NULL) {
        fprintf(stderr, "Failed to allocate memory!\n");
        return 1;
    }
    bunki_t ctx = bunki_init_prepare_ctx(stack_mem, my_coroutine, (void*)0xbeef);
    printf("Returned: %u\n", bunki_resume(ctx));
    printf("Returned: %u\n", bunki_resume(ctx));
    free(stack_mem);
    return 0;
}
