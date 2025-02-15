来源：[1000 行代码的操作系统](https://operating-system-in-1000-lines.vercel.app/zh/)

# RISC-V
下面是 RISC-V 中一些常见的寄存器：

|  寄存器   | ABI 名称 （别名） |              解释              |
| :-------: | :---------------: | :----------------------------: |
|    pc     |        pc         | 程序计数器（下一条指令的位置） |
|    x0     |       zero        |     硬连线零（始终读为零）     |
|    x1     |        ra         |            返回地址            |
|    x2     |        sp         |             栈指针             |
|  x5 - x7  |      t0 - t2      |           临时寄存器           |
|    x8     |        fp         |            栈帧指针            |
| x10 - x11 |      a0 - a1      |        函数参数/返回值         |
| x12 - x17 |      a2 - a7      |            函数参数            |
| x18 - x27 |     s0 - s11      |    调用期间保存的临时寄存器    |
| x28 - x31 |      t3 - t6      |           临时寄存器           |

内存访问：
程序从内存中读取数据或者往内存中写入数据是通过 lw (load word) 指令和 sw (store word) 指令来实现的

分支指令：
bnez 表示 "branch if not equal to zero"（分支如果不等于 0）。其他常见的分支指令有 beq (branch if equal，分支如果等于)和 blt（分支如果小于）

函数调用：
jal (jump and link，跳转和链接)和 ret (return，返回)指令被用作调用函数和返回

栈：
栈是一个被用于函数调用和局部变量的后进先出（LIFO）的内存空间。它是向下发展的，栈指针 sp 指向栈顶

CPU 模式
CPU 有多种模式，每个有不同的特权。在 RISC-V 中有三种模式：

|  模式  |               概述               |
| :----: | :------------------------------: |
| M-mode |  OpenSBI（即 BIOS）运行的模式。  |
| S-mode | 内核运行的模式，又称“内核模式”。 |
| U-mode | 应用运行的模式，又称“用户模式”。 |

特权指令：
在 CPU 指令中，有一些被称为特权指令的是应用程序（用户模式）不能执行的，本次项目中将使用下列权限指令：

|  操作码和操作数   |                        概述                        |             伪代码             |
| :---------------: | :------------------------------------------------: | :----------------------------: |
|   csrr rd, csr    |                      读取 CSR                      |           rd = csr;            |
|   csrw csr, rs    |                      写入 CSR                      |           csr = rs;            |
| csrrw rd, csr, rs |                 同时读取和写入 CSR                 | tmp = csr; csr = rs; rd = tmp; |
|       sret        | 从 trap 处理程序返回（恢复程序计数器、操作模式等） |
|    sfence.vma     |       清除 TLB(Translation Lookaside Buffer)       |

CSR (Control and Status Register, 控制和状态寄存器) 是一个保存 CPU 设置的寄存器

内联汇编：
```
__asm__ __volatile__("assembly" : output operands : input operands : clobbered registers);
```
输出和输入操作数是用冒号隔开的，每个操作数都是按 约束 (C 表达式) 格式写的。约束被用来指定操作数类型，通常用 =r (寄存器) 表示输出操作数，r 表示输入操作数。

在汇编中可以用 %0、%1、%2 等等（第一个是输出）来访问输出和输入操作数。

# 项目总览
该项目实现功能：

+ 多任务处理（Multitasking）：在进程间切换以允许多个应用程序共享 CPU。
+ 异常处理器（Exception handler）：处理需要操作系统干预的事件，如非法指令。
+ 分页（Paging）：为每个应用程序提供独立的内存地址空间。
+ 系统调用（System calls）：允许应用程序调用内核功能。
+ 设备驱动（Device drivers）：抽象硬件功能，如磁盘读写。
+ 文件系统（File system）：管理磁盘上的文件。
命令行 Shell：供人类使用的用户界面。

未实现的功能：

+ 中断处理（Interrupt handling）：相反，我们将使用轮询方法（定期检查设备上的新数据），也称为忙等待。
+ 定时器处理（Timer processing）：不实现抢占式多任务。我们将使用协作式多任务，即每个进程自愿让出 CPU。
+ 进程间通信（Inter-process communication）：不实现管道、UNIX domain socket 和共享内存等功能。
+ 多处理器支持：仅支持单处理器

项目源代码结构：

```
project-os
├── disk/     - 文件系统内容
├── common.c  - 内核/用户共用库：printf、memset 等
├── common.h  - 内核/用户共用库：结构体和常量的定义
├── kernel.c  - 内核：进程管理、系统调用、设备驱动、文件系统
├── kernel.h  - 内核：结构体和常量的定义
├── kernel.ld - 内核：链接器脚本（内存布局定义）
├── shell.c   - 命令行 shell
├── user.c    - 用户库：系统调用函数
├── user.h    - 用户库：结构体和常量的定义
├── user.ld   - 用户：链接器脚本（内存布局定义）
└── run.sh    - 构建脚本
```

# 引导

在 QEMU 中，默认启动 OpenSBI，执行特定硬件的初始化，然后引导内核。

```
$qemu-system-riscv32 -machine virt -bios default -nographic -serial mon:stdio --no-reboot
```

QEMU 使用各种选项来启动虚拟机。以下是脚本中使用的选项：

+ -machine virt：启动一个 virt 机器。你可以用 -machine '?' 选项查看其他支持的机器。
+ -bios default：使用默认固件（在本例中是 OpenSBI）。
+ -nographic：启动 QEMU 时不使用 GUI 窗口。
+ -serial mon:stdio：将 QEMU 的标准输入/输出连接到虚拟机的串行端口。指定 mon: 允许通过按下 Ctrl+A 然后 C 切换到 QEMU 监视器。
+ --no-reboot：如果虚拟机崩溃，停止模拟器而不重启（对调试有用）。

按 Ctrl+A 然后 C 切换到 QEMU 调试控制台（QEMU 监视器）。你可以在监视器中用 q 命令退出 QEMU

Ctrl+A 除了切换到 QEMU 监视器（C 键）外还有其他功能。例如，Ctrl+A后再按 x 键将立即退出 QEMU。
```
C-a h    打印此帮助
C-a x    退出模拟器
C-a s    将磁盘数据保存回文件（如果使用 -snapshot）
C-a t    切换控制台时间戳
C-a b    发送中断（magic sysrq）
C-a c    在控制台和监视器之间切换
C-a C-a  发送 C-a
```

### 链接器脚本kernel.ld

```
ENTRY(boot)

SECTIONS {
    . = 0x80200000;

    .text :{
        KEEP(*(.text.boot));
        *(.text .text.*);
    }

    .rodata : ALIGN(4) {
        *(.rodata .rodata.*);
    }

    .data : ALIGN(4) {
        *(.data .data.*);
    }

    .bss : ALIGN(4) {
        __bss = .;
        *(.bss .bss.* .sbss .sbss.*);
        __bss_end = .;
    }

    . = ALIGN(4);
    . += 128 * 1024; /* 128KB */
    __stack_top = .;
}
```

以下是链接器脚本的关键点：

+ 内核的入口点是 boot 函数。
+ 基地址是 0x80200000。
+ .text.boot 段总是放在开头。
+ 各段按 .text、.rodata、.data 和 .bss 的顺序放置。
+ 内核栈放在 .bss 段之后，大小为 128KB。
这里提到的 .text、.rodata、.data 和 .bss 段是具有特定角色的数据区域：

|  段名   |               描述               |
| :-----: | :------------------------------: |
|  .text  |       此段包含程序的代码。       |
| .rodata |     此段包含只读的常量数据。     |
|  .data  |      此段包含可读写的数据。      |
|  .bss   | 此段包含初始值为零的可读写数据。 |

让我们仔细看看链接器脚本的语法。首先，ENTRY(boot) 声明 boot 函数是程序的入口点。然后，在 SECTIONS 块中定义每个段的放置位置。

\*(.text .text.\*) 指示将所有文件（*）中的 .text 段和任何以 .text. 开头的段放在该位置。

. 符号代表当前地址。它会随着数据的放置（如 *(.text)）自动增加。语句 . += 128 * 1024 表示"当前地址前进 128KB"。ALIGN(4) 指示确保当前地址调整到 4 字节边界。

最后，__bss = . 将当前地址分配给符号 __bss。在 C 语言中，你可以使用 extern char symbol_name 引用已定义的符号。

### 最小内核

```c
typedef unsigned char uint8_t;
typedef unsigned int uint32_t;
typedef uint32_t size_t;

extern char __bss[], __bss_end[], __stack_top[];

void *memset(void *buf, char c, size_t n) {
    uint8_t *p = (uint8_t *) buf;
    while (n--)
        *p++ = c;
    return buf;
}

void kernel_main(void) {
    memset(__bss, 0, (size_t) __bss_end - (size_t) __bss);

    for (;;);
}

__attribute__((section(".text.boot")))
__attribute__((naked))
void boot(void) {
    __asm__ __volatile__(
        "mv sp, %[stack_top]\n" // 设置栈指针
        "j kernel_main\n"       // 跳转到内核主函数
        :
        : [stack_top] "r" (__stack_top) // 将栈顶地址作为 %[stack_top] 传递
    );
}
```

内核入口点
内核的执行从 boot 函数开始，该函数在链接器脚本中指定为入口点。在此函数中，栈指针（sp）被设置为链接器脚本中定义的栈区域的结束地址。然后，它跳转到 kernel_main 函数。需要注意的是，栈是向零方向增长的，也就是说，使用时它会递减。因此，必须设置栈区域的结束地址（而不是起始地址）。

boot 函数属性
boot 函数有两个特殊属性。__attribute__((naked)) 属性告诉编译器不要在函数体前后生成不必要的代码，比如返回指令。这确保内联汇编代码就是确切的函数体。

boot 函数还有 __attribute__((section(".text.boot"))) 属性，它控制函数在链接器脚本中的放置。由于 OpenSBI 简单地跳转到 0x80200000 而不知道入口点，所以需要将 boot 函数放在 0x80200000。

使用 extern char 获取链接器脚本符号
在文件开始处，使用 extern char 声明了链接器脚本中定义的每个符号。在这里，我们只关心获取符号的地址，所以使用 char 类型并不那么重要。

我们也可以声明为 extern char __bss;，但单独的 __bss 表示 “.bss 段第 0 字节的值” 而不是 “.bss 段的起始地址”。因此，建议添加 [] 以确保 __bss 返回一个地址同时防止任何不小心的错误。

.bss 段初始化
在 kernel_main 函数中，首先使用 memset 函数将 .bss 段初始化为零。虽然某些引导加载程序可能会识别并清零 .bss 段，但我们以防万一手动初始化它。最后，函数进入一个无限循环，内核终止。

# 内核恐慌

```c
#define PANIC(fmt, ...)                                                        \
    do {                                                                       \
        printf("PANIC: %s:%d: " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__);  \
        while (1) {}                                                           \
    } while (0)
```

在这里将其定义为宏。这样做的原因是为了正确显示源文件名（__FILE__）和行号（__LINE__）。如果我们将其定义为函数，__FILE__ 和 __LINE__ 将显示 PANIC 被定义的文件名和行号，而不是它被调用的位置。

这个宏还使用了两个惯用语：

第一个惯用语是 do-while 语句。由于它是 while (0)，这个循环只执行一次。这是定义由多个语句组成的宏的常见方式。简单地用 { ...} 封装可能会在与 if 等语句组合时导致意外的行为（参见这个清晰的例子）。另外，注意每行末尾的反斜杠（\）。虽然宏是在多行上定义的，但在展开时换行符会被忽略。

第二个惯用语是 ##__VA_ARGS__。这是一个用于定义接受可变数量参数的宏的有用编译器扩展（参考：[GCC 文档](https://gcc.gnu.org/onlinedocs/gcc/Variadic-Macros.html)）。当可变参数为空时，## 会删除前面的 ,。这使得即使只有一个参数，如 PANIC("booted!")，编译也能成功