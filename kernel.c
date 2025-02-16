#include "kernel.h"
#include "common.h"

// 获取对应符号起始地址
extern char __bss[], __bss_end[], __stack_top[];
extern char __free_ram[], __free_ram_end[];

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

// 异常处理程序入口点
__attribute__((naked))
__attribute__((aligned(4))) void
kernel_entry(void)
{
	__asm__ __volatile__(
		"csrw sscratch, sp\n"
		"addi sp, sp, -4 * 31\n"
		"sw ra, 4 * 0(sp)\n"
		"sw gp, 4 * 1(sp)\n"
		"sw tp, 4 * 2(sp)\n"
		"sw t0, 4 * 3(sp)\n"
		"sw t1, 4 * 4(sp)\n"
		"sw t2, 4 * 5(sp)\n"
		"sw t3, 4 * 6(sp)\n"
		"sw t4, 4 * 7(sp)\n"
		"sw t5, 4 * 8(sp)\n"
		"sw t6, 4 * 9(sp)\n"
		"sw a0, 4 * 10(sp)\n"
		"sw a1, 4 * 11(sp)\n"
		"sw a2, 4 * 12(sp)\n"
		"sw a3, 4 * 13(sp)\n"
		"sw a4, 4 * 14(sp)\n"
		"sw a5, 4 * 15(sp)\n"
		"sw a6, 4 * 16(sp)\n"
		"sw a7, 4 * 17(sp)\n"
		"sw s0, 4 * 18(sp)\n"
		"sw s1, 4 * 19(sp)\n"
		"sw s2, 4 * 20(sp)\n"
		"sw s3, 4 * 21(sp)\n"
		"sw s4, 4 * 22(sp)\n"
		"sw s5, 4 * 23(sp)\n"
		"sw s6, 4 * 24(sp)\n"
		"sw s7, 4 * 25(sp)\n"
		"sw s8, 4 * 26(sp)\n"
		"sw s9, 4 * 27(sp)\n"
		"sw s10, 4 * 28(sp)\n"
		"sw s11, 4 * 29(sp)\n"

		"csrr a0, sscratch\n"
		"sw a0, 4 * 30(sp)\n"

		"mv a0, sp\n"
		"call handle_trap\n"

		"lw ra, 4 * 0(sp)\n"
		"lw gp, 4 * 1(sp)\n"
		"lw tp, 4 * 2(sp)\n"
		"lw t0, 4 * 3(sp)\n"
		"lw t1, 4 * 4(sp)\n"
		"lw t2, 4 * 5(sp)\n"
		"lw t3, 4 * 6(sp)\n"
		"lw t4, 4 * 7(sp)\n"
		"lw t5, 4 * 8(sp)\n"
		"lw t6, 4 * 9(sp)\n"
		"lw a0, 4 * 10(sp)\n"
		"lw a1, 4 * 11(sp)\n"
		"lw a2, 4 * 12(sp)\n"
		"lw a3, 4 * 13(sp)\n"
		"lw a4, 4 * 14(sp)\n"
		"lw a5, 4 * 15(sp)\n"
		"lw a6, 4 * 16(sp)\n"
		"lw a7, 4 * 17(sp)\n"
		"lw s0, 4 * 18(sp)\n"
		"lw s1, 4 * 19(sp)\n"
		"lw s2, 4 * 20(sp)\n"
		"lw s3, 4 * 21(sp)\n"
		"lw s4, 4 * 22(sp)\n"
		"lw s5, 4 * 23(sp)\n"
		"lw s6, 4 * 24(sp)\n"
		"lw s7, 4 * 25(sp)\n"
		"lw s8, 4 * 26(sp)\n"
		"lw s9, 4 * 27(sp)\n"
		"lw s10, 4 * 28(sp)\n"
		"lw s11, 4 * 29(sp)\n"
		"lw sp, 4 * 30(sp)\n"
		"sret\n");
}

// 读取异常发生原因并触发内核恐慌
void handle_trap(struct trap_frame *f)
{
	uint32_t scause = READ_CSR(scause);
	uint32_t stval = READ_CSR(stval);
	uint32_t user_pc = READ_CSR(sepc);

	PANIC("unexpected trap scause=%x, stval=%x, sepc=%x\n", scause, stval, user_pc);
}

// 动态分配n页内存并返回对应的起始地址 待完善
// 凹凸分配器或线性分配器 (未释放分配的内存)
paddr_t alloc_pages(uint32_t n)
{
	static paddr_t next_paddr = (paddr_t)__free_ram;
	paddr_t paddr = next_paddr;
	next_paddr += n * PAGE_SIZE;

	if (next_paddr > (paddr_t)__free_ram_end)
	{
		PANIC("out of memory!!!\n");
	}
	memset((void *)paddr, 0, n * PAGE_SIZE);
	return paddr;
}

#define PROCS_MAX 8		// 最大进程数
#define PROC_UNUSED 0	// 未运行的进程
#define PROC_RUNNABLE 1 // 可运行的进程

struct process
{
	int pid;			 // 进程ID
	int state;			 // 进程状态 PROC_UNUSED或PROC_UNUSED
	vaddr_t sp;			 // 栈指针
	uint8_t stack[8192]; // 内核栈 8KB
};

