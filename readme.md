# Bunki 🛤️
A simple to use C stackful coroutine library.

The name is Japanese word bunki (分岐) which means to branch off. I consider the name quite fitting for a coroutine library just google image (分岐) and you will see what I mean.

Currently supports the Sys-V call convention for x86_64 and the Win64 x64_86 calling convention, and aarch64.

Issues and PRs are welcome 😃

# Table of Contents
   * [Example](#example)
   * [Building](#building)
      * [Defines](#defines)
   * [API](#api)
      * [bunki_init](#bunki_init)
      * [bunki_stack_min_size](#bunki_stack_min_size)
      * [bunki_init_stack_ctx](bunki_init_stack_ctx)
      * [bunki_prepare_ctx](#bunki_prepare_ctx)
      * [bunki_init_prepare_ctx](#bunki_init_prepare_ctx)
      * [bunki_resume](#bunki_resume)
      * [bunki_ctx_resume](#bunki_ctx_resume)
      * [bunki_yield](#bunki_yield)
      * [bunki_stack_push](#bunki_stack_push)
      * [bunki_stack_push_data](#bunki_stack_push_data)
      * [bunki_ctx_data_get](#bunki_ctx_data_get)
      * [bunki_ctx_data_set](#bunki_ctx_data_set)
      * [bunki_ctx_call](#bunki_ctx_call)
   * [Notes](#notes)
   * [TODO](#todo)
   * [Copyright and License](#copyright-and-license)


# Example

```c
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
```
# Building

The project does have a makefile that will be build the library. Simply run `make` and a static library will be built and copied to the build directory. The build directory will contain 2 folders lib and include. The lib directory will contain a static library you can link against, and the include directory will include header to include that should be included in applications using this library.

If not using the makefile compile `bunki_setup.c`, `bunki_common.c`, and the following for your architecture `bunki_native.c`, `bunki_ctx.S`. That's it just four files.

## Defines

* `BUNKI_STACK_CONST`

The `BUNKI_STACK_CONST` define lets you build a version of library so it can be used without calling `bunki_init()`. The main advantage is that it avoids the runtime code-patching and lets the compiler optimize with a constant for the stack sizes. Simply add to the CFLAGS in the makefile `-DBUNKI_STACK_CONST=<your-stack-size>`. The stack size must be a power of 2.

* `BUNKI_SHARE_FCW_MXCSR`

This define only applies to x86_64 builds. Adding `BUNKI_SHARE_FCW_MXCSR` as a define will make the state of a context switch a bit smaller. It also means that your coroutines will use whatever the current value of the MXCSR Register and FPU Control Word.

* `BUNKI_NO_WIN64_XMM`

This define only applies to x86_64 on windows. The define should be used with **CAUTION**. This define only applies to windows on x86_64, and defining `BUNKI_NO_WIN64_XMM` means that the non-volatile `xmm<n>` registers will no longer be saved durning a coroutine context switch. If your familiar with windows fibers this define is similar to not including the `FIBER_FLAG_FLOAT_SWITCH`. **See**: [Microsoft Fiber Parameters](https://web.archive.org/web/20230313065255/https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-createfiberex#parameters) It is still possible to use floating arithmetic with this define, but one must ensure that no stack frames above any context switch are not storing any floating or vector state in the registers `xmm6`-`xmm15`

* `BUNKI_AARCH64_NO_VEC_FLOAT`

This define only applies to aarch64. The define is similar to `BUNKI_NO_WIN64_XMM`, although the amount of context data saved from this is not as large as windows. I only recommend defining this if the hardware your targeting does not have hardware support for SIMD **AND** floating point numbers. If using to make the context switch smaller/faster proceed with **CAUTION**.

# API

## bunki_init

```c
unsigned bunki_init(uint32_t stack_size);
```

Initializes the environment for the library to be used. The argument to this function is the size of stacks you wish to use in bytes. The size must be a power of 2 since the library uses that to be able to quickly get the information about a coroutines context. This function shall **NOT** be called if any coroutines that will be resumed were made before the call to this function. Further defining `BUNKI_STACK_CONST` make this function a no op.

* The function returns 0 on success.
* The function returns 1 if the stack is too small or not a power of 2.
* The function returns 2 if it can not patch code with the needed values.

## bunki_stack_min_size

```c
uint32_t bunki_stack_min_size(void);
```

This function returns the smallest power of 2 stack size in bytes that can be used. Any memory allocated to make a context must be greater than or equal to this amount. Further this is the smallest size [bunki_init()](#bunki_init) will accept.


## bunki_init_stack_ctx

```c
bunki_t bunki_init_stack_ctx(void* stack_mem);
```

Creates a bunki_t context from the given memory. The size of stack_mem parameter **MUST** be equal to the size provided to [bunki_init()](#bunki_init) further the alignment of this memory **MUST** be aligned by that size amount. For example if the size of the stacks are 256 bytes `stack_mem` must be on a 256 byte alignment.


## bunki_prepare_ctx

```c
void bunki_prepare_ctx(bunki_t ctx, uintptr_t (*func)(void*), void* arg);
```

This function readies your `bunki_t` context so it can be resumed with [bunki_resume()](#bunki_resume) or [bunki_ctx_resume()](#bunki_ctx_resume). The reason for separate init and prepare functions is so that [bunki_stack_push()](#bunki_stack_push) and [bunki_stack_push_data()](#bunki_stack_push_data) can be used to store data on the stack before using the coroutine.

* The `ctx` parameter is your coroutines context.
* The `func` parameter is a function pointer to the starting function of your coroutine.
* THe `arg` parameter is the starting argument that will be passed to your starting function.

## bunki_init_prepare_ctx

```c
bunki_t bunki_init_prepare_ctx(void* stack_mem, uintptr_t (*func)(void*), void* arg);
```

This function is equivalent to calling [bunki_init_stack_ctx()](#bunki_init_stack_ctx) and [bunki_prepare_ctx()](#bunki_prepare_ctx) in succession.

```c
// ... code ...
bunki_t ctx = bunki_init_prepare_ctx(mem, begin_func, NULL);
// Is equivalent too:
bunki_t ctx = bunki_init_stack_ctx(mem);
bunki_prepare_ctx(ctx, begin_func, NULL);
// ... more code ...
```

## bunki_resume

```c
uintptr_t bunki_resume(bunki_t ctx);
```

Yields from the main thread and begins/resumes the execution of the coroutine passed to resume. The returned value is the value yielded or returned from the resumed coroutine. The type uintptr_t was chosen to make returning integer codes more ergonomic, but still allow coroutines to return pointers if needed. Do **NOT** call this function inside a coroutine if nested coroutines are needed call [bunki_ctx_resume()](#bunki_ctx_resume) instead. Otherwise, your coroutines can not use the family of `bunki_ctx_call` functions safely.

## bunki_ctx_resume

```c
uintptr_t bunki_ctx_resume(bunki_t ctx);
```

This function behaves just like [bunki_resume()](#bunki_resume), but **MUST** only be called inside a running coroutine. Calling it outside of a coroutine will likely crash your program. This function exists to allow for nested coroutine use.


## bunki_yield

```c
void bunki_yield(uintptr_t ret);
```
Yield the execution of the coroutine and resumes the caller. The caller gets the value passed passed to the argument `ret` returned to them. This function **MUST** not be called outside of a coroutine.

## bunki_stack_push
```c
void* bunki_stack_push(bunki_t* ctx, size_t allocation_length);
```
`bunki_stack_push()` lets you reserve memory on a coroutines stack to store information there. The parameter `allocation_length` is how many bytes to reserve. This can be handy for instance to store a structure that you may provide as the starting argument for your coroutines starting function. Please keep in mind that does reduce the amount of stack space available to the coroutine. This function **MUST** not be called after calling [bunki_prepare_ctx()](#bunki_prepare_ctx) or [bunki_init_prepare_ctx()](#bunki_init_prepare_ctx). This function is meant to be used after calling [bunki_init_stack_ctx()](#bunki_init_stack_ctx), but before the coroutine has been fully prepared.

* The return value is the pointer to allocated memory on the coroutines stack.

## bunki_stack_push_data
```c
void* bunki_stack_push_data(bunki_t* ctx, size_t data_length, void* data);
```

This function behaves almost exactly like [bunki_stack_push()](#bunki_stack_push), but in addition it copies the data from the 3rd parameter to the memory reserved on the coroutines stack.

## bunki_ctx_data_get
```c
void* bunki_ctx_data_get(void);
```

This function lets you get the value previously stored in thread context storage. This function **MUST** only be called inside a coroutine.

## bunki_ctx_data_set
```c
void  bunki_ctx_data_set(void* data);
```

This function and [bunki_ctx_data_get()](#bunki_ctx_data_get) are the building blocks for you to create thread local storage. If you need more data than what can be stored in a `void*`, feel free to allocate memory and pass a pointer to that memory instead. Just don't forget to free when your coroutine is done 😉. This function **MUST** only be called inside a coroutine.

## bunki_ctx_call
```c
uintptr_t bunki_ctx_call(void* arg, uintptr_t (*func)(void*));
```
When using stackful coroutines ideally you want small stacks. The drawback of that is you can't call any functions that generate deep call stacks. This function lets you get around that drawback by calling the function pointer provided to the second argument on the threads stack instead.

The first parameter `arg` is passed to the parameter `func` when called. The return value is the value returned from function pointer when called.

This function **MUST** only be called inside a coroutine, and secondly while on the the thread's stack [bunki_yield](#bunki_yield) and any function prefixed with `bunki_ctx` **MUST** not be called.

## bunki_ctx_call_arg2
```c
uintptr_t bunki_ctx_call_arg2(void* arg0, void* arg1, uintptr_t (*func)(void*, void*));
```
This function behaves just like [bunki_ctx_call()](#bunki_ctx_call), but 2 arguments are passed to `func` when called instead.

## bunki_ctx_call_arg3
```c
uintptr_t bunki_ctx_call_arg3(void* arg0, void* arg1, void* arg2, uintptr_t (*func)(void*, void*, void*));
```
This function behaves just like [bunki_ctx_call()](#bunki_ctx_call), but 3 arguments are passed to `func` when called instead.

# Notes

Unlike most other coroutine libraries you are allowed to return from a coroutine in Bunki with the `return` keyword. You can use this create a one-time/one-shot function. For example:

```c
#include "bunki.h"
#include <stdio.h>
#include <stdlib.h>

uintptr_t my_one_shot(void* arg) {
    return 0xbeef;
}

int main() {
    if(bunki_init(256)) {
        fprintf(stderr, "Failed to initialize!\n");
        return 1;
    }
    void* stack_mem = aligned_alloc(256, 256);
    if(stack_mem == NULL) {
        fprintf(stderr, "Failed to allocate memory!\n");
        return 1;
    }
    bunki_t ctx = bunki_init_prepare_ctx(stack_mem, my_one_shot, (void*)0xbeef);
    printf("Returned: 0x%x\n", bunki_resume(ctx));
    printf("Returned: 0x%x\n", bunki_resume(ctx));
    free(stack_mem);
    return 0;
}
```


The output of this will be:
```bash
Returned: 0xbeef
Returned: 0x0
```

After the coroutine `my_one_shot` returns it never will run again and calling [bunki_resume](#bunki_resume) afterwards will always return 0. Thus making it a one-time/one-shot function.


# TODO

Nothing here at the moment, but new ideas are Welcome!

# Copyright and License

Copyright (C) 2023, by Keith Cancel [<admin@keith.pro>](mailto:admin@keith.pro).

Under the MIT License
