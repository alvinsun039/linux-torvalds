#include <linux/module.h>
#include <linux/version.h>
#include <linux/printk.h>
#include <linux/string.h>
#include <linux/kallsyms.h>
#include <linux/tty.h>
#include <linux/io.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include "gpu_test/gb_easy_shell.h"
#include "common/gb_common.h"

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 7, 0))
#define KPROBE_LOOKUP 1
unsigned long (*_gb_kallsyms_lookup_name)(const char *name) = NULL;
#include <linux/kprobes.h>
static struct kprobe kp = {
	.symbol_name = "kallsyms_lookup_name",
};
#endif


int GB02FUNC1(void)
{
	int ret = 0;

#ifdef KPROBE_LOOKUP
	ret = register_kprobe(&kp);
	if (ret < 0) {
		gb_printf(KERN_ERR, "%s-%d: register kprobe fail\n",
				__func__, __LINE__);
		return ret;
	}
	_gb_kallsyms_lookup_name = (void *)kp.addr;
	unregister_kprobe(&kp);	
#endif

	return ret;
}

unsigned long GB02FUNC5(const char *name)
{
	#ifdef KPROBE_LOOKUP
		if (_gb_kallsyms_lookup_name) {
			return _gb_kallsyms_lookup_name(name);
		} else {
			gb_printf(KERN_ERR, "%s-%d: find kallsyms fail\n",
				__func__, __LINE__);
			return 0;
		}
	#else
		return kallsyms_lookup_name(name);
	#endif
}

#define GB02MAC51        221
#define GB02MAC52       1
#define GB02MAC53   0x4AFA0000
#define GB02MAC54   0x4AFA0001
#define GB02MAC55(tty_termios) ((GB02MAC51<<24) | C_BAUD(tty_termios))

/*it's ecall an variable, if the first input parameter is this value
the same as the one defined in ecall.c*/
#define GB02MAC56              0x28465793
typedef struct shell_tty_call_arg {
	int sign_word;
	char *func_name;
	char *arg_str1;
	union {
		long long args[6];
		struct {
			long long arg1;
			long long arg2;
			long long arg3;
			long long arg4;
			long long arg5;
			long long arg6;
		};
	};
} shell_tty_call_arg, *p_shell_tty_call_arg;

/* the struct for 32bit userspace ecall*/
typedef struct shell_tty_call_arg32 {
	int sign_word;
	int func_name;		/* to aligned space the same as 32bit userspace,actully it's a userpace pointer */
	char *arg_str1;
	union {
		long long args[6];
		struct {
			long long arg1;
			long long arg2;
			long long arg3;
			long long arg4;
			long long arg5;
			long long arg6;
		};
	};
} shell_tty_call_arg32, *p_shell_tty_call_arg32;

long long dbg_value_for_ecall = 0x12345678;	/*only for testing ecall + variable */
typedef long long (*call_ptr_with_str) (char *arg_str1, long long arg1,
					long long arg2, long long arg3,
					long long arg4, long long arg5,
					long long arg6);
typedef long long (*call_ptr) (long long arg1, long long arg2, long long arg3,
			       long long arg4, long long arg5, long long arg6);

static struct tty_driver *shell_tty_drv;
static struct tty_port shell_tty_port;
static long long GB02FUNC19(char *func_name, char *arg_str1, long long arg1,
			    long long arg2, long long arg3, long long arg4,
			    long long arg5, long long arg6);

/*below functions are used for system debug*/

#define REG_VIR_ADDR_MAP(phyAddr)        ioremap(phyAddr, sizeof(unsigned long))
#define GB02MAC59(virAddr)      iounmap(virAddr)

void GB02FUNC24(void)
{
	long long *t;

	t = &dbg_value_for_ecall;
	gb_printf(KERN_INFO, "t = 0x%pK\n", t);
}

