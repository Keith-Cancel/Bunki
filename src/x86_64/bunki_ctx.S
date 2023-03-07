.intel_syntax noprefix

#if defined(__linux__)
    #define SYSTEM_V_ABI 1
#elif defined(__APPLE__) && defined(__MACH__)
    #define SYSTEM_V_ABI 1
#elif defined(__OpenBSD__) || defined(__FreeBSD__) || defined(__NetBSD__)
    #define SYSTEM_V_ABI 1
#endif

#if defined(SYSTEM_V_ABI)
    #define ARG1 rdi
    #define ARG2 rsi
    #define ARG3 rdx
#elif defined(_WIN64)
    #define ARG1 rcx
    #define ARG2 rdx
    #define ARG3 r8
#else
    #error Unknown x86_64 ABI. Sadly not ported to your system.
#endif

#if !defined(BUNKI_STACK_CONST)
    #define BUNKI_STACK_CONST 0x01
#endif

.macro swap_ctx save_ptr, save_off, load_ptr, load_off
    # save the current context on the stack
    push rbp
    push rbx
    push r12
    push r13
    push r14
    push r15
    #if !defined(BUNKI_SHARE_FCW_MXCSR)
        sub      rsp, 8
        stmxcsr [rsp]
        fnstcw  [rsp + 0x04]
    #endif
    mov [\save_ptr - \save_off], rsp
    # load the context we are swapping too
    mov rsp, [\load_ptr - \load_off]
    #if !defined(BUNKI_SHARE_FCW_MXCSR)
        // fnclex maybe?
        ldmxcsr [rsp]
        fldcw   [rsp + 0x04]
        add     rsp, 8
    #endif
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    pop rbp
.endm

    .text
    .global bunki_call
    .global __bunki_patch0__
    .align  16
bunki_call:
    #if defined(_WIN64) || 1
        .byte 0x48, 0x81, 0xca
    #endif
    #if defined(SYSTEM_V_ABI) && 0
        .byte 0x48, 0x81, 0xce
    #endif
__bunki_patch0__:
    .long ((BUNKI_STACK_CONST) - 1)
    swap_ctx ARG2, 0xf, ARG2, 0x7
    mov rax, ARG1
    ret

    .global bunki_yield
    .global __bunki_patch1__
    .align 16
bunki_yield:
    mov rax, rsp
    .byte 0x48, 0x0d
__bunki_patch1__:
    .long ((BUNKI_STACK_CONST) - 1)
    swap_ctx rax, 0x7, rax, 0xf
    mov      rax, ARG1
    ret

bunki_swap_ctx:
    swap_ctx ARG3, 0, ARG2, 0
    mov rax, ARG1
    ret