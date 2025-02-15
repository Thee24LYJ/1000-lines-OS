typedef unsigned char uint8_t;
typedef unsigned int uint32_t;
typedef uint32_t size_t;

// 获取对应符号起始地址
extern char __bss[], __bss_end[], __stack_top[];

// TODO:完善内存拷贝函数
void *memset(void *buf, char c, size_t n)
{
	uint8_t *p = (uint8_t *)buf;
	while (n--)
	{
		*p++ = c;
	}
	return buf;
}

void kernel_main(void)
{
	// bss段初始化为零
	memset(__bss, 0, (size_t)__bss_end - (size_t)__bss);

	for (;;)
		;
}

__attribute__((section(".text.boot")))	// 控制boot函数在编译器脚本的位置
__attribute__((naked))	// 告诉编译器不要再函数体前后生成不必要代码
void boot(void)
{
	__asm__ __volatile__(
		"mv sp, %[stack_top]\n" // 设置栈指针
		"j kernel_main\n"		// 跳转到内核主函数
		:
		: [stack_top] "r"(__stack_top)); // 将栈顶地址作为%[stack_top]传递
}