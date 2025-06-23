#ifndef __REG_H_ // Ini adalah 'include guard'. Jika simbol __REG_H_ belum didefinisikan,
#define __REG_H_ // maka definisikan, dan proses sisa file. Ini mencegah definisi ganda
                 // jika file ini di-include berkali-kali di proyek yang sama.

// Mendefinisikan tipe data dasar untuk representasi register hardware.
// volatile: Kata kunci ini sangat penting dalam pemrograman bare-metal/embedded.
//           Ia memberitahu compiler bahwa nilai di alamat memori yang ditunjuk oleh pointer ini
//           bisa berubah kapan saja di luar kendali program (misalnya, oleh hardware, DMA, atau interrupt).
//           Tanpa 'volatile', compiler mungkin mengoptimasi kode dengan berasumsi nilai tidak berubah
//           dan menyimpan nilai register di cache CPU, sehingga pembacaan/penulisan ke hardware menjadi salah.
// uint32_t: Menentukan bahwa register ini adalah integer tak bertanda (unsigned) dengan lebar 32 bit.
//           Sebagian besar register pada mikrokontroler ARM 32-bit (seperti STM32 yang kode ini targetkan)
//           memiliki lebar 32 bit.
#define __REG_TYPE volatile uint32_t

// Mendefinisikan makro '__REG' yang akan digunakan untuk membuat pointer ke register.
// '__REG_TYPE *' berarti "pointer ke tipe data register (__REG_TYPE)".
// Saat Anda menggunakan `(__REG) (ALAMAT_DASAR + OFFSET)`, itu akan 'meng-casting' alamat
// menjadi pointer yang bisa digunakan untuk membaca atau menulis ke register tersebut.
#define __REG __REG_TYPE *

/*
 * Konsep Memory-Mapped Peripherals (MMP):
 * Ini adalah cara CPU berinteraksi dengan peripheral (seperti GPIO, USART, Timer, dll.).
 * Setiap peripheral memiliki blok alamat memori unik yang dialokasikan khusus untuk register-registernya.
 * Dengan membaca atau menulis ke alamat-alamat ini, program secara langsung mengontrol fungsi hardware.
 * Ini berbeda dengan I/O-mapped I/O yang menggunakan instruksi khusus (seperti IN/OUT di x86).
 * ARM umumnya menggunakan Memory-Mapped Peripherals.
 */


/* RCC Memory Map */
// RCC (Reset and Clock Control) adalah modul inti yang bertanggung jawab untuk:
// - Mengelola frekuensi clock untuk CPU dan semua peripheral.
// - Melakukan reset pada berbagai bagian chip.
// Anda harus "menyalakan" clock untuk peripheral apa pun sebelum Anda bisa menggunakannya.
#define RCC ((__REG_TYPE)0x40021000) // Alamat dasar (base address) memori untuk modul RCC.
                                     // Alamat ini ditemukan di Reference Manual mikrokontroler.

// Definisi register-register spesifik dalam modul RCC, menggunakan offset dari alamat dasar RCC.
// Misalnya, RCC_CR ada di alamat (RCC_BASE_ADDRESS + 0x00).
// Saat Anda menulis `*(RCC_APB2ENR) |= ...`, Anda sedang menulis ke register di alamat ini.
#define RCC_CR        ((__REG)(RCC + 0x00))  // Clock Control Register: Mengontrol osilator utama (HSE, HSI, PLL), dll.
#define RCC_CFGR      ((__REG)(RCC + 0x04))  // Clock Configuration Register: Mengatur pembagian frekuensi clock untuk bus dan peripheral.
#define RCC_CIR       ((__REG)(RCC + 0x08))  // Clock Interrupt Register: Untuk interrupt terkait clock.
#define RCC_APB2RSTR  ((__REG)(RCC + 0x0C))  // APB2 Peripheral Reset Register: Digunakan untuk mereset peripheral yang terhubung ke bus APB2.
#define RCC_APB1RSTR  ((__REG)(RCC + 0x10))  // APB1 Peripheral Reset Register: Digunakan untuk mereset peripheral yang terhubung ke bus APB1.
#define RCC_AHBENR    ((__REG)(RCC + 0x14))  // AHB Peripheral Clock Enable Register: Mengaktifkan clock untuk peripheral di bus AHB.
#define RCC_APB2ENR   ((__REG)(RCC + 0x18))  // APB2 Peripheral Clock Enable Register: Mengaktifkan clock untuk peripheral di bus APB2 (misalnya GPIO, USART1).
                                             // Ini adalah register yang digunakan di main.c untuk mengaktifkan clock GPIOA dan AFIO.
#define RCC_APB1ENR   ((__REG)(RCC + 0x1C))  // APB1 Peripheral Clock Enable Register: Mengaktifkan clock untuk peripheral di bus APB1 (misalnya USART2).
                                             // Ini adalah register yang digunakan di main.c untuk mengaktifkan clock USART2.
#define RCC_BDCR      ((__REG)(RCC + 0x20))  // Backup Domain Control Register: Mengontrol RTC (Real-Time Clock) dan backup registers.
#define RCC_CSR       ((__REG)(RCC + 0x24))  // Control/Status Register: Status reset dan low power reset.

