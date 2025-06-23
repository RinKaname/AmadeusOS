#include <stdint.h> // Header standar C untuk tipe data integer dengan ukuran tetap (misal: uint32_t, uint16_t)
#include "reg.h"    // Header kustom yang kemungkinan mendefinisikan alamat-alamat register hardware

/* USART TXE Flag
 * This flag is cleared when data is written to USARTx_DR and
 * set when that data is transferred to the TDR
 */
// Mendefinisikan flag (bendera) untuk status Transmit Data Register Empty (TXE) pada USART.
// Bendera ini akan diset (menjadi 1) ketika data yang ada di register data USART (USARTx_DR)
// sudah berhasil dipindahkan ke Transmit Data Register (TDR) dan siap untuk dikirim.
// Jika flag ini 0, berarti USART masih sibuk mengirim data sebelumnya.
#define USART_FLAG_TXE ((uint16_t)0x0080)

/* greet is a global variable
 * This variable will be load by the loader at LMA and will be
 * copy to VMA by startup.c during startup.
 *
 * Add a global variable greet instead of a string literal
 * because string literal is in .rodata region and is put
 * under the .text region by the linker script
 */
// Mendeklarasikan sebuah array karakter (string) global bernama 'greet'.
// String ini berisi pesan selamat datang untuk Amadeus OS.
// Mengapa 'static char greet[]' daripada string literal langsung (misal: print_str("Welcome..."))?
// Penjelasan di komentar atas kode sudah bagus:
// - String literal (seperti "Welcome...") biasanya ditempatkan di region memori ".rodata" (read-only data).
// - Region ".rodata" ini sering ditempatkan setelah region ".text" (kode program) oleh linker script.
// - Jika bootloader/startup.c Anda hanya memuat atau menyalin region .text dan mengabaikan .rodata,
//   maka string literal tersebut tidak akan tersedia di RAM saat OS berjalan.
// - Dengan mendeklarasikannya sebagai variabel global biasa di luar fungsi (seperti 'static char greet[]'),
//   variabel ini akan ditempatkan di region '.data' (data yang diinisialisasi).
// - Region '.data' biasanya diatur oleh linker script untuk dimuat oleh loader (dari LMA - Load Memory Address)
//   dan disalin ke VMA (Virtual Memory Address) oleh startup.c. Ini memastikan 'greet' tersedia di RAM.
static char greet[] = "Welcome to Amadeus OS. ^__________^\n";

// Fungsi untuk mencetak (menampilkan) sebuah string ke konsol serial (melalui USART)
void print_str(const char *str)
{
    // Loop ini akan berjalan selama karakter yang ditunjuk oleh 'str' bukan '\0' (null terminator),
    // yang menandakan akhir dari sebuah string C.
    while (*str)
    {
        // Loop internal ini menunggu sampai USART siap untuk menerima data baru.
        // *(USART2_SR) adalah membaca status register USART2.
        // & USART_FLAG_TXE melakukan operasi bitwise AND dengan flag TXE.
        // Jika hasilnya non-zero (yaitu, flag TXE set/1), berarti USART sudah siap.
        while (!(*(USART2_SR) & USART_FLAG_TXE))
            ; // Loop kosong, hanya menunggu
        
        // Mengirimkan karakter saat ini ke USART2.
        // *(USART2_DR) adalah menulis ke data register USART2.
        // (*str & 0xFF) memastikan hanya 8 bit terbawah dari karakter yang dikirim (karakter ASCII).
        *(USART2_DR) = (*str & 0xFF);
        
        // Pindah ke karakter berikutnya dalam string.
        str++;
    }
}

