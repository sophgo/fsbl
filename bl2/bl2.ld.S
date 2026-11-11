/*
 * Copyright (c) 2013-2017, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <platform_def.h>

#ifdef __riscv
OUTPUT_FORMAT("elf64-littleriscv")
OUTPUT_ARCH(riscv)
#else
OUTPUT_FORMAT("elf64-littleaarch64")
OUTPUT_ARCH(aarch64)
#endif
ENTRY(bl2_entrypoint)

/*
 * rom_ddr phobos3 验证布局: BL2 在 VC SRAM (BL2_BASE=VC_RAM_BASE,
 * BL2_SIZE=VC_SRAM_SIZE=0x16C80), 内部三段划分 RO/PAD/RW。
 * RO 限制 0xA200, RW 起始 0xCA80, 与 rom_ddr bl2.ld.S 完全一致。
 */
MEMORY {
    RAM_RO  (rwx): ORIGIN = BL2_BASE,              LENGTH = 0xA200
    RAM_PAD (rwx): ORIGIN = BL2_BASE + 0xA200,     LENGTH = (0xCA80 - 0xA200)
    RAM_RW  (rwx): ORIGIN = BL2_BASE + 0xCA80,     LENGTH = (BL2_SIZE - 0xCA80)
    SRAM    (rwx): ORIGIN = 0x0C0F0000,             LENGTH = 0x10000
}

SECTIONS
{
    . = BL2_BASE;
    ASSERT(. == ALIGN(4096),
           "BL2_BASE address is not aligned on a page boundary.")

    ro . : {
        __RO_START__ = .;
        *bl2_entrypoint.o(.text*)
        *(.vectors)
        *(.text*)
        *(.rodata*)

        __RO_END__ = .;
    } >RAM_RO

    ASSERT(__RO_END__ <= (BL2_BASE + 0xA200), "BL2 RO region has exceeded 0x0-0xA200.")

    .bl2_pad ORIGIN(RAM_PAD) : {
        __BL2_PAD_START__ = .;
        FILL(0x00)
        . += LENGTH(RAM_PAD);
        __BL2_PAD_END__ = .;
    } >RAM_PAD

    /*
     * .data must be placed at a lower address than the stacks if the stack
     * protector is enabled. Alternatively, the .data.stack_protector_canary
     * section can be placed independently of the main .data section.
     */
    .data . : {
        __DATA_START__ = .;
        *(.data*)
        __DATA_END__ = .;
    } >RAM_RW

    stacks (NOLOAD) : {
        . = ALIGN(64);
        __STACKS_START__ = .;
        . += PLATFORM_STACK_SIZE;
        . = ALIGN(64);
        __STACKS_END__ = .;
    } >RAM_RW

    /*
     * The .bss section gets initialised to 0 at runtime.
     * Its base address should be 16-byte aligned for better performance of the
     * zero-initialization code.
     */
    .bss : ALIGN(16) {
        __BSS_START__ = .;
        *(SORT_BY_ALIGNMENT(.bss*))
        *(COMMON)
        . = ALIGN(8);
        __BSS_END__ = .;
    } >RAM_RW

    /*
     * Define a linker symbol to mark end of the RW memory area for this
     * image.
     */
    __BL2_END__ = .;

    __BSS_SIZE__ = SIZEOF(.bss);

    ASSERT(. <= (BL2_BASE + BL2_SIZE), "BL2 image has exceeded its limit.")

    .suspend_sect : {
        . = ALIGN(8);
        __suspend_sect_start = .;
        KEEP(*(.suspend_entry))
        KEEP(*(.suspend_func))
        . = ALIGN(8);
        __suspend_data_start = .;
        KEEP(*(.suspend_data))
        . = ALIGN(8);
        __suspend_sect_end = .;
    } > SRAM

    #include <rom_api_refer.h>
}