void GB02FUNC27(unsigned long long pAddr, unsigned long long size,
		  unsigned long long nodesize)
{
	void __iomem *virAddr;
	unsigned int i;

	if (((nodesize != sizeof(char)) && (nodesize != sizeof(short))
	     && (nodesize != sizeof(int)) && (nodesize != sizeof(long long)))) {
		gb_printf(KERN_INFO, "dump type should be 1(char),2(short),4(int),8(long long)\n");
		return;
	}

	gb_printf(KERN_INFO, "\n");

	virAddr = ioremap(pAddr, nodesize * size);
	if (!virAddr) {
		gb_printf(KERN_ERR, "virAddr is NULL\n");
		return;
	}

	for (i = 0; i < size; i++) {
		if ((i * nodesize) % 32 == 0) {
			gb_printf(KERN_INFO, "\n[%16llx]: ", pAddr + (i * nodesize));
		}

		switch (nodesize) {
		case sizeof(char):
			gb_printf(KERN_INFO, "%02x ", *((unsigned char *)virAddr + i));
			break;
		case sizeof(short):
			gb_printf(KERN_INFO, "%04x ", *((unsigned short *)virAddr + i));
			break;
		case sizeof(int):
			gb_printf(KERN_INFO, "%08x ", *((unsigned int *)virAddr + i));
			break;
		case sizeof(long long):
			gb_printf(KERN_INFO, "%16llx ", *((unsigned long long *)virAddr + i));
			break;
		default:
			break;
		}
	}

	gb_printf(KERN_INFO, "\n");
	iounmap(virAddr);

}

void lkup(char *func_name)
{
	unsigned long address;

	if (func_name) {
		address = (unsigned long)GB02FUNC5(func_name);
		gb_printf(KERN_INFO, "lk_addr (0x%x)%s \n", (unsigned int)address, func_name);
	} else {
		gb_printf(KERN_ERR, "null func\n");
	}
}

unsigned long long GB02FUNC36(phys_addr_t phy_addr, unsigned int value)
{
	void *vir_addr = NULL;

	vir_addr = ioremap_wc(phy_addr, sizeof(unsigned long));
	if (!vir_addr) {
		gb_printf(KERN_ERR, "vir_addr is NULL\n");
		return 0;
	}

	writel(value, vir_addr);
	iounmap(vir_addr);

	return 0;
}

/***********************************************************
 Function: GB02FUNC40--write an UINT32 value to physical memory
 Input:    the  writed address and data
 return:   void
 see also: write_uint16
 History:
 1.    2012.8.20   Creat
************************************************************/
unsigned long long GB02FUNC40(unsigned int pAddr, unsigned int value)
{
	void __iomem *virAddr = NULL;

	virAddr = REG_VIR_ADDR_MAP(pAddr);
	if (!virAddr) {
		gb_printf(KERN_ERR, "virAddr is NULL\n");
		return 0;
	}

	writel(value, virAddr);
	GB02MAC59(virAddr);
	return 0;
}

/***********************************************************
 Function: GB02FUNC47--write an UINT16 value to physical memory
 Input:    the  writed address and data
 return:   void
 see also: GB02FUNC100
 History:
 1.    2012.8.20   Creat
************************************************************/
unsigned long long GB02FUNC47(unsigned int pAddr, unsigned short value)
{
	void __iomem *virAddr = NULL;

	virAddr = REG_VIR_ADDR_MAP(pAddr);
	if (!virAddr) {
		gb_printf(KERN_ERR, "virAddr is NULL\n");
		return 0;
	}

	writew(value, virAddr);
	GB02MAC59(virAddr);
	return 0;
}

/***********************************************************
 Function: GB02FUNC51--write an UINT8 value to physical memory
 Input:    the  writed address and data
 return:   void
 see also: GB02FUNC89
 History:
 1.    2012.8.20   Creat
************************************************************/
unsigned long long GB02FUNC51(unsigned int pAddr, unsigned char value)
{
	void __iomem *virAddr = NULL;

	virAddr = REG_VIR_ADDR_MAP(pAddr);
	if (!virAddr) {
		gb_printf(KERN_ERR, "virAddr is NULL\n");
		return 0;
	}

	writeb(value, virAddr);
	GB02MAC59(virAddr);
	return 0;
}

unsigned int GB02FUNC54(phys_addr_t phy_addr)
{
	unsigned int value = 0;
	void *vir_addr = NULL;

	vir_addr = ioremap_wc(phy_addr, sizeof(unsigned long));
	if (!vir_addr) {
		gb_printf(KERN_ERR, "vir_addr is NULL\n");
		return 0;
	}

	value = readl(vir_addr);
	iounmap(vir_addr);

	return value;
}

