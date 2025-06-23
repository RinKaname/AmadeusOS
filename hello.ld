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
	 */
	FLASH (rx) : ORIGIN = 0x00000000, LENGTH = 128K

	/* RAM (rwx):
	 * Base address for SRAM on LM3S6965 is 0x20000000 (from datasheet).
	 */
	RAM (rwx) : ORIGIN = 0x20000000, LENGTH = 40K
}

SECTIONS
{
	.text :
	{
		KEEP(*(.isr_vector))
		*(.text)
		*(.text.*)
		*(.rodata)
		_sromdev = .;
		_eromdev = .;
		_sidata = .;
	} >FLASH

	.data : AT(_sidata)
	{
		_sdata = .;
		*(.data)
		*(.data*)
		_edata = .;
	} >RAM

	.bss :
	{
		_sbss = .;
		*(.bss)
		_ebss = .;
	} >RAM

	_estack = ORIGIN(RAM) + LENGTH(RAM);
}
