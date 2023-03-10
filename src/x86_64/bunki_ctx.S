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

#if defined(_WIN64) && !defined(BUNKI_NO_WIN64_XMM)
    #define WIN_SZ 168
#elif defined(_WIN64)
    #define WIN_SZ 40
#else
    #define WIN_SZ 0
#endif

#if !defined(BUNKI_SHARE_FCW_MXCSR)
    #define FCW_SAVE 8
#else
    #define FCW_SAVE 0
#endif

#define CTX_SZ (56 + FCW_SAVE + WIN_SZ)

.macro save_ctx save_ptr, save_off
    # save the current context on the stack
    push rbx
    push rbp
    push r12
    push r13
    push r14
    push r15
    #if defined(_WIN64)
        push rdi
        push rsi
        # Get TIB pointer
        mov  rdi, gs:0x30
        push [rdi + 0x1478]
        push [rdi + 0x0010]
        push [rdi + 0x0008]
        #if !defined(BUNKI_NO_WIN64_XMM)
            # Store xmm6 and xmm7 in the shadow/home space
            movaps [rsp + 0x60], xmm6
            movaps [rsp + 0x70], xmm7
            sub    rsp, 0x80
            movaps [rsp + 0x00], xmm15
            movaps [rsp + 0x10], xmm14
            movaps [rsp + 0x20], xmm13
            movaps [rsp + 0x30], xmm12
            movaps [rsp + 0x40], xmm11
            movaps [rsp + 0x50], xmm10
            movaps [rsp + 0x60], xmm9
            movaps [rsp + 0x70], xmm8
        #endif
    #endif
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
    #if defined(_WIN64)
        #if !defined(BUNKI_NO_WIN64_XMM)
            movaps xmm15, [rsp + 0x00]
            movaps xmm14, [rsp + 0x10]
            movaps xmm13, [rsp + 0x20]
            movaps xmm12, [rsp + 0x30]
            movaps xmm11, [rsp + 0x40]
            movaps xmm10, [rsp + 0x50]
            movaps xmm9,  [rsp + 0x60]
            movaps xmm8,  [rsp + 0x70]
            add    rsp,   0x80
            # load xmm6 and xmm7 from the shadow/home space
            movaps xmm7,  [rsp + 0x70]
            movaps xmm6,  [rsp + 0x60]
        #endif
        mov rdi, gs:0x30
        # needs to be set for things like __chkstk to work properly
        pop [rdi + 0x0008]
        pop [rdi + 0x0010]
        # from what I gather not setting this can trigger some
        # stack corruption protection code on windows and other problems.
        pop [rdi + 0x1478]
        pop rsi
        pop rdi
    #endif
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbp
    pop rbx
.endm

# bunki_resume function
    .text
    .global bunki_resume
    .global __bunki_patch0__
    .align  16
bunki_resume:
    #if defined(_WIN64)
        .byte 0x48, 0x81, 0xc9
    #endif
    #if defined(SYSTEM_V_ABI)
        .byte 0x48, 0x81, 0xcf
    #endif
__bunki_patch0__:
    .long ((BUNKI_STACK_CONST) - 1)
    save_ctx ARG1, 0xf
    # save the thread stack
    and rsp, -16
    mov [ARG1 - 0x17], rsp
    load_ctx ARG1, 0x7
    ret

# bunki_ctx_resume function
    .text
    .global bunki_ctx_resume
    .global __bunki_patch1__
    .align  16
bunki_ctx_resume:
    .byte 0x48, 0xc7, 0xc0
__bunki_patch1__:
    .long ((BUNKI_STACK_CONST) - 1)
    or       ARG1, rax
    save_ctx ARG1, 0xf
    # get the the thread stack from
    # the callers ctx.
    or       rax,  rsp
    mov      rax,  [rax - 0x17]
    mov      [ARG1 - 0x17], rax
    load_ctx ARG1, 0x7
    ret

# bunki_yield function
    .global bunki_yield
    .global __bunki_patch2__
    .align 16
bunki_yield:
    mov rax, rsp
    .byte 0x48, 0x0d
__bunki_patch2__:
    .long ((BUNKI_STACK_CONST) - 1)
    save_ctx rax, 0x7
    load_ctx rax, 0xf
    mov rax, ARG1
    ret

# bunki_init_ctx function
    .global bunki_init_ctx
    .global __bunki_patch3__
    .align 16
bunki_init_ctx:
    mov  ARG1, r15
    call rbx
    mov  rbx, rsp
    .byte 0x48, 0x81, 0xcb
__bunki_patch3__:
    .long ((BUNKI_STACK_CONST) - 1)
    lea  ARG1, [rip + bunki_set_rax]
    push ARG1
    push rbx
    sub  rsp, (CTX_SZ - 16)
    #if !defined(BUNKI_SHARE_FCW_MXCSR)
        stmxcsr [rsp]
        fnstcw  [rsp + 0x04]
    #endif
    mov [rbx - 0x7], rsp
bunki_done:
    load_ctx rbx, 0xf
    ret

    .align 16
bunki_set_rax:
    xor eax, eax
    jmp bunki_done

# bunki_ctx_stack_start
    .global bunki_ctx_stack_start
    .global __bunki_patch4__
    .align 16
bunki_ctx_stack_start:
    mov rax, rsp
    .byte 0x48, 0x0d
__bunki_patch4__:
    .long ((BUNKI_STACK_CONST) - 1)
    inc rax
    ret

# bunki_ctx_data_get
    .global bunki_ctx_data_get
    .align 16
bunki_ctx_data_get:
    call bunki_ctx_stack_start
    mov rax, [rax - 0x20]
    ret

# bunki_ctx_data_set
    .global bunki_ctx_data_set
    .align 16
bunki_ctx_data_set:
    call bunki_ctx_stack_start
    mov [rax - 0x20], ARG1
    ret

    .global bunki_call_on_stack
    .align 16
bunki_call_on_stack:
    and ARG3, -16 # align stack
    mov  rax, rsp
    mov  rsp, ARG3
    sub  rsp, 8 # to ensure stack is aligned before call
    push rax
    call ARG2
    pop  rsp
    ret