/***********************************************************
 Function: GB02FUNC61 --read an UINT32 value from physical memory
 Input:    the  read address
 return:   the value
 see also: GB02FUNC112
 History:
 1.    2012.8.20   Creat
************************************************************/
unsigned int GB02FUNC61(unsigned int pAddr)
{
	unsigned int value = 0;
	void __iomem *virAddr = NULL;

	virAddr = REG_VIR_ADDR_MAP(pAddr);
	if (!virAddr) {
		gb_printf(KERN_ERR, "virAddr is NULL\n");
		return 0;
	}

	value = readl(virAddr);
	GB02MAC59(virAddr);

	return value;
}

/***********************************************************
 Function: GB02FUNC70 --read an UINT16 value from physical memory
 Input:    the  read address
 return:   the value
 see also: GB02FUNC116
 History:
 1.    2012.8.20   Creat
************************************************************/
unsigned short GB02FUNC70(unsigned int pAddr)
{
	unsigned short value = 0;
	void __iomem *virAddr = NULL;

	virAddr = REG_VIR_ADDR_MAP(pAddr);
	if (!virAddr) {
		gb_printf(KERN_ERR, "virAddr is NULL\n");
		return 0;
	}

	value = readw(virAddr);
	GB02MAC59(virAddr);

	return value;
}

/***********************************************************
 Function: GB02FUNC77 --read an UINT8 value from physical memory
 Input:    the  read address
 return:   the value
 see also: GB02FUNC108
 History:
 1.    2012.8.20   Creat
************************************************************/
unsigned char GB02FUNC77(unsigned int pAddr)
{
	unsigned char value = 0;
	void __iomem *virAddr = NULL;

	virAddr = REG_VIR_ADDR_MAP(pAddr);
	if (!virAddr) {
		gb_printf(KERN_ERR, "virAddr is NULL\n");
		return 0;
	}

	value = readb(virAddr);
	GB02MAC59(virAddr);

	return value;
}

/***********************************************************
 Function: write_uint32--write an UINT32 value to memory
 Input:    the  writed address and data
 return:   void
 see also: write_uint16
 History:
 1.    2012.8.20   Creat
************************************************************/
unsigned long long GB02FUNC81(unsigned long long pAddr, unsigned long long value)
{
	*(volatile unsigned long long *)(pAddr) = value;
	return 0;
}

/***********************************************************
 Function: write_uint32--write an UINT32 value to memory
 Input:    the  writed address and data
 return:   void
 see also: write_uint16
 History:
 1.    2012.8.20   Creat
************************************************************/
unsigned long long GB02FUNC89(unsigned long long pAddr, unsigned int value)
{
	*(volatile unsigned int *)(pAddr) = value;
	return 0;
}

/***********************************************************
 Function: GB02FUNC94--write an UINT16 value to memory
 Input:    the  writed address and data
 return:   void
 see also: GB02FUNC100
 History:
 1.    2012.8.20   Creat
************************************************************/
unsigned long long GB02FUNC94(unsigned long long pAddr, unsigned short value)
{
	*(volatile unsigned short *)(pAddr) = value;
	return 0;
}

/***********************************************************
 Function: GB02FUNC100--write an UINT8 value to memory
 Input:    the  writed address and data
 return:   void
 see also: GB02FUNC89
 History:
 1.    2012.8.20   Creat
************************************************************/
unsigned long long GB02FUNC100(unsigned long long pAddr, unsigned char value)
{
	*(volatile unsigned char *)(pAddr) = value;
	return 0;
}

/***********************************************************
 Function: GB02FUNC108 --read an UINT32 value from memory
 Input:    the  read address
 return:   the value
 see also: GB02FUNC112
 History:
 1.    2012.8.20   Creat
************************************************************/
unsigned long long GB02FUNC103(unsigned long long pAddr)
{
	unsigned long long value = 0;

	value = *(volatile unsigned long long *)(pAddr);

	return value;
}

/***********************************************************
 Function: GB02FUNC108 --read an UINT32 value from memory
 Input:    the  read address
 return:   the value
 see also: GB02FUNC112
 History:
 1.    2012.8.20   Creat
************************************************************/
unsigned int GB02FUNC108(unsigned long long pAddr)
{
	unsigned int value = 0;

	value = *(volatile unsigned int *)(pAddr);
	return value;
}

/***********************************************************
 Function: GB02FUNC112 --read an UINT16 value from memory
 Input:    the  read address
 return:   the value
 see also: GB02FUNC116
 History:
 1.    2012.8.20   Creat
************************************************************/
unsigned short GB02FUNC112(unsigned long long pAddr)
{
	unsigned short value = 0;

	value = *(volatile unsigned short *)(pAddr);
	return value;
}

