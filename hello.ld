/*
 * Linker script for Amadeus OS.
 * This script defines how the code and data sections of the program will be
 * arranged in the memory of the target microcontroller (LM3S6965evb).
 */

ENTRY(reset_handler)

MEMORY
{
    /* FLASH (rx):
     * Base address for Flash memory on LM3S6965 is 0x00000000 (from datasheet).
     * This is where the executable code will be loaded.
     * Total Flash is 256KB for LM3S6965 (from datasheet, page 37)
     */
    FLASH (rx) : ORIGIN = 0x00000000, LENGTH = 256K

    /* RAM (rwx):
     * Base address for SRAM on LM3S6965 is 0x20000000 (from datasheet).
     * Total RAM is 64 KB (64K).
     */
    RAM (rwx) : ORIGIN = 0x20000000, LENGTH = 64K
}

SECTIONS
{
    /* .text: This section contains the executable code (CPU instructions).
     * Placed in Flash memory.
     */
    .text :
    {
        KEEP(*(.isr_vector)) /* Keep the Interrupt Vector Table. */
        *(.text)             /* All executable code. */
        *(.text.*)           /* Sub-sections of executable code. */
        *(.rodata)           /* Read-only data (like string literals). */
        *(.rodata.*)         /* Sub-sections of read-only data. */

        _sidata = .; /* The LMA of .data starts right after the current position */
    } >FLASH

    /* .data: This section contains initialized global variables.
     * Their initial values are loaded from Flash but reside in RAM at runtime.
     */
    .data : AT(_sidata) /* AT(_sidata) specifies the LMA of this section. */
    {
        _sdata = .;         /* Virtual Memory Address (VMA) start of .data in RAM. */
        *(.data)            /* All initialized data. */
        *(.data*)           /* Sub-sections of initialized data. */
        _edata = .;         /* VMA end of .data in RAM. */
    } >RAM

    /* .bss: This section contains uninitialized global variables.
     * It does not occupy space in Flash, only in RAM.
     */
    .bss :
    {
        _sbss = .;          /* VMA start of .bss in RAM. */
        *(.bss)             /* All uninitialized data. */
        *(.bss*)            /* All uninitialized data subsections. */
        _ebss = .;          /* VMA end of .bss in RAM. */
    } >RAM

    /* Stack: The stack region (grows downwards from _estack).
     * Its top is defined at the end of RAM.
     */
    _estack = ORIGIN(RAM) + LENGTH(RAM);

    /* Heap: Defines the memory area for dynamic memory allocation (malloc/free).
     * It is placed immediately after .bss.
     */
    .heap (NOLOAD) :
    {
        . = ALIGN(8);
        __heap_start__ = .;
        . = . + 0x4000; /* Reserve 16K for the heap */
        __heap_end__ = .;
    } >RAM

    /* Discard sections that are not needed in the final binary. */
    /DISCARD/ :
    {
        *(.comment)
        *(.ARM.exidx*)
        *(.ARM.extab*)
        *(.ARM.attributes*)
    }
}
