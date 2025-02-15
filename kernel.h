#ifndef __KERNEL_H
#define __KERNEL_H

// 返回值结构体
struct sbiret
{
	long error;
	long value;
};

// 内核恐慌 Kernel Panic
#define PANIC(fmt, ...)                                                      \
	do                                                                       \
	{                                                                        \
		printf("PANIC:%s:%d: " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__); \
		while (1)                                                            \
		{                                                                    \
		}                                                                    \
	} while (0)

#endif