/***********************************************************
 Function: GB02FUNC116 --read an UINT8 value from memory
 Input:    the  read address
 return:   the value
 see also: GB02FUNC108
 History:
 1.    2012.8.20   Creat
************************************************************/
unsigned char GB02FUNC116(unsigned long long pAddr)
{
	unsigned char value = 0;

	value = *(volatile unsigned char *)(pAddr);
	return value;
}

static int GB02FUNC121(struct tty_struct *tty, struct file *filp)
{
	gb_printf(KERN_INFO, "easy shell open\n");
	return 0;
}

static void GB02FUNC124(struct tty_struct *tty, struct file *filp)
{
	gb_printf(KERN_INFO, "easy shell close\n");
}

static int GB02FUNC127(struct tty_struct *tty, unsigned int cmd,
		       unsigned long arg)
{
	p_shell_tty_call_arg call_arg = NULL;
	shell_tty_call_arg temp_call_arg;
	unsigned long len = 1024;
	long long ret;
	char *func_name_user;
	char *arg_str1_user;

	gb_printf(KERN_DEBUG, "received shell ioctl cmd=0x%02x\n, arg=0x%02lx\n",
	       cmd, arg);

	switch (cmd) {
	case GB02MAC53:
		return GB02MAC55(tty);

	case GB02MAC54:
		if (!arg) {
			gb_printf(KERN_ERR, "%s: arg is null\n", __func__);
			return -EFAULT;
		}

		if (copy_from_user
		    ((void *)&temp_call_arg, (void *)arg,
		     sizeof(shell_tty_call_arg))) {
			gb_printf(KERN_ERR, "%s: copy_from_user fail\n", __func__);
			return -EFAULT;
		}

		func_name_user = temp_call_arg.func_name;
		if (!func_name_user) {
			gb_printf(KERN_ERR, "%s: func_name_user is null\n",
				__FUNCTION__);
			return -EFAULT;
		}

		temp_call_arg.func_name = kmalloc(len, GFP_KERNEL);
		if (temp_call_arg.func_name == NULL) {
			gb_printf(KERN_ERR, "%s: out of memory\n", __func__);
			return -ENOMEM;
		}
		arg_str1_user = temp_call_arg.arg_str1;
		temp_call_arg.arg_str1 = kmalloc(len, GFP_KERNEL);
		if (temp_call_arg.arg_str1 == NULL) {
			gb_printf(KERN_ERR, "%s: out of memory\n", __func__);
			kfree(temp_call_arg.func_name);
			return -ENOMEM;
		}
		ret =
		    strncpy_from_user(temp_call_arg.func_name,
				      func_name_user, len);
		if (ret >= len) {
			gb_printf(KERN_ERR, "%s: strncpy_from_user fail, too long!\n",
			       __func__);
			kfree(temp_call_arg.func_name);
			kfree(temp_call_arg.arg_str1);
			return -ENAMETOOLONG;
		}
		if (!ret) {
			gb_printf(KERN_ERR, "%s: strncpy_from_user fail, no func_name!\n",
			       __func__);
			kfree(temp_call_arg.func_name);
			kfree(temp_call_arg.arg_str1);
			return -ENOENT;
		}
		if (ret < 0) {
			gb_printf(KERN_ERR, "%s: strncpy_from_user fail, can't copy!\n",
			       __func__);
			kfree(temp_call_arg.func_name);
			kfree(temp_call_arg.arg_str1);
			return ret;
		}
		if (arg_str1_user) {
			ret =
			    strncpy_from_user(temp_call_arg.arg_str1,
					      arg_str1_user, len);
			if (ret >= len) {
				gb_printf(KERN_ERR, "%s: strncpy_from_user fail, too long!\n",
				     __func__);
				kfree(temp_call_arg.func_name);
				kfree(temp_call_arg.arg_str1);
				return -ENAMETOOLONG;
			}
			if (ret < 0) {
				gb_printf(KERN_ERR, "%s: strncpy_from_user fail, can't copy!\n",
				     __func__);
				kfree(temp_call_arg.func_name);
				kfree(temp_call_arg.arg_str1);
				return ret;
			}
			if (!ret) {
				kfree(temp_call_arg.arg_str1);
				temp_call_arg.arg_str1 = NULL;
			}
		} else {
			kfree(temp_call_arg.arg_str1);
			temp_call_arg.arg_str1 = NULL;
		}
		call_arg = &temp_call_arg;

		if ((unsigned)(call_arg->sign_word) & ~GB02MAC55(tty)) {
			gb_printf(KERN_ERR, "Unallowed call\n");
			kfree(call_arg->func_name);
			return -EPERM;
		}

		ret = GB02FUNC19(call_arg->func_name, call_arg->arg_str1,
				 call_arg->arg1,
				 call_arg->arg2,
				 call_arg->arg3,
				 call_arg->arg4,
				 call_arg->arg5, call_arg->arg6);

		kfree(call_arg->func_name);
		kfree(call_arg->arg_str1);
		return (int)ret;

	default:
		gb_printf(KERN_ERR, "GB02FUNC127 unknown cmd 0x%x\n", cmd);
		break;
	}

	return -ENOIOCTLCMD;
}