/* Flash Memory Map */
// Modul Flash Memory mengontrol akses ke memori Flash di mana kode program Anda disimpan.
// Ini bisa mengatur parameter seperti latency akses atau mengaktifkan/menonaktifkan prefetch.
#define FLASH     ((__REG_TYPE)0x40022000) // Alamat dasar untuk modul Flash.
#define FLASH_ACR ((__REG)(FLASH + 0x00))  // Access Control Register: Mengatur wait states (latency) untuk akses Flash.

/* GPIO Memory Map */
// GPIO (General Purpose Input/Output) adalah modul yang mengontrol pin-pin fisik pada mikrokontroler.
// Anda bisa mengkonfigurasi pin sebagai input atau output, mengatur nilai high/low, atau mengalihkan fungsinya
// untuk peripheral lain (Alternate Function).
#define GPIOA       ((__REG_TYPE)0x40010800) // Alamat dasar untuk GPIO Port A.
                                             // Mikrokontroler memiliki beberapa port GPIO (A, B, C, dst.),
                                             // masing-masing dengan blok alamat registernya sendiri.
// Definisi register-register spesifik dalam modul GPIOA.
#define GPIOA_CRL   ((__REG)(GPIOA + 0x00))  // Configuration Register Low: Mengkonfigurasi pin PA0-PA7 (mode, kecepatan, tipe).
                                             // Digunakan di main.c untuk mengatur PA2 (USART2_Tx) dan PA3 (USART2_Rx).
#define GPIOA_CRH   ((__REG)(GPIOA + 0x04))  // Configuration Register High: Mengkonfigurasi pin PA8-PA15.
#define GPIOA_IDR   ((__REG)(GPIOA + 0x08))  // Input Data Register: Digunakan untuk membaca nilai (0 atau 1) dari pin yang dikonfigurasi sebagai input.
#define GPIOA_ODR   ((__REG)(GPIOA + 0x0C))  // Output Data Register: Digunakan untuk mengatur nilai (0 atau 1) pin yang dikonfigurasi sebagai output.
                                             // Digunakan di main.c untuk inisialisasi awal.
#define GPIOA_BSRR  ((__REG)(GPIOA + 0x10))  // Bit Set/Reset Register: Cara atomik untuk mengatur (SET) atau mereset (CLEAR) pin output.
                                             // Menggunakan satu operasi tulis untuk kedua aksi ini.
                                             // Digunakan di main.c untuk inisialisasi awal.
#define GPIOA_BRR   ((__REG)(GPIOA + 0x14))  // Bit Reset Register: Cara atomik untuk mereset (CLEAR) pin output saja.
                                             // Digunakan di main.c untuk inisialisasi awal.
#define GPIOA_LCKR  ((__REG)(GPIOA + 0x18))  // Lock Register: Untuk mengunci konfigurasi pin agar tidak bisa diubah sampai reset.

/* USART2 Memory Map */
// USART (Universal Synchronous/Asynchronous Receiver/Transmitter) adalah modul komunikasi serial.
// Digunakan untuk mengirim/menerima data bit demi bit, seringkali untuk debug konsol atau komunikasi dengan sensor/modul eksternal.
// Ada beberapa modul USART/UART di mikrokontroler (USART1, USART2, dll.). Kode ini menggunakan USART2.
#define USART2      ((__REG_TYPE)0x40004400) // Alamat dasar untuk modul USART2.
                                             // Ini adalah lokasi di memori tempat semua register kontrol USART2 berada.
// Definisi register-register spesifik dalam modul USART2.
#define USART2_SR   ((__REG)(USART2 + 0x00)) // Status Register: Berisi berbagai flag status seperti:
                                             //   - TXE (Transmit Data Register Empty): Data siap dikirim.
                                             //   - RXNE (Read Data Register Not Empty): Data baru sudah diterima.
                                             // Ini register yang dibaca di fungsi print_str().
#define USART2_DR   ((__REG)(USART2 + 0x04)) // Data Register: Register tempat Anda menulis data yang ingin dikirim (Transmit Data Register / TDR)
                                             // atau membaca data yang diterima (Receive Data Register / RDR).
                                             // Ini register tempat karakter ditulis di print_str().
#define USART2_BRR  ((__REG)(USART2 + 0x08)) // Baud Rate Register: Mengatur kecepatan transfer data (baud rate) komunikasi serial.
                                             // Diatur berdasarkan frekuensi clock peripheral dan nilai divisor yang dihitung.
#define USART2_CR1  ((__REG)(USART2 + 0x0C)) // Control Register 1: Mengaktifkan/menonaktifkan transmitter (TE) dan receiver (RE),
                                             // mengatur panjang kata (word length), paritas, dan mengaktifkan USART secara keseluruhan (UE).
                                             // Digunakan di main.c untuk mengaktifkan USART2.
#define USART2_CR2  ((__REG)(USART2 + 0x10)) // Control Register 2: Mengatur jumlah stop bits, clock pin, mode LIN, dll.
#define USART2_CR3  ((__REG)(USART2 + 0x14)) // Control Register 3: Mengatur flow control, DMA (Direct Memory Access), mode IRDA, dll.
#define USART2_GTPR ((__REG)(USART2 + 0x18)) // Guard Time and Prescaler Register: Untuk mode synchronous dan Smartcard.

#endif // Akhir dari include guard.