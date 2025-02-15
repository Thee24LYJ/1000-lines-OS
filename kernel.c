#include "kernel.h"
#include "common.h"

// 获取对应符号起始地址
extern char __bss[], __bss_end[], __stack_top[];

struct sbiret sbi_call(long arg0, long arg1, long arg2, long arg3, long arg4, long arg5, long fid, long eid)
{
	register long a0 __asm__("a0") = arg0;
	register long a1 __asm__("a1") = arg1;
	register long a2 __asm__("a2") = arg2;
	register long a3 __asm__("a3") = arg3;
	register long a4 __asm__("a4") = arg4;
	register long a5 __asm__("a5") = arg5;
	register long a6 __asm__("a6") = fid;
	register long a7 __asm__("a7") = eid;

	// ECALL 作为管理模式和 SEE 之间的控制转移指令
	__asm__ __volatile__("ecall"
						 : "=r"(a0), "=r"(a1)
						 : "r"(a0), "r"(a1), "r"(a2), "r"(a3), "r"(a4), "r"(a5), "r"(a6), "r"(a7)
						 : "memory");
	return (struct sbiret){.error = a0, .value = a1};
}
void putchar(char ch)
{
	sbi_call(ch, 0, 0, 0, 0, 0, 0, 1 /*终端打印*/);
}

void kernel_main(void)
{
	// // bss段初始化为零
	// memset(__bss, 0, (size_t)__bss_end - (size_t)__bss);

	// 调用putchar()输出字符串
	const char *s = "\n\nhello world from putchar()!!!\n\n";
	for (int i = 0; s[i] != '\0'; i++)
	{
		putchar(s[i]);
	}
	// 调用printf()输出字符串
	printf("hello %s from printf()!!!\n", "world");
	printf("1 + 2 = %d, %x\n", 1 + 2, 0x123abc);
	int a = 2, b = 7;
	printf("%d + %d = %d\n", a, b, a + b);

	for (;;)
	{
		__asm__ __volatile__("wfi");
	}
}

__attribute__((section(".text.boot"))) // 控制boot函数在编译器脚本的位置
__attribute__((naked))				   // 告诉编译器不要再函数体前后生成不必要代码
void
boot(void)
{
	__asm__ __volatile__(
		"mv sp, %[stack_top]\n" // 设置栈指针
		"j kernel_main\n"		// 跳转到内核主函数
		:
		: [stack_top] "r"(__stack_top)); // 将栈顶地址作为%[stack_top]传递
}