__attribute__((naked)) void switch_context(uint32_t *prev_sp, uint32_t *next_sp)
{
	__asm__ __volatile__(
		// 将被调用者寄存器保存到当前进程栈上
		// 为13个4字节寄存器分配栈空间再将对应寄存器保存到该栈空间上
		"addi sp, sp, -4 * 13\n"
		"sw ra, 4 * 0(sp)\n"
		"sw s0, 4 * 1(sp)\n"
		"sw s1, 4 * 2(sp)\n"
		"sw s2, 4 * 3(sp)\n"
		"sw s3, 4 * 4(sp)\n"
		"sw s4, 4 * 5(sp)\n"
		"sw s5, 4 * 6(sp)\n"
		"sw s6, 4 * 7(sp)\n"
		"sw s7, 4 * 8(sp)\n"
		"sw s8, 4 * 9(sp)\n"
		"sw s9, 4 * 10(sp)\n"
		"sw s10, 4 * 11(sp)\n"
		"sw s11, 4 * 12(sp)\n"

		// 切换栈指针
		"sw sp, (a0)\n" // *prev_sp = sp; 保存上一个sp指针
		"lw sp, (a1)\n" // 切换sp操作 切换为当前sp指针

		// 从下一个进程栈中恢复被调用者的寄存器
		"lw ra, 4 * 0(sp)\n"
		"lw s0, 4 * 1(sp)\n"
		"lw s1, 4 * 2(sp)\n"
		"lw s2, 4 * 3(sp)\n"
		"lw s3, 4 * 4(sp)\n"
		"lw s4, 4 * 5(sp)\n"
		"lw s5, 4 * 6(sp)\n"
		"lw s6, 4 * 7(sp)\n"
		"lw s7, 4 * 8(sp)\n"
		"lw s8, 4 * 9(sp)\n"
		"lw s9, 4 * 10(sp)\n"
		"lw s10, 4 * 11(sp)\n"
		"lw s11, 4 * 12(sp)\n"
		"addi sp, sp, 4 * 13\n" // 从栈中弹出13个4字节大小寄存器
		"ret\n");
}

// 所有进程
struct process procs[PROCS_MAX];

struct process *create_process(uint32_t pc)
{
	// 查找未使用的进程
	struct process *proc = NULL;
	int i;
	for (i = 0; i < PROCS_MAX; i++)
	{
		if (procs[i].state == PROC_UNUSED)
		{
			proc = &procs[i];
			break;
		}
	}
	if (!proc)
	{
		PANIC("no free process slots!!!\n");
	}

	// 设置被调用者保存寄存器，这些寄存器将在switch_context第一次上下文切换时恢复
	uint32_t *sp = (uint32_t *)&proc->stack[sizeof(proc->stack)];
	printf("sizeof(proc->stack)=%d\n", sizeof(proc->stack));
	*--sp = 0;			  // s11
	*--sp = 0;			  // s10
	*--sp = 0;			  // s9
	*--sp = 0;			  // s8
	*--sp = 0;			  // s7
	*--sp = 0;			  // s6
	*--sp = 0;			  // s5
	*--sp = 0;			  // s4
	*--sp = 0;			  // s3
	*--sp = 0;			  // s2
	*--sp = 0;			  // s1
	*--sp = 0;			  // s0
	*--sp = (uint32_t)pc; // ra

	// 初始化进程相关字段
	proc->pid = i + 1;
	proc->state = PROC_RUNNABLE;
	proc->sp = (uint32_t)sp;
	return proc;
}

// 上下文切换测试
void delay(void)
{
	for (int i = 0; i < 3000000; i++)
	{
		__asm__ __volatile__("nop");
	}
}

struct process *proc_a;
struct process *proc_b;

void proc_a_entry(void)
{
	printf("starting process A\n");
	while (1)
	{
		printf("process a running.\n");
		switch_context(&proc_a->sp, &proc_b->sp);
		delay();
	}
}

void proc_b_entry(void)
{
	printf("starting process B\n");
	while (1)
	{
		printf("process b running.\n");
		switch_context(&proc_b->sp, &proc_a->sp);
		delay();
	}
}

void kernel_main(void)
{
	// bss段初始化为零
	memset(__bss, 0, (size_t)__bss_end - (size_t)__bss);

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

	// PANIC("booted!");
	// printf("unreachable here!\n");

	// // 在stvec寄存器中注册异常处理程序
	// WRITE_CSR(stvec, (uint32_t)kernel_entry);
	// // 触发非法指令异常的伪指令
	// __asm__ __volatile__("unimp");

	// 内存分配测试
	paddr_t paddr0 = alloc_pages(3);
	paddr_t paddr1 = alloc_pages(1);
	printf("call alloc_pages: paddr0=%x\n", paddr0);
	printf("call alloc_pages: paddr1=%x\n", paddr1);
	// PANIC("kernel booted!!!\n");

	// 进程手动切换测试
	proc_a = create_process((uint32_t)proc_a_entry);
	proc_b = create_process((uint32_t)proc_b_entry);
	proc_a_entry();
	printf("unreachable here!\n");

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