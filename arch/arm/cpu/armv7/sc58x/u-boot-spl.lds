/*
 * (C) Copyright 2020
 * Jian Chen, Analog Devices Software Engineering, <jian.chen@analog.com>
 *
 * (C) Copyright 2020
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

MEMORY { .sdram0 : ORIGIN = CONFIG_SPL_TEXT_BASE,\
		LENGTH = CONFIG_SPL_MAX_SIZE }
MEMORY { .sdram1 : ORIGIN = CONFIG_SPL_BSS_START_ADDR, \
		LENGTH = CONFIG_SPL_BSS_MAX_SIZE }

OUTPUT_FORMAT("elf32-littlearm", "elf32-littlearm", "elf32-littlearm")
OUTPUT_ARCH(arm)
ENTRY(_start)
SECTIONS
{
	.text      :
	{
		__start = .;
		*(.vectors)
		arch/arm/cpu/armv7/start.o	(.text*)
		*(.text*)
	} >.sdram0

	. = ALIGN(4);
	.rodata : { *(SORT_BY_ALIGNMENT(.rodata*)) } >.sdram0

	. = ALIGN(4);
	.data : { *(SORT_BY_ALIGNMENT(.data*)) } >.sdram0

	. = ALIGN(4);
	.u_boot_list : {
		KEEP(*(SORT(.u_boot_list*_i2c_*)));
	} >.sdram0

	. = ALIGN(4);
	__image_copy_end = .;

	.end :
	{
		*(.__end)
	}

	.bss :
	{
		. = ALIGN(4);
		__bss_start = .;
		*(.bss*)
		. = ALIGN(4);
		__bss_end = .;
	} >.sdram1
}
