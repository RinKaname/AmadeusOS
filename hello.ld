/*
 * Linker script for Amadeus OS.
 * This script defines how the code and data sections of the program will be
 * arranged in the memory of the target microcontroller (STM32F103-like).
 */

/* ENTRY(reset_handler):
 * This tells the linker that 'reset_handler' is the first entry point for your program
 * after the CPU is reset. When the microcontroller boots, it will look up this address
 * in the Vector Table and begin execution from there.
 * (On ARM Cortex-M, this is typically the second element in the interrupt vector table).
 */
ENTRY(reset_handler)

/* MEMORY: This block defines the physical memory regions available on your target microcontroller.
 * You typically get this information from the microcontroller's datasheet or Reference Manual.
 */
MEMORY
{
	/* FLASH (rx):
	 * - FLASH: Name of the memory region (like a hard drive in a PC, but this is Read-Only Memory/ROM).
	 * This is where the executable code and read-only data are permanently stored.
	 * - (rx): Access attributes for this region:
	 * - r: readable
	 * - x: executable
	 * - ORIGIN = 0x08000000: The base (starting) address of the Flash memory.
	 * This is the physical address where your program will be loaded.
	 * - LENGTH = 128K: The total size of the available Flash memory (128 Kilobytes).
	 */
	FLASH (rx) : ORIGIN = 0x08000000, LENGTH = 128K

	/* RAM (rwx):
	 * - RAM: Name of the memory region (like RAM in a PC, this is Random Access Memory/SRAM).
	 * This is where modifiable data (global variables, stack, heap) is stored during program execution.
	 * - (rwx): Access attributes for this region:
	 * - r: readable
	 * - w: writable
	 * - x: executable (sometimes RAM can be executable for self-modifying code or stage 2 bootloaders, but not always)
	 * - ORIGIN = 0x20000000: The base (starting) address of the RAM (SRAM).
	 * - LENGTH = 40K: The total size of the available RAM memory (40 Kilobytes).
	 */
	RAM (rwx) : ORIGIN = 0x20000000, LENGTH = 40K
}

/* SECTIONS: This block defines how the different parts (sections) of your program
 * will be arranged and placed into the memory regions defined above.
 * Each .c or .s file you compile will produce several sections
 * (e.g., .text, .data, .rodata, .bss).
 */
SECTIONS
{
	/* .text: This section contains the executable code (CPU instructions).
	 */
	.text :
	{
		/* KEEP(*(.isr_vector)):
		 * - .isr_vector: This is a special section that contains the Interrupt Vector Table.
		 * This table is crucial for ARM Cortex-M startup as it holds the address of the reset_handler
		 * and other important exception/interrupt handlers.
		 * - KEEP(*(...)): This instruction tells the linker to "keep" this section.
		 * This means that even if the linker doesn't see a direct reference from other C code
		 * to every entry in .isr_vector, it must not remove it (it must not be optimized out).
		 * This is crucial for the Interrupt Vector Table to not disappear from the final binary.
		 */
		KEEP(*(.isr_vector))

		/* *(.text): Collects all .text sections from all object files (.o).
		 * *(.text.*): Collects all sub-sections of .text (e.g., .text.main, .text.print_str).
		 * This ensures all executable code is included.
		 */
		*(.text)
		*(.text.*)

		/* *(.rodata): Collects all .rodata sections (read-only data).
		 * This includes string literals (like "Welcome to Amadeus OS.\n" if not declared as global variables).
		 * In this linker script, .rodata is placed IMMEDIATELY AFTER the executable code (.text) in Flash.
		 */
		*(.rodata)

		/* _sromdev, _eromdev, _sidata:
		 * These are symbols (variables) defined by the linker.
		 * The '.' (dot) symbol represents the current location counter of the linker.
		 * - _sromdev = .; : Marks the starting address of the data area in ROM (Read-Only Memory)
		 * (as a "ROM device").
		 * - _eromdev = .; : Marks the ending address of the data area in ROM.
		 * - _sidata = .; : This is the LOAD Memory Address (LMA)
		 * for the .data section. This is where the initial values of .data variables
		 * are stored in Flash (ROM) before being copied to RAM.
		 */
		_sromdev = .;
		_eromdev = .;
		_sidata = .; /* This is the crucial point connecting .text to .data. */
	} >FLASH /* The >FLASH symbol means all sections defined within this curly brace
	          * (i.e., .isr_vector, .text, .rodata, and its symbols)
	          * will be PLACED in the FLASH memory region.
	          */

	/* .data: This section contains initialized global variables (variables with an initial value).
	 */
	.data : AT(_sidata) /* AT(_sidata) is the MOST IMPORTANT part here.
	                     * This is the LOAD Memory Address (LMA).
	                     * It tells the linker that the initial values of the variables in the .data section
	                     * will be found at the _sidata address (i.e., in Flash, after .rodata).
	                     * This is their "storage location" in the binary file.
	                     */
	{
		_sdata = .; /* _sdata: The actual starting address (Virtual Memory Address / VMA) of the .data section in RAM.
		             * This is where the .data variables will RESIDE when the program is running.
		             */
		*(.data)    /* Collects all .data sections from all object files. */
		*(.data*)   /* Collects all sub-sections of .data. */
		_edata = .; /* _edata: The actual ending address of the .data section in RAM. */
	} >RAM /* The >RAM symbol means this .data section (after being copied by startup.c)
	        * will RESIDE and run in the RAM memory region.
	        */

	/* .bss: This section contains uninitialized global variables (their value is zero by default).
	 * Example: `int counter;`
	 * This section is not stored in Flash to save space; it's only allocated in RAM
	 * and zero-filled by the `reset_handler` in `startup.c` during startup.
	 */
	.bss :
	{
		_sbss = .; /* _sbss: Starting address of the .bss section in RAM. */
		*(.bss)    /* Collects all .bss sections from all object files. */
		_ebss = .; /* _ebss: Ending address of the .bss section in RAM. */
	} >RAM /* The >RAM symbol means this .bss section will RESIDE and run in the RAM memory region. */

	/* _estack: This symbol marks the top (highest address) of the stack.
	 * The stack grows downwards (from high address to low address) on ARM Cortex-M.
	 * ORIGIN(RAM) is the starting address of RAM, LENGTH(RAM) is the length of RAM.
	 * So, _estack = (RAM Start Address + RAM Length) = Top of RAM Address.
	 * This value will be used by the CPU to initialize the Stack Pointer (SP) upon reset.
	 */
	_estack = ORIGIN(RAM) + LENGTH(RAM);
}