// Fungsi utama (main function) dari OS minimal Anda
void main(void)
{
    // Mengaktifkan clock untuk peripheral yang diperlukan.
    // Mikrokontroler ARM (seperti STM32) memiliki unit kontrol clock (RCC)
    // yang mengelola daya ke berbagai peripheral untuk menghemat energi.
    // Peripheral harus diaktifkan (diberi clock) sebelum bisa digunakan.

    // Mengaktifkan clock untuk:
    // Bit 0x00000001 (GPIOAEN): GPIO Port A. (Untuk pin USART2 Tx/Rx)
    // Bit 0x00000004 (AFIOEN): Alternate Function I/O. (Untuk mengkonfigurasi pin GPIO sebagai fungsi alternatif USART)
    *(RCC_APB2ENR) |= (uint32_t)(0x00000001 | 0x00000004); // RCC_APB2ENR adalah register kontrol clock untuk APB2 bus
    
    // Mengaktifkan clock untuk:
    // Bit 0x00020000 (USART2EN): USART2 (Modul komunikasi serial USART2)
    *(RCC_APB1ENR) |= (uint32_t)(0x00020000); // RCC_APB1ENR adalah register kontrol clock untuk APB1 bus

    /* USART2 Configuration, Rx->PA3, Tx->PA2 */
    // Mengkonfigurasi pin GPIO (General Purpose Input/Output) untuk berfungsi sebagai USART2.
    // GPIOA_CRL adalah register konfigurasi GPIO untuk pin-pin PA0-PA7.
    // PA2 akan menjadi USART2_Tx (Transmit) dan PA3 akan menjadi USART2_Rx (Receive).
    // Konfigurasi ini biasanya melibatkan mode (input/output/alternate function) dan tipe (push-pull/open-drain/analog).
    // Nilai 0x00004B00 ini sangat spesifik untuk konfigurasi GPIO tertentu pada mikrokontroler STM32.
    // '4' untuk input/output mode, 'B' untuk alternate function output, '0' untuk input/output.
    *(GPIOA_CRL) = 0x00004B00;
    
    // GPIOA_CRH adalah register konfigurasi GPIO untuk pin-pin PA8-PA15.
    // Nilai 0x44444444 ini kemungkinan mengatur semua pin sisanya sebagai input/output default,
    // atau sekadar nilai default yang tidak relevan dengan USART2.
    *(GPIOA_CRH) = 0x44444444;
    
    // Mengatur Output Data Register (ODR) untuk GPIO Port A ke 0.
    // Ini memastikan semua pin output GPIO Port A dalam keadaan low (0) secara default.
    *(GPIOA_ODR) = 0x00000000;
    
    // Bit Set Reset Register (BSRR) untuk GPIO Port A. Digunakan untuk mengatur atau mereset pin GPIO.
    // Mengaturnya ke 0x00000000 berarti tidak ada pin yang di-set atau di-reset secara paksa.
    *(GPIOA_BSRR) = 0x00000000;
    
    // Bit Reset Register (BRR) untuk GPIO Port A. Digunakan untuk mereset pin GPIO.
    // Mengaturnya ke 0x00000000 berarti tidak ada pin yang di-reset secara paksa.
    *(GPIOA_BRR) = 0x00000000;

    // Bagian ini mengkonfigurasi modul USART2 itu sendiri.
    // Ini adalah register-register kontrol utama USART2.
    // Nilai-nilai hex ini sangat spesifik dan penting untuk fungsi USART.
    // Biasanya meliputi:
    // - Enable Transmitter/Receiver
    // - Word Length (8 bit data, dll.)
    // - Parity
    // - Stop bits
    // - Baud Rate (ini akan diatur di register lain, mungkin ada di reg.h atau implicit dari clock)

    // *(USART2_CR1) = 0x0000000C; // Mengatur USART2 Control Register 1.
                               // 0x0000000C (binary 1100) -> ini kemungkinan mengaktifkan TE (Transmitter Enable) dan RE (Receiver Enable).
    // *(USART2_CR2) = 0x00000000; // Mengatur USART2 Control Register 2. (Biasanya untuk Stop bits, CLKEN, LINEN, dll.)
                               // 0x00000000 berarti default (biasanya 1 stop bit, clock disable, dll.)
    // *(USART2_CR3) = 0x00000000; // Mengatur USART2 Control Register 3. (Biasanya untuk Flow Control, DMAT, DMAR, dll.)
                               // 0x00000000 berarti default (tanpa flow control, DMA disable, dll.)

    // Catatan penting: Konfigurasi Baud Rate seringkali melibatkan register USARTx_BRR (Baud Rate Register).
    // Kode ini tidak secara eksplisit mengaturnya di sini, yang berarti:
    //   a) Baud rate sudah diatur secara default atau
    //   b) Ada pengaturan di 'reg.h' atau di tempat lain yang memengaruhi baud rate, atau
    //   c) Ini adalah contoh sangat minimal yang mengandalkan nilai reset default yang mungkin berfungsi (tapi tidak direkomendasikan).
    // Untuk pengembangan OS yang kokoh, pengaturan baud rate harus eksplisit.

    *(USART2_CR1) = 0x0000000C; // Mengaktifkan Transmitter dan Receiver (TE dan RE bit di CR1)
    *(USART2_CR2) = 0x00000000; // Konfigurasi default untuk CR2 (biasanya 1 stop bit, no clock)
    *(USART2_CR3) = 0x00000000; // Konfigurasi default untuk CR3 (biasanya no flow control)
    *(USART2_CR1) |= 0x2000;    // Mengaktifkan USART itu sendiri (UE - USART Enable bit di CR1).
                               // Bit 13 (0x2000) di CR1 biasanya adalah USART Enable (UE).

    // Memanggil fungsi 'print_str' untuk menampilkan pesan selamat datang.
    // Pesan ini akan dikirim melalui USART2 ke konsol serial Anda.
    print_str(greet);

    // Loop tak terbatas. Setelah pesan dicetak, OS tidak melakukan apa-apa lagi
    // dan tetap di dalam loop ini. Ini adalah karakteristik umum dari OS minimal
    // atau program bare-metal di tahap awal.
    while (1)
        ;
}