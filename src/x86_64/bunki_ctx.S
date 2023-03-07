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

.macro save_ctx save_ptr, save_off
    # save the current context on the stack
    push rbx
    push rbp
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
.endm

.macro load_ctx load_ptr, load_off
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
    pop rbp
    pop rbx
.endm

    .text
    .global bunki_call
    .global __bunki_patch0__
    .align  16
bunki_call:
    #if defined(_WIN64)
        .byte 0x48, 0x81, 0xc9
    #endif
    #if defined(SYSTEM_V_ABI)
        .byte 0x48, 0x81, 0xcf
    #endif
__bunki_patch0__:
    .long ((BUNKI_STACK_CONST) - 1)
    save_ctx ARG1, 0xf
    load_ctx ARG1, 0x7
    ret

    .global bunki_yield
    .global __bunki_patch1__
    .align 16
bunki_yield:
    mov rax, rsp
    .byte 0x48, 0x0d
__bunki_patch1__:
    .long ((BUNKI_STACK_CONST) - 1)
    save_ctx rax, 0x7
    load_ctx rax, 0xf
    mov rax, ARG1
    ret

    .global bunki_init_ctx
    .global __bunki_patch2__
    .align 16
bunki_init_ctx:
    mov  ARG1, r15
    call rbx
    mov  rbx, rsp
    .byte 0x48, 0x81, 0xcb
__bunki_patch2__:
    .long ((BUNKI_STACK_CONST) - 1)
    lea  ARG1, [rip + bunki_set_rax]
    push ARG1
    push rbx
    #if defined(_WIN64)
        sub  rsp, 216
    #else
        sub  rsp, 48
    #endif
    mov  [rbx - 0x7], rsp
bunki_done:
    load_ctx rbx, 0xf
    ret

    .align 16
bunki_set_rax:
    xor eax, eax
    jmp bunki_done

    .global bunki_swap_ctx
    .align 16
bunki_swap_ctx:
    save_ctx ARG3, 0x0
    load_ctx ARG2, 0x0
    mov rax, ARG1
    ret