/**
 * GB02FUNC146() - for 32bit aligned userspace ecall as the kernel is 64bit aligned
							we make kernel struct the same aligned as userspace
 * @tty: ecall tty struct
 * @cmd: ecall command
 * @arg: ecall parameter
 * Return ecall command exec result.
 */
static long GB02FUNC146(struct tty_struct *tty, unsigned int cmd,
				unsigned long arg)
{
	p_shell_tty_call_arg32 call_arg = NULL;
	shell_tty_call_arg32 temp_call_arg;
	char *func_name = NULL;
	unsigned long user_func = 0;
	unsigned long len = 1024;
	long ret;

	gb_printf(KERN_DEBUG, "received shell ioctl cmd=0x%02x\n, arg=0x%02lx\n",
	       cmd, arg);

	switch (cmd) {
	case GB02MAC53:
		return GB02MAC55(tty);

	case GB02MAC54:
		if (!arg) {
			gb_printf(KERN_ERR, "%s: arg is null\n", __func__);
			return -EFAULT;
		}
		call_arg = (p_shell_tty_call_arg32) arg;
		if (copy_from_user
		    ((void *)&temp_call_arg, (const void __user *)arg,
		     sizeof(shell_tty_call_arg32))) {
			gb_printf(KERN_ERR, "%s: copy_from_user fail\n", __func__);
			return -EFAULT;
		}
		func_name = kmalloc(len, GFP_KERNEL);
		if (func_name == NULL) {
			gb_printf(KERN_ERR, "%s: out of memory\n", __func__);
			return -ENOMEM;
		}
		user_func = (unsigned long)temp_call_arg.func_name;
		user_func = user_func | 0xFFFFFFFF;
		if (!user_func) {
			gb_printf(KERN_ERR, "%s: user_func is null\n", __func__);
			kfree(func_name);
			return -EFAULT;
		}
		ret =
		    strncpy_from_user(func_name, (const char __user *)user_func,
				      len);
		if (ret >= len) {
			kfree(func_name);
			gb_printf(KERN_ERR, "%s: strncpy_from_user fail, too long!\n",
			       __func__);
			return -ENAMETOOLONG;
		}
		if (!ret) {
			kfree(func_name);
			gb_printf(KERN_ERR, "%s: strncpy_from_user fail, no func_name!\n",
			       __func__);
			return -ENOENT;
		}
		if (ret < 0) {
			kfree(func_name);
			gb_printf(KERN_ERR, "%s: strncpy_from_user fail, can't copy!\n",
			       __func__);
			return ret;
		}

		call_arg = &temp_call_arg;

		if ((unsigned)(call_arg->sign_word) & ~GB02MAC55(tty)) {
			gb_printf(KERN_ERR, "Unallowed call\n");
			kfree(func_name);
			return -EPERM;
		}
		call_arg->arg_str1 = NULL;
		ret = GB02FUNC19(func_name, call_arg->arg_str1,
				 call_arg->arg1,
				 call_arg->arg2,
				 call_arg->arg3,
				 call_arg->arg4,
				 call_arg->arg5, call_arg->arg6);

		kfree(func_name);
		return ret;

	default:
		gb_printf(KERN_ERR, "GB02FUNC127 unknown cmd 0x%x\n", cmd);
		break;
	}

	return -ENOIOCTLCMD;
}

