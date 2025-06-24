        #ifndef __REG_H_
        #define __REG_H_

        #include <stdint.h> // Standard C header for fixed-width integer types.

        // Definisi tipe data dasar untuk register.
        // volatile: Penting untuk memberitahu compiler bahwa nilai di lokasi memori ini bisa berubah kapan saja.
        // uint32_t: Lebar register umumnya 32 bit pada ARM Cortex-M.
        #define __REG_TYPE volatile uint32_t
        #define __REG      __REG_TYPE * // Makro untuk pointer ke register.

        /*
         * Konsep Memory-Mapped Peripherals (MMP):
         * Ini adalah cara CPU berinteraksi dengan peripheral. Setiap peripheral memiliki blok alamat memori unik
         * untuk register-registernya. Membaca/menulis ke alamat ini langsung mengontrol hardware.
         */

        /* ============================================================================
         * Perubahan untuk LM3S6965evb (Stellaris LM3S6965 Microcontroller)
         * Alamat-alamat ini ditemukan di Datasheet LM3S6965 (mis. DS-LM3S6965, Rev. I)
         * ============================================================================
         */

        /* System Control (SYSCTL) Memory Map - Pengganti RCC pada STM32 */
        // SYSCTL mengontrol clock, reset, dan fungsi sistem dasar lainnya untuk LM3S6965.
        #define SYSCTL_BASE         ((__REG_TYPE)0x400FE000) // Alamat dasar untuk modul SYSCTL
        #define SYSCTL_RCGC2        ((__REG)(SYSCTL_BASE + 0x108)) // Run Mode Clock Gating Control Register 2 (untuk GPIO)

        // Definisi Clock Enable untuk GPIO Port A dari RCGC2
        #define SYSCTL_RCGC2_GPIOA  (1 << 0)  // Clock Enable for GPIO Port A (Bit 0)


        // Definisi untuk mengaktifkan clock UART (SYSCTL_RCGCUART)
        // UART0EN adalah bit 0 dari register RCGCUART, bukan RCGC2.
        #define SYSCTL_RCGCUART_BASE ((__REG_TYPE)0x400FE618) // Alamat dasar untuk RCGCUART
        #define SYSCTL_RCGCUART ((__REG)(SYSCTL_RCGCUART_BASE))
        #define SYSCTL_RCGCUART_UART0 (1 << 0) // Clock Enable for UART0 (Bit 0 of RCGCUART)

        // Definisi untuk mengaktifkan clock Timer0 (SYSCTL_RCGC1) - JIKA NANTI DIBUTUHKAN
        // Datasheet page 220, offset 0x104 dari SYSCTL_BASE
        #define SYSCTL_RCGC1_BASE   ((__REG_TYPE)0x400FE104) // Alamat dasar untuk RCGC1
        #define SYSCTL_RCGC1        ((__REG)(SYSCTL_RCGC1_BASE))
        #define SYSCTL_RCGC1_TIMER0 (1 << 0)  // Clock Enable for Timer0 (Bit 0 of RCGC1)


        /* GPIO Memory Map */
        // GPIO (General Purpose Input/Output) mengontrol pin-pin fisik pada mikrokontroler.
        // LM3S6965 memiliki beberapa port GPIO (A, B, C, dst.). UART0 menggunakan PA0 (Rx) dan PA1 (Tx).
        #define GPIOA_BASE          ((__REG_TYPE)0x40004000) // Alamat dasar untuk GPIO Port A.
        #define GPIOA_DEN           ((__REG)(GPIOA_BASE + 0x51C)) // Digital Enable Register
        #define GPIOA_AFSEL         ((__REG)(GPIOA_BASE + 0x420)) // Alternate Function Select Register
        #define GPIOA_PCTL          ((__REG)(GPIOA_BASE + 0x52C)) // Port Control Register (untuk memilih fungsi alternatif pin)

        /* UART0 Memory Map (ARM PrimeCell PL011 UART) */
        // UART0 adalah modul komunikasi serial pertama yang akan kita gunakan.
        // Alamat-alamat dan offset register ini spesifik untuk PL011 UART.
        #define UART0_BASE          ((__REG_TYPE)0x4000C000) // Alamat dasar untuk modul UART0.

        // Register-register utama UART0 dan offset-nya dari alamat dasar.
        #define UART0_DR            ((__REG)(UART0_BASE + 0x000)) // Data Register (tempat data dikirim/diterima)
        #define UART0_RSR_ECR       ((__REG)(UART0_BASE + 0x004)) // Receive Status Register / Error Clear Register
        #define UART0_FR            ((__REG)(UART0_BASE + 0x018)) // Flag Register (berisi status seperti FIFO penuh/kosong)
        #define UART0_IBRD          ((__REG)(UART0_BASE + 0x024)) // Integer Baud Rate Divisor Register (bagian integer dari pembagi baud rate)
        #define UART0_FBRD          ((__REG)(UART0_BASE + 0x028)) // Fractional Baud Rate Divisor Register (bagian pecahan dari pembagi baud rate)
        #define UART0_LCRH          ((__REG)(UART0_BASE + 0x02C)) // Line Control Register, High (mengatur panjang kata, stop bit, parity, FIFO enable)
        #define UART0_CTL           ((__REG)(UART0_BASE + 0x030)) // Control Register (mengaktifkan UART, transmitter, receiver)
        #define UART0_IFLS          ((__REG)(UART0_BASE + 0x034)) // Interrupt FIFO Level Select Register
        #define UART0_IM            ((__REG)(UART0_BASE + 0x038)) // Interrupt Mask Register
        #define UART0_RIS           ((__REG)(UART0_BASE + 0x03C)) // Raw Interrupt Status Register
        #define UART0_MIS           ((__REG)(UART0_BASE + 0x040)) // Masked Interrupt Status Register
        #define UART0_ICR           ((__REG)(UART0_BASE + 0x044)) // Interrupt Clear Register
        #define UART0_DMACR         ((__REG)(UART0_BASE + 0x048)) // DMA Control Register
        #define UART0_CC            ((__REG)(UART0_BASE + 0xFC8)) // Clock Configuration Register (untuk memilih sumber clock UART)

        /* Bit definitions for UART0_FR (Flag Register) */
        #define UART0_FR_TXFF       (1 << 5) // Transmit FIFO Full: Bit 5 (FIFO penuh, tidak bisa kirim lagi)
        #define UART0_FR_RXFE       (1 << 4) // Receive FIFO Empty: Bit 4 (FIFO kosong, tidak ada data diterima)

        /* Bit definitions for UART0_LCRH (Line Control Register, High) */
        #define UART0_LCRH_WLEN_8   (0x3 << 5) // Word Length 8-bit: Bit 5-6 set ke 0b11
        #define UART0_LCRH_FEN      (1 << 4)   // FIFO Enable: Bit 4 (mengaktifkan FIFO)

        /* Bit definitions for UART0_CTL (Control Register) */
        #define UART0_CTL_UARTEN    (1 << 0)   // UART Enable: Bit 0 (mengaktifkan UART)
        #define UART0_CTL_TXE       (1 << 8)   // Transmit Enable: Bit 8 (mengaktifkan transmitter)
        #define UART0_CTL_RXE       (1 << 9)   // Receive Enable: Bit 9 (mengaktifkan receiver)


        /* ============================================================================
         * Definisi Register Timer0 (General-Purpose Timer Module)
         * LM3S6965 memiliki 4 GPTM (Timer0-Timer3). Kita akan gunakan Timer0.
         * Datasheet: Bagian 9 "General-Purpose Timers", Table 9-5 "Timers Register Map" (halaman 345)
         * ============================================================================
         */
        #define TIMER0_BASE          ((__REG_TYPE)0x40030000) // Alamat dasar untuk Timer0

        // Register-register utama Timer0 dan offset-nya dari alamat dasar.
        #define TIMER0_GPTMCFG       ((__REG)(TIMER0_BASE + 0x000)) // GPTM Configuration Register
        #define TIMER0_GPTMTAMR      ((__REG)(TIMER0_BASE + 0x004)) // GPTM TimerA Mode Register
        #define TIMER0_GPTMCTL       ((__REG)(TIMER0_BASE + 0x00C)) // GPTM Control Register
        #define TIMER0_GPTMIMR       ((__REG)(TIMER0_BASE + 0x018)) // GPTM Interrupt Mask Register
        #define TIMER0_GPTMRIS       ((__REG)(TIMER0_BASE + 0x01C)) // GPTM Raw Interrupt Status Register
        #define TIMER0_GPTMICR       ((__REG)(TIMER0_BASE + 0x024)) // GPTM Interrupt Clear Register
        #define TIMER0_GPTMTAILR     ((__REG)(TIMER0_BASE + 0x028)) // GPTM TimerA Interval Load Register
        #define TIMER0_GPTMTAR       ((__REG)(TIMER0_BASE + 0x048)) // GPTM TimerA Register (Current Value)

        /* Bit definitions for Timer0 Registers */

        // GPTM Configuration Register (GPTMCFG)
        #define GPTM_CFG_32BIT_TIMER  0x00000000 // Konfigurasi sebagai timer 32-bit (nilai 0x0)

        // GPTM TimerA Mode Register (GPTMTAMR)
        #define GPTM_TAMR_TAMR_PERIODIC (0x2) // Timer A Mode: Periodic (nilai 0x2)

        // GPTM Control Register (GPTMCTL)
        #define GPTM_CTL_TAEN         (1 << 0)   // Timer A Enable (Bit 0)

        // GPTM Raw Interrupt Status Register (GPTMRIS)
        #define GPTM_RIS_TATORIS      (1 << 0)   // TimerA Time-Out Raw Interrupt Status (Bit 0)

        // GPTM Interrupt Clear Register (GPTMICR)
        #define GPTM_ICR_TATOCINT     (1 << 0)   // TimerA Time-Out Interrupt Clear (Bit 0)

        #endif // Akhir dari include guard
        