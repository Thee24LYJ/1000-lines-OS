#include "kernel.h"
#include "common.h"

// 获取对应符号起始地址
extern char __bss[], __bss_end[], __stack_top[];
extern char __free_ram[], __free_ram_end[];
extern char __kernel_base[];
extern char _binary_shell_bin_start[], _binary_shell_bin_size[];

struct process *cur_proc;		 // 当前运行进程
struct process *idle_proc;		 // 空闲进程
struct process procs[PROCS_MAX]; // 所有进程

// virtio虚拟设备结构体
struct virtio_virtq *blk_request_vq;
struct virtio_blk_req *blk_req;
paddr_t blk_req_paddr;
unsigned blk_capacity;
// 文件系统结构体
struct file files[FILES_MAX];
uint8_t disk[DISK_MAX_SIZE];

/* 前置函数声明 */
void fs_flush(void);

// 实现OpenSBI调用
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

// 字符输出
void putchar(char ch)
{
	sbi_call(ch, 0, 0, 0, 0, 0, 0, 1 /*终端打印*/);
}

// 从键盘接收字符
long getchar(void)
{
	struct sbiret ret = sbi_call(0, 0, 0, 0, 0, 0, 0, 2);
	return ret.error;
}

// 异常处理程序入口点
__attribute__((naked))
__attribute__((aligned(4))) void
kernel_entry(void)
{
	__asm__ __volatile__(
		// 从sscratch中获取运行进程的内核栈
		"csrrw sp, sscratch, sp\n" // 交换sp和sscratch
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

		// 获取并保存异常发生时的sp指针
		"csrr a0, sscratch\n"
		"sw a0, 4 * 30(sp)\n"

		// 重置内核栈
		"addi a0, sp, 4 * 31\n"
		"csrw sscratch, a0\n"

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

// 上下文切换
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

// CPU调度
void yield(void)
{
	// 寻找可以运行的进程
	struct process *next = idle_proc;
	for (int i = 0; i < PROCS_MAX; i++)
	{
		struct process *proc = &procs[(cur_proc->pid + i) % PROCS_MAX];
		if (proc->state == PROC_RUNNABLE && proc->pid > 0)
		{
			next = proc;
			break;
		}
	}

	// 除当前进程外没有其他可运行进程则返回继续运行当前进程
	if (next == cur_proc)
	{
		return;
	}

	// 在sscratch寄存器中设置当前执行进程的内核栈的初始值
	// 上下文切换时切换进程页表
	__asm__ __volatile__(
		"sfence.vma\n"
		"csrw satp, %[stap]\n"
		"sfence.vma\n"
		"csrw sscratch, %[sscratch]\n"
		:
		: [stap] "r"(SATP_SV32 | ((uint32_t)next->page_table / PAGE_SIZE)),
		  [sscratch] "r"((uint32_t)&next->stack[sizeof(next->stack)]));

	// 上下文切换
	struct process *prev = cur_proc;
	cur_proc = next;
	switch_context(&prev->sp, &next->sp);
}

struct file *fs_lookup(const char *filename)
{
	for (int i = 0; i < FILES_MAX; i++)
	{
		struct file *file = &files[i];
		if (!strcmp(file->name, filename))
		{
			return file;
		}
	}
	return NULL;
}

// 关机命令调用
void shutdown(void)
{
	sbi_call(0, 0, 0, 0, 0, 0, 0, 8);
}

/**
 * @brief 系统调用处理程序
 *
 * @param f 异常时寄存器结构体
 */
void handle_syscall(struct trap_frame *f)
{
	switch (f->a3)
	{
	case SYS_GETCHAR:
		// 重复调用SBI直到输入一个字符
		while (1)
		{
			long ch = getchar();
			if (ch >= 0)
			{
				f->a0 = ch;
				break;
			}
			// 切换到其他进程
			yield();
		}
		break;
	case SYS_PUTCHAR:
		putchar(f->a0);
		break;
	case SYS_EXIT:
		printf("process %d exited\n", cur_proc->pid);
		// 简单起见只标记进程为退出，并未释放进程持有的资源
		cur_proc->state = PROC_EXITED;
		yield();
		PANIC("unreachable");
	case SYS_READFILE:
	case SYS_WRITEFILE:
	{
		const char *filename = (const char *)f->a0;
		char *buf = (char *)f->a1;
		int len = f->a2;
		struct file *file = fs_lookup(filename);
		if (!file)
		{
			printf("file not found: %s\n", filename);
			f->a0 = -1;
			break;
		}

		if (len > (int)sizeof(file->data))
		{
			len = file->size;
		}

		if (f->a3 == SYS_WRITEFILE)
		{
			memcpy(file->data, buf, len);
			file->size = len;
			fs_flush();
		}
		else
		{
			memcpy(buf, file->data, len);
		}

		f->a0 = len;
		break;
	}
	case SYS_SHUTDOWN:
	{
		printf("shutdown!!!\n");
		shutdown();
		break;
	}

	default:
		PANIC("unexpected syscall a3=%x\n", f->a3);
	}
}

// 读取异常发生原因并触发内核恐慌
void handle_trap(struct trap_frame *f)
{
	uint32_t scause = READ_CSR(scause);
	uint32_t stval = READ_CSR(stval);
	uint32_t user_pc = READ_CSR(sepc);

	if (scause == SCAUSE_ECALL)
	{
		handle_syscall(f);
		user_pc += 4;
	}
	else
	{
		PANIC("unexpected trap scause=%x, stval=%x, sepc=%x\n", scause, stval, user_pc);
	}
	WRITE_CSR(sepc, user_pc);
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

// 准备二级页表并填充二级中的页表项
void map_page(uint32_t *table1, uint32_t vaddr, paddr_t paddr, uint32_t flags)
{
	if (!is_aligned(vaddr, PAGE_SIZE))
	{
		PANIC("unaligned vaddr %x", vaddr);
	}
	if (!is_aligned(paddr, PAGE_SIZE))
	{
		PANIC("unaligned paddr %x", paddr);
	}

	uint32_t vpn1 = (vaddr >> 22) & 0x3ff; // 10位vpn[1]
	if ((table1[vpn1] & PAGE_V) == 0)
	{
		// 创建不存在的二级页表
		uint32_t pt_addr = alloc_pages(1);
		table1[vpn1] = ((pt_addr / PAGE_SIZE) << 10) | PAGE_V;
	}

	// 设置二级页表以映射物理页面
	uint32_t vpn0 = (vaddr >> 12) & 0x3ff;
	uint32_t *table0 = (uint32_t *)((table1[vpn1] >> 10) * PAGE_SIZE);
	table0[vpn0] = ((paddr / PAGE_SIZE) << 10) | flags | PAGE_V;
}

__attribute__((naked)) void user_entry(void)
{
	__asm__ __volatile__(
		"csrw sepc, %[sepc]\n"		 // 设置sret跳转地址
		"csrw sstatus, %[sstatus]\n" // 在sstatus寄存器中设置SPIE位
		"sret\n"
		:
		: [sepc] "r"(USER_BASE),
		  [sstatus] "r"(SSTATUS_SPIE | SSTATUS_SUM));
}

struct process *create_process(const void *image, size_t image_size)
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
	*--sp = 0;					  // s11
	*--sp = 0;					  // s10
	*--sp = 0;					  // s9
	*--sp = 0;					  // s8
	*--sp = 0;					  // s7
	*--sp = 0;					  // s6
	*--sp = 0;					  // s5
	*--sp = 0;					  // s4
	*--sp = 0;					  // s3
	*--sp = 0;					  // s2
	*--sp = 0;					  // s1
	*--sp = 0;					  // s0
	*--sp = (uint32_t)user_entry; // ra

	// 映射内核页面
	// 确保内核始终可以访问静态分配区域和由alloc_pages管理的动态分配区域
	uint32_t *page_table = (uint32_t *)alloc_pages(1);
	for (paddr_t paddr = (paddr_t)__kernel_base; paddr < (paddr_t)__free_ram_end; paddr += PAGE_SIZE)
	{
		map_page(page_table, paddr, paddr, PAGE_R | PAGE_W | PAGE_X);
	}
	// 映射virtio-blk MMIO区域到页表中，以便内核访问MMIO寄存器
	map_page(page_table, VIRTIO_BLK_PADDR, VIRTIO_BLK_PADDR, PAGE_R | PAGE_W);

	// 映射用户页面
	for (uint32_t off = 0; off < image_size; off += PAGE_SIZE)
	{
		paddr_t page = alloc_pages(1);

		// 复制的数据小于页面大小的情况
		size_t remaining = image_size - off;
		size_t copy_size = (PAGE_SIZE <= remaining ? PAGE_SIZE : remaining);

		// 填充并映射页面
		memcpy((void *)page, image + off, copy_size); // 逐页复制进程数据
		map_page(page_table, USER_BASE + off, page, PAGE_U | PAGE_R | PAGE_W | PAGE_X);
	}

	// 初始化进程相关字段
	proc->pid = i + 1;
	proc->state = PROC_RUNNABLE;
	proc->sp = (uint32_t)sp;
	proc->page_table = page_table;
	return proc;
}

// 上下文切换测试
struct process *proc_a;
struct process *proc_b;

void proc_a_entry(void)
{
	printf("starting process A\n");
	while (1)
	{
		// printf("process a running.\n");
		yield();
	}
}

void proc_b_entry(void)
{
	printf("starting process B\n");
	while (1)
	{
		// printf("process b running.\n");
		yield();
	}
}
/* 访问MMIO寄存器的相关函数 */
uint32_t virtio_reg_read32(unsigned offset)
{
	return *((volatile uint32_t *)(VIRTIO_BLK_PADDR + offset));
}

uint32_t virtio_reg_read64(unsigned offset)
{
	return *((volatile uint64_t *)(VIRTIO_BLK_PADDR + offset));
}

void virtio_reg_write32(unsigned offset, uint32_t value)
{
	*((volatile uint32_t *)(VIRTIO_BLK_PADDR + offset)) = value;
}

void virtio_reg_featch_and_or32(unsigned offset, uint32_t value)
{
	virtio_reg_write32(offset, virtio_reg_read32(offset) | value);
}

// Virtqueue初始化
// 为virtqueue分配内存区域并告诉设备其物理地址，设备将使用这个内存区域来读写请求
struct virtio_virtq *virtq_init(unsigned index)
{
	// 为virtqueue分配一个区域
	paddr_t virtq_paddr = alloc_pages(align_up(sizeof(struct virtio_virtq), PAGE_SIZE) / PAGE_SIZE);
	struct virtio_virtq *vq = (struct virtio_virtq *)virtq_paddr;
	vq->queue_index = index;
	vq->used_index = (volatile uint16_t *)&vq->used.index;

	// 通过写入队列的索引(第一个队列为 0)到 QueueSel 来选择队列
	virtio_reg_write32(VIRTIO_REG_QUEUE_SEL, index);
	// 通过写入大小到 QueueNum 来通知设备队列大小
	virtio_reg_write32(VIRTIO_REG_QUEUE_NUM, VIRTQ_ENTRY_NUM);
	// 通过写入其字节值到 QueueAlign 来通知设备已使用的对齐方式
	virtio_reg_write32(VIRTIO_REG_QUEUE_ALIGN, 0);
	// 将队列第一页的物理编号写入 QueuePFN 寄存器
	virtio_reg_write32(VIRTIO_REG_QUEUE_PFN, virtq_paddr);
	return vq;
}

// Virtio设备初始化
void virtio_blk_init(void)
{
	if (virtio_reg_read32(VIRTIO_REG_MAGIC) != 0x74726976)
	{
		PANIC("virtio: invalid magic value");
	}
	if (virtio_reg_read32(VIRTIO_REG_VERSION) != 1)
	{
		PANIC("virtio: invalid version");
	}
	if (virtio_reg_read32(VIRTIO_REG_DEVICE_ID) != VIRTIO_DEVICE_BLK)
	{
		PANIC("virtio: invalid device id");
	}

	// 重置设备
	virtio_reg_write32(VIRTIO_REG_DEVICE_STATUS, 0);
	// 设置ACKNOWLEDGE状态位
	virtio_reg_featch_and_or32(VIRTIO_REG_DEVICE_STATUS, VIRTIO_STATUS_ACK);
	// 设置DRIVER状态位
	virtio_reg_featch_and_or32(VIRTIO_REG_DEVICE_STATUS, VIRTIO_STATUS_DRIVER);
	// 设置FEATURES_OK状态位
	virtio_reg_featch_and_or32(VIRTIO_REG_DEVICE_STATUS, VIRTIO_STATUS_FEAT_OK);
	// 执行设备特定设置
	blk_request_vq = virtq_init(0);
	// 设置DRIVER_OK状态位
	virtio_reg_write32(VIRTIO_REG_DEVICE_STATUS, VIRTIO_STATUS_DRIVER_OK);

	// 获取磁盘容量
	blk_capacity = virtio_reg_read64(VIRTIO_REG_DEVICE_CONFIG + 0) * SECTOR_SIZE;
	printf("virtio-blk: capcity is %d bytes\n", blk_capacity);

	// 分配一个区域存储对设备的请求
	blk_req_paddr = alloc_pages(align_up(sizeof(*blk_req), PAGE_SIZE) / PAGE_SIZE);
	blk_req = (struct virtio_blk_req *)blk_req_paddr;
}

// 通知设备有新的请求 desc_index是新请求头描述符的索引
void virtq_kick(struct virtio_virtq *vq, int desc_index)
{
	vq->avail.ring[vq->avail.index % VIRTQ_ENTRY_NUM] = desc_index;
	vq->avail.index++;
	__sync_synchronize();
	virtio_reg_write32(VIRTIO_REG_QUEUE_NOTIFY, vq->queue_index);
	vq->last_used_index++;
}

// 返回是否有请求正在被设备处理
bool virtq_is_busy(struct virtio_virtq *vq)
{
	return vq->last_used_index != *vq->used_index;
}

// 从Virtio-blk设备读取/写入
void read_write_disk(void *buf, unsigned sector, int is_write)
{
	if (sector >= blk_capacity / SECTOR_SIZE)
	{
		printf("virtio: tried to read/write sector=%d, but capacity is %d\n", sector, blk_capacity / SECTOR_SIZE);
		return;
	}

	// 根据Virtio-blk规范构造请求
	blk_req->sector = sector;
	blk_req->type = is_write ? VIRTIO_BLK_T_OUT : VIRTIO_BLK_T_IN;
	if (is_write)
	{
		memcpy(blk_req->data, buf, SECTOR_SIZE);
	}

	// 构造virtqueue描述符
	struct virtio_virtq *vq = blk_request_vq;
	vq->descs[0].addr = blk_req_paddr;
	vq->descs[0].len = sizeof(uint32_t) * 2 + sizeof(uint64_t);
	vq->descs[0].flags = VIRTIO_DESC_F_NEXT;
	vq->descs[0].next = 1;

	vq->descs[1].addr = blk_req_paddr + offsetof(struct virtio_blk_req, data);
	vq->descs[1].len = SECTOR_SIZE;
	vq->descs[1].flags = VIRTIO_DESC_F_NEXT | (is_write ? 0 : VIRTIO_DESC_F_WRITE);
	vq->descs[1].next = 2;

	vq->descs[2].addr = blk_req_paddr + offsetof(struct virtio_blk_req, status);
	vq->descs[2].len = sizeof(uint8_t);
	vq->descs[2].flags = VIRTIO_DESC_F_WRITE;

	// 通知设备有新请求
	virtq_kick(vq, 0);

	// 等待设备处理完成
	while (virtq_is_busy(vq))
		;

	// virtio-blk若返回非零则表示出错
	if (blk_req->status != 0)
	{
		printf("virtio: warn: failed to read/write sector=%d status=%d\n", sector, blk_req->status);
		return;
	}

	// 对于读操作将数据复制到缓冲区
	if (!is_write)
	{
		memcpy(buf, blk_req->data, SECTOR_SIZE);
	}
}

// 八进制字符串转十进制整数
int oct2int(char *oct, int len)
{
	int dec = 0;
	for (int i = 0; i < len; i++)
	{
		if (oct[i] < '0' || oct[i] > '7')
		{
			break;
		}
		dec = dec * 8 + (oct[i] - '0');
	}
	return dec;
}

// 文件系统初始化
void fs_init(void)
{
	for (unsigned sector = 0; sector < sizeof(disk) / SECTOR_SIZE; sector++)
	{
		// 加载磁盘镜像到临时缓冲区disk数组
		read_write_disk(&disk[sector * SECTOR_SIZE], sector, false);
	}
	unsigned off = 0;
	for (int i = 0; i < FILES_MAX; i++)
	{
		struct tar_header *header = (struct tar_header *)&disk[off];
		if (header->name[0] == '\0')
		{
			break;
		}
		if (strcmp(header->magic, "ustar") != 0)
		{
			PANIC("invalid tar header: magic=\"%s\"", header->magic);
		}
		int filesz = oct2int(header->size, sizeof(header->size));
		struct file *file = &files[i];
		file->in_use = true;
		strcpy(file->name, header->name);
		memcpy(file->data, header->data, filesz);
		file->size = filesz;
		printf("file: %s, size=%d\n", file->name, file->size);

		off += align_up(sizeof(struct tar_header) + filesz, SECTOR_SIZE);
	}
}

void fs_flush(void)
{
	memset(disk, 0, sizeof(disk));
	unsigned off = 0;
	for (int file_i = 0; file_i < FILES_MAX; file_i++)
	{
		struct file *file = &files[file_i];
		if (!file->in_use)
		{
			continue;
		}

		struct tar_header *header = (struct tar_header *)&disk[off];
		memset(header, 0, sizeof(*header));
		strcpy(header->name, file->name);
		strcpy(header->mode, "000644");
		strcpy(header->magic, "ustar");
		strcpy(header->version, "00");
		header->type = '0';

		// 将文件大小转换为八进制字符串
		int filesz = file->size;
		for (int i = sizeof(header->size); i > 0; i--)
		{
			header->size[i - 1] = (filesz % 8) + '0';
			filesz /= 8;
		}

		// 计算校验和
		int checksum = ' ' * sizeof(header->checksum);
		for (unsigned i = 0; i < sizeof(struct tar_header); i++)
		{
			checksum += (unsigned char)disk[off + i];
		}
		for (int i = 5; i >= 0; i--)
		{
			header->checksum[i] = (checksum % 8) + '0';
			checksum /= 8;
		}

		// 复制文件数据
		memcpy(header->data, file->data, file->size);
		off += align_up(sizeof(struct tar_header) + file->size, SECTOR_SIZE);
	}

	// 将disk缓冲区写入virtio-blk
	for (unsigned sector = 0; sector < sizeof(disk) / SECTOR_SIZE; sector++)
	{
		read_write_disk(&disk[sector * SECTOR_SIZE], sector, true);
	}
	printf("wrote %d bytes to disk\n", sizeof(disk));
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

	// 在stvec寄存器中注册异常处理程序
	WRITE_CSR(stvec, (uint32_t)kernel_entry);
	// // 触发非法指令异常的伪指令
	// __asm__ __volatile__("unimp");
	// __asm__ __volatile__(
	// 	"li sp, 0xdeadbeef\n" // 将无效地址设置给sp
	// 	"unimp"				  // 触发异常
	// );
	virtio_blk_init();
	fs_init();

	char buf[SECTOR_SIZE];
	read_write_disk(buf, 0, false);
	printf("first sector: %s\n", buf);

	strcpy(buf, "hello from kernel!!!\n");
	read_write_disk(buf, 0, true);

	// 初始化空闲进程 ID=-1
	idle_proc = create_process(NULL, 0);
	idle_proc->pid = -1;
	cur_proc = idle_proc;

	// 内存分配测试
	paddr_t paddr0 = alloc_pages(3);
	paddr_t paddr1 = alloc_pages(1);
	printf("call alloc_pages: paddr0=%x\n", paddr0);
	printf("call alloc_pages: paddr1=%x\n", paddr1);
	// PANIC("kernel booted!!!\n");

	// // 调度器进程切换测试
	// proc_a = create_process((uint32_t)proc_a_entry);
	// proc_b = create_process((uint32_t)proc_b_entry);

	create_process(_binary_shell_bin_start, (size_t)_binary_shell_bin_size);
	yield();
	PANIC("switched to idle process");

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