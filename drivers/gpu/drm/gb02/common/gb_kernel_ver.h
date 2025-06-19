#ifndef _GENBU_KERNEL_VER_H_
#define _GENBU_KERNEL_VER_H_
#include <asm/uaccess.h>
#include <linux/uaccess.h>

#ifndef VERIFY_READ
#define VERIFY_READ 0
#endif

#ifndef VERIFY_WRITE
#define VERIFY_WRITE 1
#endif

#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 0, 0))
    #if (defined(GB_4_19_90)) && (LINUX_VERSION_CODE == KERNEL_VERSION(4, 19, 90))
        #define gb_access_ok(type, addr, size) access_ok(addr, size)
    #else
        #define gb_access_ok(type, addr, size) access_ok(type, addr, size)  
    #endif
#else
    #define gb_access_ok(type, addr, size) access_ok(addr, size);
#endif

#endif