static const struct tty_operations shell_ops = {
	.open = GB02FUNC121,
	.close = GB02FUNC124,
	.ioctl = GB02FUNC127,
	.compat_ioctl = GB02FUNC146,
};

static const struct tty_port_operations shell_port_ops = {
};

int GB02FUNC165(void)
{
	gb_printf(KERN_INFO, "Enter ecall init\n");

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 15, 0)
	shell_tty_drv = alloc_tty_driver(GB02MAC52);
#else
	shell_tty_drv = tty_alloc_driver(GB02MAC52,
			TTY_DRIVER_REAL_RAW);
#endif
	if (!shell_tty_drv) {
		gb_printf(KERN_ERR, "Cannot alloc shell tty driver\n");
		return -1;
	}
	shell_tty_drv->owner = THIS_MODULE;
	shell_tty_drv->driver_name = "ecall_serial";
	shell_tty_drv->name = "ecall_tty";
	shell_tty_drv->major = GB02MAC51;
	shell_tty_drv->minor_start = 0;
	shell_tty_drv->type = TTY_DRIVER_TYPE_SERIAL;
	shell_tty_drv->subtype = SERIAL_TYPE_NORMAL;
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 15, 0)
	shell_tty_drv->flags = TTY_DRIVER_REAL_RAW;
#endif
	shell_tty_drv->init_termios = tty_std_termios;
	shell_tty_drv->init_termios.c_cflag =
	    B921600 | CS8 | CREAD | HUPCL | CLOCAL;

	tty_set_operations(shell_tty_drv, &shell_ops);
	tty_port_init(&shell_tty_port);
	shell_tty_port.ops = &shell_port_ops;
	tty_port_link_device(&shell_tty_port, shell_tty_drv, 0);

	if (tty_register_driver(shell_tty_drv)) {
		gb_printf(KERN_ERR, "Error registering shell tty driver\n");
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 15, 0)
		put_tty_driver(shell_tty_drv);
#else
		tty_driver_kref_put(shell_tty_drv);
#endif
		return -1;
	}

	gb_printf(KERN_INFO, "Finish ecall init\n");

	return 0;
}
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 13, 0)
static long long GB02FUNC19(char *func_name, char *arg_str1, long long arg1,
			    long long arg2, long long arg3, long long arg4,
			    long long arg5, long long arg6)
{
	long long result = -1;
	call_ptr address;
	call_ptr_with_str address_with_str;
	unsigned long long addr_variable = 0;

	if (!func_name) {
		goto call_error_input;
	}

	/*view the value of an variable */
	if (GB02MAC56 == arg1) {
		addr_variable = GB02FUNC5(func_name);
		if (!addr_variable) {
			goto call_no_symbol;
		}

		result = *(long long *)addr_variable;

		gb_printf(KERN_INFO, "variable is %s, addr = 0x%llx, value = 0x%llx\n",
		       func_name, (unsigned long long)addr_variable, result);
	} else {
		gb_printf
		    (KERN_INFO, "input parameter: arg1=%llx,arg2=%llx,arg3=%llx,arg4=%llx,arg5=%llx,arg6=%llx\n",
		     arg1, arg2, arg3, arg4, arg5, arg6);
		if (arg_str1) {
			gb_printf(KERN_INFO, "arg_str1 = %s\n", arg_str1);
			address_with_str =
			    (call_ptr_with_str) GB02FUNC5(func_name);
			if (!address_with_str) {
				goto call_no_symbol;
			}
			result =
			    address_with_str(arg_str1, arg1, arg2, arg3, arg4,
					     arg5, arg6);
		} else {
			address = (call_ptr) GB02FUNC5(func_name);
			if (!address) {
				goto call_no_symbol;
			}
			result = address(arg1, arg2, arg3, arg4, arg5, arg6);
		}

		gb_printf(KERN_INFO, "Call %s return, value = 0x%lx\n", func_name,
		       (unsigned long)result);
	}
	return result;

call_error_input:
	gb_printf(KERN_ERR, "Error input, value = -1\n");

call_no_symbol:
	gb_printf(KERN_ERR, "Invalid function, value = -1\n");
	return -1;
}
#else
static long long GB02FUNC19(char *func_name, char *arg_str1, long long arg1,
			    long long arg2, long long arg3, long long arg4,
			    long long arg5, long long arg6)
{
	return -1;
}
#endif