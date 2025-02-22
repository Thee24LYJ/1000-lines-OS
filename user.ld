ENTRY(start)

SECTIONS{
	. = 0x1000000;
	
    /* 机器代码 */
    .text :{
        KEEP(*(.text.start));
        *(.text .text.*);
    }

    /* 只读数据 */
	.rodata : ALIGN(4){
		*(.rodata .rodata.*);
	}

    /* 初始化值的数据 */
	.data : ALIGN(4){
		*(.data .data.*);
	}

    /* 运行时自动填充为零的数据 */
	.bss : ALIGN(4){
		*(.bss .bss.* .sbss .sbss.*);

        . = ALIGN(16);
        . += 64 * 1024;     /* 64KB */
		__stack_top = .;   /* 栈顶地址 */ 

        /* 确保可执行文件不会太大 */
        ASSERT(. < 0x1800000, "too large executable!");
	}
}