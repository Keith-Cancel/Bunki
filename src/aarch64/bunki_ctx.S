#if !defined(BUNKI_STACK_CONST)
    #define BUNKI_STACK_CONST 0x2
#endif

.macro save_ctx save_ptr, save_off
    stp x20, x19, [sp, -16]!
    stp x22, x21, [sp, -16]!
    stp x24, x23, [sp, -16]!
    stp x26, x25, [sp, -16]!
    stp x28, x27, [sp, -16]!
    stp lr,  x29, [sp, -16]!
    mov x7,  sp
    str	x7, [\save_ptr, -\save_off]
.endm

.macro load_ctx load_ptr, load_off
    ldr	x7, [\load_ptr, -\load_off]
    mov sp,  x7
    ldp lr,  x29, [sp], 16
    ldp x28, x27, [sp], 16
    ldp x26, x25, [sp], 16
    ldp x24, x23, [sp], 16
    ldp x22, x21, [sp], 16
    ldp x20, x19, [sp], 16
.endm

    .text
    .global bunki_resume
    .global __bunki_patch0_x0__
    .align  4
bunki_resume:
__bunki_patch0_x0__:
    orr      x0,  x0, (BUNKI_STACK_CONST - 1)
    save_ctx x0, 0xf
    str	     x7, [x0, -0x17]
    load_ctx x0, 0x7
    ret

    .global bunki_ctx_resume
    .global __bunki_patch1_x6__
    .global __bunki_patch2_x0__
    .align  4
bunki_ctx_resume:
    mov      x6,  sp
__bunki_patch1_x6__:
    orr      x6,  x6, (BUNKI_STACK_CONST - 1)
__bunki_patch2_x0__:
    orr      x0,  x0, (BUNKI_STACK_CONST - 1)
    ldr	     x6, [x6, -0x17]
    str	     x6, [x0, -0x17]
    save_ctx x0, 0xf
    load_ctx x0, 0x7
    ret


    .global bunki_yield
    .global __bunki_patch3_x6__
    .align  4
bunki_yield:
    mov      x6, sp
__bunki_patch3_x6__:
    orr      x6, x6, (BUNKI_STACK_CONST - 1)
    save_ctx x6, 0x7
    load_ctx x6, 0xf
    ret

    .global bunki_ctx_stack_start
    .global __bunki_patch4_x0__
    .align 4
bunki_ctx_stack_start:
    mov x0, sp
__bunki_patch4_x0__:
    orr x0, x0, (BUNKI_STACK_CONST - 1)
    add x0, x0, 1
    ret

    .global bunki_init_ctx
    .global __bunki_patch5_x29__
    .align 4
bunki_init_ctx:
    mov  x0,  x20
    blr  x19
    mov  x29, sp
__bunki_patch5_x29__:
    orr  x29, x29, (BUNKI_STACK_CONST - 1)
    adrp x5,  bunki_set_ret
    add  x5,  x5, #:lo12:bunki_set_ret
    sub  x4,  x29, 0x7f
    str	 x4, [x29, -0x7]
    stp  x5,  x29, [x4]
bunki_done:
    load_ctx x29, 0xf
    ret
bunki_set_ret:
    mov  x0, xzr
    b    bunki_done

    .global bunki_ctx_call
    .align 6
bunki_ctx_call:
    mov x3, x1
    b   bunki_ctx_call_arg3

    .global bunki_ctx_call_arg2
    .align 4
bunki_ctx_call_arg2:
    mov x3, x2
    b   bunki_ctx_call_arg3

    .global bunki_ctx_call_arg3
    .global __bunki_patch6_x6__
    .align 4
bunki_ctx_call_arg3:
    mov  x6, sp
__bunki_patch6_x6__:
    orr  x6, x6, (BUNKI_STACK_CONST - 1)
    ldr	 x6, [x6, -0x17]
    and  x6, x6, -16        // align stack
    mov  x5, sp             // save stack pointer
    mov  sp, x6             // switch stacks
    stp  lr, x5, [sp, -16]! // save ret and stack pointer
    blr  x3
    ldp  lr, x5, [sp], 16
    mov  sp, x5             // restore stack
    ret
