#if !defined(BUNKI_STACK_CONST)
    #define BUNKI_STACK_CONST 0x2
#endif

.macro save_ctx save_ptr, save_off
	push { r4-r11, lr }    
	str  sp, [\save_ptr, #-\save_off]
.endm

.macro load_ctx load_ptr, load_off
	ldr sp, [\load_ptr, #-\load_off]
	pop { r4-r11, lr }   
.endm

.text

.global bunki_resume
.global __bunki_patch0_r0_r0__
.align  4
bunki_resume:
__bunki_patch0_r0_r0__:
	orr      r0, r0, #(BUNKI_STACK_CONST - 1)
	save_ctx r0, 0x7
	str	 sp, [r0, #-0xb]
	load_ctx r0, 0x3
	mov      pc, lr

.global bunki_ctx_resume
.global __bunki_patch1_r3_sp__
.global __bunki_patch2_r0_r0__
.align  4
bunki_ctx_resume:
__bunki_patch1_r3_sp__:
	orr      r3, sp, #(BUNKI_STACK_CONST - 1)
__bunki_patch2_r0_r0__:
	orr      r0, r0, #(BUNKI_STACK_CONST - 1)
	ldr	 r3, [r3, #-0xb]
	str	 r3, [r0, #-0xb]
	save_ctx r0, 0x7
	load_ctx r0, 0x3
        mov      pc, lr

.global bunki_yield
.global __bunki_patch3_r3_sp__
.align  4
bunki_yield:
__bunki_patch3_r3_sp__:
	orr      r3, sp, #(BUNKI_STACK_CONST - 1)
        save_ctx r3, 0x3
        load_ctx r3, 0x7
        mov      pc, lr

.global bunki_ctx_stack_start
.global __bunki_patch4_r0_sp__
.align 4
bunki_ctx_stack_start:
__bunki_patch4_r0_sp__:
	orr      r0, sp, #(BUNKI_STACK_CONST - 1)
	add      r0, r0, #1
        mov      pc, lr

.global bunki_init_ctx
.global __bunki_patch5_r11_sp__
.align 4
bunki_init_ctx:
	mov      r0, r5
	blx      r4
__bunki_patch5_r11_sp__:
	orr      r11, sp, #(BUNKI_STACK_CONST - 1)
	ldr      r5, =bunki_set_ret
	sub      r4, sp, #0x24   
	str	 r4, [r11, #-0x3]
        stm      r4, { r5, r11 }
bunki_done:
	load_ctx r11, 0x7
        mov pc, lr
bunki_set_ret:
	eor      r0, r0
	b        bunki_done

.global bunki_ctx_call
.align 6
bunki_ctx_call:
	mov      r3, r1
	b        bunki_ctx_call_arg3

.global bunki_ctx_call_arg2
.align 4
bunki_ctx_call_arg2:
	mov      r3, r2
	b        bunki_ctx_call_arg3

.global bunki_ctx_call_arg3
.global __bunki_patch6_r4_sp__
.align 4
bunki_ctx_call_arg3:
        push     { r4, r5 }
__bunki_patch6_r4_sp__:
	orr      r4, sp, #(BUNKI_STACK_CONST - 1)
	ldr	 r4, [r4, #-0xb]
	and      r4, r4, #-8
	mov      r5, sp
	mov      sp, r4
        push     { r5, lr }
	blx      r3
        pop      { r5, lr }
	mov      sp, r5
        pop      { r4, r5 }
        mov      pc, lr
