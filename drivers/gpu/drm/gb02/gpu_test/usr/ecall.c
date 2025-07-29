#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <libgen.h>
#include <sys/mman.h>

#define GB02MAC53   0x4AFA0000
#define GB02MAC54   0x4AFA0001
#define GB02MAC56  0x28465793

#define ECALL_PATH "/dev/ecall_tty0"

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

int main(int argc, char *argv[])
{
    int fd, ret, index, cmd;
    shell_tty_call_arg u_args;
    memset(&u_args, sizeof(shell_tty_call_arg), 0);
    if (!strcmp(argv[1], "help")) {
        printf(" \
        \t1. 輸入debug用來調試ecall tool \t eg: ecall debug func_name\n  \
        \t2. 輸入str/str_all調用第一個或者全部參數都是str類型的函數 \t eg： ecall str/str_all func_name arg1 arg2 arg3...\n \
        \t3. 默認爲直接調用函數，可以用16進制也可以使用10進制 \t eg: ecall func_name arg1 arg2 ... \n \
        \t 最大支持6個參數");
        return 0;
    }

    if (!strcmp(argv[1], "debug") && argc > 1) {
        u_args.func_name = argv[2];
        u_args.arg_str1 = NULL;
        u_args.arg1 = GB02MAC56;
    } else if (!strncmp(argv[1], "str", 3) && argc > 3) {
        u_args.func_name = argv[2];
        u_args.arg_str1 = argv[3];
        for (index = 4; index < argc; index++) {
            if (!strcmp(argv[1], "str"))
                u_args.args[index - 4] = strtol(argv[index], NULL, 0);
            if (!strcmp(argv[1], "str_all"))
                u_args.args[index - 4] = (long long)argv[index];
        }   
    } else {
        if (argc < 2) {
            printf("input pamater error");
            return -EINVAL;
        }
        u_args.func_name = argv[1];
        u_args.arg_str1 = NULL;
        for (index = 2; index < argc; index++)
            u_args.args[index - 2] = strtol(argv[index], NULL, 0);
    }

    fd = open(ECALL_PATH, O_RDWR);
    if (!fd) {
        printf("error open\n");
        return -EINVAL;
    }

    ret = ioctl(fd, GB02MAC53);
    u_args.sign_word = ret;

    ret = ioctl(fd, GB02MAC54, u_args);
    printf("ret = %llx\n", (unsigned long long)ret);
    return (long long)ret;
}