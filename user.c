#include "user.h"

extern char __stack_top[];

// 用于终止应用程序
__attribute__((noreturn)) void exit(void)
{
    syscall(SYS_EXIT, 0, 0, 0);
    for (;;)
        ;
}

// 系统调用
/**
 * @brief 系统调用实现
 *
 * @param sysno 系统调用号
 * @param arg0 系统调用参数0
 * @param arg1 系统调用参数1
 * @param arg2 系统调用参数2
 * @return int 系统调用后的内核返回值
 */
int syscall(int sysno, int arg0, int arg1, int arg2)
{
    register int a0 __asm__("a0") = arg0;
    register int a1 __asm__("a1") = arg1;
    register int a2 __asm__("a2") = arg2;
    register int a3 __asm__("a3") = sysno;

    __asm__ __volatile__("ecall"
                         : "=r"(a0)
                         : "r"(a0), "r"(a1), "r"(a2), "r"(a3)
                         : "memory");

    return a0;
}

/**
 * @brief 使用系统调用实现字符输出
 *
 * @param ch
 */
void putchar(char ch)
{
    syscall(SYS_PUTCHAR, ch, 0, 0);
}

/**
 * @brief 从键盘接收字符
 *
 * @return int 接收的字符,若没有输入返回-1
 */
int getchar(void)
{
    return syscall(SYS_GETCHAR, 0, 0, 0);
}
/**
 * @brief 文件读取
 *
 * @param filename 待读取文件名
 * @param buf 读取缓冲区
 * @param len 长度
 * @return int 内核返回值
 */
int readfile(const char *filename, char *buf, int len)
{
    return syscall(SYS_READFILE, (int)filename, (int)buf, len);
}

/**
 * @brief 文件写入
 *
 * @param filename 待写入文件名
 * @param buf 写入缓冲区
 * @param len 长度
 * @return int 内核返回值
 */
int writefile(const char *filename, const char *buf, int len)
{
    return syscall(SYS_WRITEFILE, (int)filename, (int)buf, len);
}

/**
 * @brief 关机
 *
 * @return int 内核返回值
 */
int shutdown(void)
{
    return syscall(SYS_SHUTDOWN, 0, 0, 0);
}

__attribute__((section(".text.start")))
__attribute__((naked)) void
start(void)
{
    __asm__ __volatile__(
        "mv sp, %[stack_top]\n"
        "call main\n"
        "call exit\n" ::[stack_top] "r"(__stack_top));
}
