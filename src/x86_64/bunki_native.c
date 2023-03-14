#include "bunki.h"
#include "bunki_ctx.h"
#include "bunki_common.h"
#include <string.h>

void bunki_init_ctx(void);
extern uint32_t __bunki_patch0__;
extern uint32_t __bunki_patch1__;
extern uint32_t __bunki_patch2__;
extern uint32_t __bunki_patch3__;
extern uint32_t __bunki_patch4__;

bunki_t bunki_native_finalize_ctx(bunki_t ctx, uintptr_t (*func)(void*), void* arg, uintptr_t stack_end) {
    uintptr_t stk = (uintptr_t)ctx;
    // If stack alignment does not match use a little space to fix that.
    stk &= BUNKI_ALIGN_MASK(ARCH_STK_ALIGN);
    struct stack_ctx_s* new_ctx = (struct stack_ctx_s*)(stk - sizeof(struct stack_ctx_s));
    // init_ctx moves arg where needed for calling argument then jumps to func.
    new_ctx->rip = (uintptr_t)bunki_init_ctx;
    new_ctx->rbx = (uintptr_t)func;
    new_ctx->r15 = (uintptr_t)arg;
    #if !defined(BUNKI_SHARE_FCW_MXCSR)
        #if defined(_WIN32)
            // https://docs.microsoft.com/en-us/cpp/build/x64-calling-convention?view=vs-2019#fpcsr
            new_ctx->both_csrs.fcw = 0x17F;
        #else
            // https://github.com/torvalds/linux/blob/63355b9884b3d1677de6bd1517cd2b8a9bf53978/arch/x86/kernel/fpu/core.c#L483
            new_ctx->both_csrs.fcw = 0x37f;
        #endif

        // https://www.amd.com/system/files/TechDocs/24592.pdf page 113 use default reset value
        // Bits 7-12 should be set
        // MS uses same reset value
        // https://docs.microsoft.com/en-us/cpp/build/x64-calling-convention?view=vs-2019#mxcsr
        new_ctx->both_csrs.mxcsr = 0x1F80;
    #endif
    #ifdef _WIN64
        new_ctx->stk_base    = stk;
        new_ctx->stk_limit   = stack_end;
        new_ctx->stk_dealloc = stack_end;
    #endif
    return new_ctx;
}

#if !defined(BUNKI_STACK_CONST)
unsigned bunki_patch_call_yield(uint32_t stack_size) {
    unsigned ret = 0;
    // Probably could do with less calls since I know some these patch points
    // are within a single page, but if I refactor any of the assembly at least
    // I do not need to update this.
    ret |= bunki_patch_obj_mprotect_exec(&__bunki_patch0__, sizeof(uint32_t), 1);
    ret |= bunki_patch_obj_mprotect_exec(&__bunki_patch1__, sizeof(uint32_t), 1);
    ret |= bunki_patch_obj_mprotect_exec(&__bunki_patch2__, sizeof(uint32_t), 1);
    ret |= bunki_patch_obj_mprotect_exec(&__bunki_patch3__, sizeof(uint32_t), 1);
    ret |= bunki_patch_obj_mprotect_exec(&__bunki_patch4__, sizeof(uint32_t), 1);
    if(ret) {
        goto done;
    }
    stack_size -= 1;
    memcpy(&__bunki_patch0__, &stack_size, sizeof(uint32_t));
    memcpy(&__bunki_patch1__, &stack_size, sizeof(uint32_t));
    memcpy(&__bunki_patch2__, &stack_size, sizeof(uint32_t));
    memcpy(&__bunki_patch3__, &stack_size, sizeof(uint32_t));
    memcpy(&__bunki_patch4__, &stack_size, sizeof(uint32_t));
done:
    ret |= bunki_patch_obj_mprotect_exec(&__bunki_patch0__, sizeof(uint32_t), 0);
    ret |= bunki_patch_obj_mprotect_exec(&__bunki_patch1__, sizeof(uint32_t), 0);
    ret |= bunki_patch_obj_mprotect_exec(&__bunki_patch2__, sizeof(uint32_t), 0);
    ret |= bunki_patch_obj_mprotect_exec(&__bunki_patch3__, sizeof(uint32_t), 0);
    ret |= bunki_patch_obj_mprotect_exec(&__bunki_patch4__, sizeof(uint32_t), 0);
    return ret;
}
#endif
