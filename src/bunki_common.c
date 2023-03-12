#include "bunki_common.h"

#if defined(_WIN32)
    #include <windows.h>
    static unsigned get_page_size(void) {
        #if defined(__x86_64__)
            return 4096;
        #else
            SYSTEM_INFO si;
            GetSystemInfo(&si);
            return si.dwPageSize;
        #endif
    }

    unsigned bunki_patch_obj_mprotect_exec(void* ptr, uint8_t obj_size, unsigned write) {
        uintptr_t pg_sz   = get_page_size();
        uintptr_t shift   = 0;
        uintptr_t ptr_beg = (uintptr_t)ptr;
        uintptr_t ptr_end = ptr_beg + obj_size - 1;
        ptr_beg &= -pg_sz;
        ptr_end &= -pg_sz;
        // straddles page boundary
        if(ptr_beg != ptr_end) {
            shift += 1;
        }
        int   prot = write ? PAGE_EXECUTE_READWRITE : PAGE_EXECUTE_READ;
        DWORD old;
        return VirtualProtect((void*)ptr_beg, pg_sz << shift, prot, &old) == 0;
    }

#elif defined (__unix__) || (defined (__APPLE__) && defined (__MACH__))
    #include <unistd.h>
    #include <sys/mman.h>

    static unsigned get_page_size(void) {
        #if defined(__x86_64__)
            return 4096;
        #else
            long size = sysconf(_SC_PAGESIZE);
            return size;
        #endif
    }
    // Currently as written the object must only straddle two pages at most.
    unsigned bunki_patch_obj_mprotect_exec(void* ptr, uint8_t obj_size, unsigned write) {
        uintptr_t pg_sz   = get_page_size();
        uintptr_t shift   = 0;
        uintptr_t ptr_beg = (uintptr_t)ptr;
        uintptr_t ptr_end = ptr_beg + obj_size - 1;
        ptr_beg &= -pg_sz;
        ptr_end &= -pg_sz;
        // straddles page boundary
        if(ptr_beg != ptr_end) {
            shift += 1;
        }
        int prot = PROT_READ | PROT_EXEC;
        prot |= write ? PROT_WRITE : 0;
        return mprotect((void*)ptr_beg, pg_sz << shift, prot) != 0;
    }
#endif
