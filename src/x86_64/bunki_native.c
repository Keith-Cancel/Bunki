#include "bunki.h"
#include "bunki_ctx.h"
#include <string.h>

#include <stdio.h>
#include <errno.h>

extern uint32_t __bunki_patch0__;
extern uint32_t __bunki_patch1__;

#if defined(_WIN32)

#elif defined (__unix__) || (defined (__APPLE__) && defined (__MACH__))
    #include <unistd.h>
    #include <sys/mman.h>
    #define  OBJ_READ  PROT_READ
    #define  OBJ_WRITE PROT_WRITE
    #define  OBJ_EXEC  PROT_EXEC

    static unsigned get_page_size(void) {
        #if defined(__x86_64__)
            return 4096;
        #else
            long size = sysconf(_SC_PAGESIZE);
            return size;
        #endif
    }

    static unsigned patch_obj_mprotect(void* ptr, uint8_t obj_size, int prot) {
        uintptr_t pg_sz   = get_page_size();
        uintptr_t mult    = 1;
        uintptr_t ptr_beg = (uintptr_t)ptr;
        uintptr_t ptr_end = ptr_beg + obj_size - 1;
        ptr_beg &= -pg_sz;
        ptr_end &= -pg_sz;
        // straddles page boundary
        if(ptr_beg != ptr_end) {
            mult += 1;
        }
        int ret = mprotect((void*)ptr_beg, pg_sz * mult, prot);
        if(ret != 0) {
            return 1;
        }
        return 0;
    }

#endif

/*
void bunki_finalize_ctx(bunki_t* ctx, void (*func)(void*), void* arg) {
    uintptr_t stk = (uintptr_t)ctx;
    // If stack alignment does not match use a little space to fix that.
    stk &= ALIGN_MASK(ARCH_STK_ALIGN);
    struct stack_ctx* ctx = (struct stack_ctx*)(stk - sizeof(struct stack_ctx));
    // init_ctx moves arg where needed for calling argument then jumps to func.
    ctx->rip = (uintptr_t)init_ctx;
    ctx->r12 = (uintptr_t)func;
    ctx->r13 = (uintptr_t)arg;
    #if !defined(BUNKI_SHARE_FCW_MXCSR)
    // https://docs.microsoft.com/en-us/cpp/build/x64-calling-convention?view=vs-2019#fpcsr
    ctx->fcw = 0x17F;
    // https://www.amd.com/system/files/TechDocs/24592.pdf page 113 use default reset value
    // Bits 7-12 should be set
    // MS uses same reset value
    // https://docs.microsoft.com/en-us/cpp/build/x64-calling-convention?view=vs-2019#mxcsr
    ctx->mxcsr = 0x1F80;
    #endif
    #ifdef _WIN64
        ctx->stk_base    = stk;
        ctx->stk_limit   = (uintptr_t)stack;
        ctx->stk_dealloc = (uintptr_t)stack;
    #endif
    *rsp = ctx;
}
*/
#if !defined(BUNKI_STACK_CONST)
unsigned bunk_patch_call_yield(uint32_t stack_size) {
    unsigned ret = 0;
    ret |= patch_obj_mprotect(&__bunki_patch0__, sizeof(uint32_t), OBJ_READ | OBJ_WRITE | OBJ_EXEC);
    ret |= patch_obj_mprotect(&__bunki_patch1__, sizeof(uint32_t), OBJ_READ | OBJ_WRITE | OBJ_EXEC);
    if(ret) {
        goto done;
    }
    stack_size -= 1;
    memcpy(&__bunki_patch0__, &stack_size, sizeof(uint32_t));
    memcpy(&__bunki_patch1__, &stack_size, sizeof(uint32_t));
done:
    ret |= patch_obj_mprotect(&__bunki_patch0__, sizeof(uint32_t), OBJ_READ | OBJ_EXEC);
    ret |= patch_obj_mprotect(&__bunki_patch1__, sizeof(uint32_t), OBJ_READ | OBJ_EXEC);
    return ret;

}
#endif