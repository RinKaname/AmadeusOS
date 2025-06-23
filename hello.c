#include <stdint.h>
#include "reg.h"    // Sekarang file reg.h sudah disesuaikan untuk LM3S6965

// String pesan yang akan ditampilkan.
static char greet[] = "Welcome to Amadeus OS on LM3S6965evb! ^__________^\n";

// Fungsi untuk mengirim satu karakter melalui UART0.
void uart0_putc(char c)
{
    // *(UART0_FR) & UART0_FR_TXFF: Membaca Flag Register UART0 dan memeriksa bit TXFF (Transmit FIFO Full).
    // Loop ini akan menunggu selama TX FIFO (buffer pengirim) penuh.
    // Jika TX FIFO tidak penuh, kita bisa mengirim data.
    while (*(UART0_FR) & UART0_FR_TXFF);

    // Menulis karakter ke Data Register UART0.
    // Karakter akan otomatis dimasukkan ke TX FIFO dan dikirim.
    *(UART0_DR) = c;
}

// Fungsi untuk mencetak sebuah string ke konsol serial melalui UART0.
void print_str(const char *str)
{
    while (*str) {
        uart0_putc(*str);
        str++;
    }
}

// Fungsi inisialisasi UART0
void uart0_init(void)
{
    // 1. Aktifkan clock untuk GPIO Port A dan UART0
    // LM3S6965: Gunakan register SYSCTL_RCGC2 untuk GPIO dan SYSCTL_RCGCUART untuk UART.
    *(SYSCTL_RCGC2)   |= SYSCTL_RCGC2_GPIOA;  // Aktifkan clock untuk GPIO Port A
    *(SYSCTL_RCGCUART) |= SYSCTL_RCGCUART_UART0; // Aktifkan clock untuk UART0

    // Beri waktu sejenak agar clock stabil (delay kecil, penting untuk hardware nyata)
    // Variabel ui32Loop yang tidak terpakai telah dihapus karena -Werror.
    // Kode sebelumnya:
    // volatile uint32_t ui32Loop = 0;
    // ui32Loop = *(SYSCTL_RCGC2);
    // ui32Loop = *(SYSCTL_RCGCUART);

    // 2. Nonaktifkan UART0 (sebelum konfigurasi)
    *(UART0_CTL) &= ~(UART0_CTL_UARTEN);

    // 3. Konfigurasi GPIO Pin PA0 (Rx) dan PA1 (Tx) sebagai fungsi alternatif UART
    // Data sheet LM3S6965, Table 2-9. "GPIO Alternate Functions"
    // PA0 dan PA1 memiliki fungsi alternatif UART0
    *(GPIOA_AFSEL) |= ((1 << 0) | (1 << 1)); // Aktifkan fungsi alternatif untuk PA0 dan PA1
    *(GPIOA_PCTL)  |= ((1 << 0) | (1 << 4)); // Konfigurasi PCTL untuk PA0 dan PA1 (U0Rx/U0Tx)
                                             // PCTL_PA0_U0RX (0x00000001), PCTL_PA1_U0TX (0x00000010)

    *(GPIOA_DEN)   |= ((1 << 0) | (1 << 1)); // Aktifkan fungsi digital untuk PA0 dan PA1

    // 4. Konfigurasi Baud Rate UART0 (misal: 115200 baud)
    // Clock sistem untuk LM3S6965evb di QEMU biasanya 16 MHz.
    // Baud Rate Divisor (BRD) = System Clock / (16 * Baud Rate)
    // BRD = 16,000,000 / (16 * 115200) = 16,000,000 / 1,843,200 = 8.68055...
    // IBRD (Integer Baud Rate Divisor) = floor(BRD) = 8
    // FBRD (Fractional Baud Rate Divisor) = round(0.68055 * 64) = round(43.55) = 44
    // Jadi, IBRD = 8, FBRD = 44.

    *(UART0_IBRD) = 8;  // Set Integer Baud Rate Divisor
    *(UART0_FBRD) = 44; // Set Fractional Baud Rate Divisor

    // 5. Konfigurasi Line Control Register, High (LCRH)
    // - UART0_LCRH_WLEN_8: 8-bit data
    // - UART0_LCRH_FEN: Aktifkan FIFO
    // - No parity, 1 stop bit (default setelah bit-bit lain tidak diset)
    *(UART0_LCRH) = (UART0_LCRH_WLEN_8 | UART0_LCRH_FEN);

    // 6. Aktifkan UART0, Transmitter, dan Receiver
    *(UART0_CTL) = (UART0_CTL_UARTEN | UART0_CTL_TXE | UART0_CTL_RXE);
}


// Fungsi utama (main function) dari OS minimal Anda
void main(void)
{
    // Tidak ada inisialisasi clock global di sini.
    // Diasumsikan QEMU menyediakan clock dasar untuk LM3S6965evb,
    // atau diatur di linker script/startup assembly yang tidak kita lihat.
    // Inisialisasi clock yang lebih kompleks seperti di startup.c sebelumnya
    // harus disesuaikan dengan datasheet LM3S6965.
    // Untuk "Hello World", inisialisasi UART sudah termasuk aktivasi clock peripheral.

    // Inisialisasi UART0
    uart0_init();

    // Cetak pesan selamat datang
    print_str(greet);

    // Loop tak terbatas agar kernel tidak mati/reboot
    while (1);
}
