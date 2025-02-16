ENTRY(boot)

SECTIONS{
	. = 0x80200000;

	.text :{
		KEEP(*(.text.boot));
		*(.text .text.*);
	}

	.rodata : ALIGN(4){
		*(.rodata .rodata.*);
	}

	.data : ALIGN(4){
		*(.data .data.*);
	}

	.bss : ALIGN(4){
		__bss = .;
		*(.bss .bss.* .sbss .sbss.*);
		__bss_end = .;
	}

	. = ALIGN(4);
	. += 128*1024;	/* 内核栈大小为128KB */
	__stack_top = .;	/* 内核栈顶地址 */

	/* 内存分配内存地址管理 */
	. = ALIGN(4096);	/* 4KB边界对齐 */
	__free_ram = .;		/* 内存分配起始地址 */
	. += 64 * 1024 * 1024;	/* 分配器管理内存大小为64MB */
	__free_ram_end = .;	/* 内存分配结束地址 */
}