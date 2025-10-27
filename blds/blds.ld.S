/*
 * Copyright (c) 2013-2017, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

//#include <platform_def.h>

#define BLDS_BASE 0x25020000
#define BLDS_SIZE 0x20000
#define PLATFORM_STACK_SIZE 0x2000
#define PLATFORM_LINKER_ARCH            aarch64
#define PLATFORM_LINKER_FORMAT          "elf64-littleaarch64"

OUTPUT_FORMAT(PLATFORM_LINKER_FORMAT)
OUTPUT_ARCH(PLATFORM_LINKER_ARCH)
ENTRY(blds_entry)

MEMORY {
    RAM (rwx): ORIGIN = BLDS_BASE, LENGTH = BLDS_SIZE
}

SECTIONS
{
    . = BLDS_BASE;
    ASSERT(. == ALIGN(4096),
           "BLDS_BASE address is not aligned on a page boundary.")

    ro . : {
        __RO_START__ = .;
        *(.text.entry*)
        *(.vectors)
        *(.text*)
        *(.rodata*)

        __RO_END__ = .;
    } >RAM

    /*
     * .data must be placed at a lower address than the stacks if the stack
     * protector is enabled. Alternatively, the .data.stack_protector_canary
     * section can be placed independently of the main .data section.
     */
    .data . : {
        . = ALIGN(16);
        __DATA_START__ = .;
        *(.data*)
        . = ALIGN(16);
        __DATA_END__ = .;
    } >RAM

    stacks (NOLOAD) : {
        . = ALIGN(64);
        __STACKS_START__ = .;
        . += PLATFORM_STACK_SIZE;
        . = ALIGN(64);
        __STACKS_END__ = .;
    } >RAM

    /*
     * The .bss section gets initialised to 0 at runtime.
     * Its base address should be 16-byte aligned for better performance of the
     * zero-initialization code.
     */
    .bss : ALIGN(16) {
        . = ALIGN(16);
        __BSS_START__ = .;
        *(SORT_BY_ALIGNMENT(.bss*))
        *(COMMON)
        . = ALIGN(16);
        __BSS_END__ = .;
    } >RAM

    /*
     * Define a linker symbol to mark end of the RW memory area for this
     * image.
     */
    __BLDS_END__ = .;

    __BSS_SIZE__ = SIZEOF(.bss);

    ASSERT(. <= (BLDS_BASE + BLDS_SIZE), "BLDS image has exceeded its limit.")

}
