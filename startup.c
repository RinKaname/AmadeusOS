#include <stdint.h> // Standard C header for fixed-width integer types (e.g., uint32_t, uint16_t).
                    // Crucial for precise bit-width interactions with hardware registers.
#include "reg.h"    // Custom header defining hardware register addresses, now updated for LM3S6965evb.

/*
 * --- IMPORTANT NOTE ---
 * This 'startup.c' has been simplified for the LM3S6965evb target.
 * The complex clock initialization functions (like RCC_CR, RCC_CFGR, FLASH_ACR definitions and the
 * rcc_clock_init() function itself) that were specific to STM32 microcontrollers have been removed.
 *
 * For the LM3S6965evb target on QEMU, basic clocking for peripherals like UART0 is often implicitly
 * handled by the emulator or is simple enough to be done directly in the peripheral's initialization
 * function (e.g., uart0_init() in hello.c).
 *
 * The primary roles of this simplified startup.c are now:
 * 1. Copying initialized global data from Flash (ROM) to SRAM (RAM).
 * 2. Zero-filling uninitialized global data in SRAM.
 * 3. Defining the Interrupt Vector Table (which includes the initial Stack Pointer and the Reset Handler).
 * 4. Calling the 'main' function of the OS.
 */


/* main program entry point */
// External declaration of the 'main' function from your kernel (hello.c).
// This signifies that 'main' is the primary entry point for your application
// code after the fundamental startup routines are complete.
extern void main(void);

/*
 * External Declarations (Symbols from Linker Script)
 * These are symbols (names) that are defined within the linker script (`hello.ld`).
 * This C code does not directly define their memory locations but refers to them
 * to manage memory segments.
 */
extern uint32_t _sidata; // Start address for the initialization values of the .data section in Flash (Load Memory Address/LMA).
                         // This is where the initial values of initialized global variables are stored in the program binary.
extern uint32_t _sdata;  // Start address for the .data section in RAM (Virtual Memory Address/VMA).
                         // This is the actual RAM location where the initialized global variables will reside during runtime.
extern uint32_t _edata;  // End address for the .data section in RAM.
                         // Used as a boundary for the data copying loop.
extern uint32_t _sbss;   // Start address for the .bss section in RAM.
                         // This is where uninitialized global variables will reside.
extern uint32_t _ebss;   // End address for the .bss section in RAM.
                         // Used as a boundary for the zero-filling loop.
extern uint32_t _estack; // End address for the stack (typically the highest memory address allocated for the stack).
                         // This value will be loaded into the CPU's Stack Pointer (SP) upon reset.

/*
 * reset_handler: The Reset Handler Function
 * This is the very first C function executed by the CPU after a reset event (e.g., power-on).
 * Its address is crucial and is placed in the Interrupt Vector Table.
 * Its main responsibilities are to set up the C runtime environment before calling main().
 */
void reset_handler(void)
{
    /* Copy the data segment initializers from flash to SRAM */
    // This section copies initialized global variables from Flash memory (ROM),
    // where they are stored as part of the program binary, to SRAM (RAM).
    // Variables initialized in C (like 'static char greet[]' in hello.c)
    // need to be copied to RAM so the CPU can access and potentially modify them during runtime.
    uint32_t *idata_begin = &_sidata; // Pointer to the beginning of initialized data in Flash (ROM).
    uint32_t *data_begin = &_sdata;   // Pointer to the beginning of the .data section in RAM.
    uint32_t *data_end = &_edata;     // Pointer to the end of the .data section in RAM.
    while (data_begin < data_end)     // Loop to copy data word by word (32-bit words).
    {
        *data_begin++ = *idata_begin++; // Copy from Flash to RAM, then advance pointers.
    }

    /* Zero fill the bss segment. */
    // This section fills all bytes in the '.bss' segment with zero.
    // The '.bss' segment contains uninitialized global variables (e.g., `int counter;`).
    // In the program binary stored in Flash, these variables don't consume space,
    // but in RAM, they need to be allocated and initialized to zero as per C standard.
    uint32_t *bss_begin = &_sbss; // Pointer to the beginning of the .bss section in RAM.
    uint32_t *bss_end = &_ebss;   // Pointer to the end of the .bss section in RAM.
    while (bss_begin < bss_end)   // Loop to zero-fill the .bss segment word by word.
    {
        *bss_begin++ = 0; // Write 0 to each location in .bss, then advance pointer.
    }

    // --- Clock system initialization removed for LM3S6965evb simplification ---
    // The complex clock initialization function (rcc_clock_init) and its related
    // bit definitions for STM32 have been removed as they are not compatible
    // with LM3S6965evb. Basic peripheral clocking for UART0 is now handled
    // within the uart0_init() function in hello.c.

    // Call the 'main()' function of your kernel.
    // After all fundamental setup (copy .data, zero .bss) is complete,
    // program control is transferred to your 'main' function, where your OS
    // begins its primary execution.
    main();
}

/*
 * Default Exception/Interrupt Handlers:
 * These functions will be called if an Exception (error) or Interrupt occurs
 * that is not handled specifically elsewhere.
 * For minimal OSes, they often just contain an infinite loop to halt the CPU
 * if an unexpected error occurs.
 */
void nmi_handler(void) // Non-Maskable Interrupt (NMI) handler.
{                      // This is a high-priority interrupt that cannot be ignored.
    while (1);         // Infinite loop: If an NMI occurs, the CPU will halt here.
}

void hardfault_handler(void) // Hard Fault handler.
{                            // This is a fatal error detected by the CPU (e.g., illegal memory access).
    while (1);               // Infinite loop: If a Hard Fault occurs, the CPU will halt here.
}

/*
 * Interrupt Vector Table (ISR Vector Table)
 * This is a crucial table of function addresses for ARM Cortex-M CPUs.
 * When a reset, interrupt, or exception occurs, the CPU consults this table
 * to determine which function (handler) to call.
 *
 * __attribute((section(".isr_vector"))) tells the linker to place this array
 * into a special memory section named ".isr_vector". This section must be at a specific
 * address (typically 0x00000000) for the CPU to find it upon startup.
 */
__attribute((section(".isr_vector")))
uint32_t *isr_vectors[] = {
    (uint32_t *)&_estack,         /* 0x00: Initial Stack Pointer (SP) value. The CPU loads this value into SP upon reset. */
    (uint32_t *)reset_handler,    /* 0x04: Reset Handler. The address of the function called immediately after reset. */
    (uint32_t *)nmi_handler,      /* 0x08: NMI Handler. The address of the function called when an NMI occurs. */
    (uint32_t *)hardfault_handler /* 0x0C: Hard Fault Handler. The address of the function called when a Hard Fault occurs. */
    // Typically, there are many other handlers after this (Memory Management, Bus Fault, SVC, SysTick, etc.),
    // as well as handlers for peripheral interrupts (USART, GPIO, Timers, etc.).
    // For minimal OSes, only the most crucial ones are defined initially.
};

// --- rcc_clock_init function has been removed from here ---
// Its functionality is now handled by the simplified UART initialization in hello.c
// and basic QEMU emulation clocking.
