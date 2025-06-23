#include <stdint.h> // Standard C header for fixed-width integer types (e.g., uint32_t, uint16_t).
                    // Crucial for precise bit-width interactions with hardware registers.
#include "reg.h"    // Custom header defining hardware register addresses, now updated for LM3S6965evb.
#include <stddef.h> // Diperlukan untuk definisi NULL.

// Definisi karakter kontrol umum
#define ASCII_CR    0x0D // Carriage Return (Enter) - Karakter ASCII untuk Enter.
#define ASCII_LF    0x0A // Line Feed (Newline) - Karakter ASCII untuk baris baru.
#define ASCII_BS    0x08 // Backspace - Karakter ASCII untuk backspace.
#define ASCII_DEL   0x7F // Delete (Alternatif Backspace pada beberapa terminal/keyboard) - Karakter ASCII alternatif untuk delete/backspace.

// Ukuran buffer untuk input baris
#define MAX_LINE_LENGTH 128 // Mendefinisikan ukuran maksimum untuk buffer input baris (128 karakter).

// String pesan yang akan ditampilkan.
static char greet[] = "Welcome to Amadeus OS on LM3S6965evb! ^_^\n"; // Pesan selamat datang untuk OS.

// Fungsi untuk mengirim satu karakter melalui UART0.
void uart0_putc(char c)
{
    // *(UART0_FR) & UART0_FR_TXFF: Membaca Flag Register UART0 dan memeriksa bit TXFF (Transmit FIFO Full).
    // Loop ini akan menunggu selama TX FIFO (buffer pengirim) penuh.
    // Jika TX FIFO tidak penuh, kita bisa mengirim data.
    while (*(UART0_FR) & UART0_FR_TXFF); // Menunggu sampai Transmit FIFO (TX FIFO) tidak penuh.

    // Menulis karakter ke Data Register UART0.
    // Karakter akan otomatis dimasukkan ke TX FIFO dan dikirim.
    *(UART0_DR) = c; // Mengirim karakter 'c' ke UART Data Register (UART0_DR) untuk transmisi.
}

// Fungsi untuk menerima satu karakter melalui UART0.
char uart0_getc(void)
{
    // *(UART0_FR) & UART0_FR_RXFE: Membaca Flag Register UART0 dan memeriksa bit RXFE (Receive FIFO Empty).
    // Loop ini akan menunggu selama RX FIFO (buffer penerima) kosong.
    // Jika RX FIFO tidak kosong, berarti ada data yang bisa dibaca.
    while (*(UART0_FR) & UART0_FR_RXFE); // Menunggu sampai Receive FIFO (RX FIFO) tidak kosong (ada data).

    // Membaca karakter dari Data Register UART0.
    // Karakter akan otomatis diambil dari RX FIFO.
    return (char)(*(UART0_DR)); // Mengambil dan mengembalikan karakter dari UART Data Register (UART0_DR).
}


// Fungsi untuk mencetak sebuah string ke konsol serial melalui UART0.
// Ini akan mengkonversi setiap '\n' menjadi '\r\n' untuk kompatibilitas terminal yang lebih baik.
void print_str(const char *str)
{
    while (*str) { // Loop selama karakter saat ini bukan null terminator ('\0').
        if (*str == '\n') { // Jika menemukan karakter newline
            uart0_putc('\r'); // Kirim Carriage Return
        }
        uart0_putc(*str); // Kirim karakter saat ini
        str++; // Pindah ke karakter berikutnya.
    }
}

// Fungsi untuk membandingkan dua string (mirip dengan strcmp standar C).
// Mengembalikan 0 jika kedua string sama.
int strcmp(const char *s1, const char *s2) {
    while (*s1 && (*s1 == *s2)) { // Loop selama s1 belum berakhir dan karakter s1 sama dengan karakter s2.
        s1++; // Pindah ke karakter berikutnya di s1.
        s2++; // Pindah ke karakter berikutnya di s2.
    }
    // Mengembalikan perbedaan nilai ASCII dari karakter pertama yang tidak cocok, atau 0 jika string sama.
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

// Fungsi untuk menghitung panjang string.
int strlen(const char *s) {
    int len = 0;
    while (*s != '\0') {
        len++;
        s++;
    }
    return len;
}

// Fungsi untuk menyalin string (hingga n karakter atau sampai null terminator).
void strncpy(char *dest, const char *src, int n) {
    int i;
    for (i = 0; i < n && src[i] != '\0'; i++) {
        dest[i] = src[i];
    }
    for (; i < n; i++) { // Isi sisa dest dengan null jika src lebih pendek dari n
        dest[i] = '\0';
    }
}

// Fungsi untuk membersihkan layar terminal (menggunakan ANSI escape codes).
// ANSI escape codes adalah urutan karakter khusus yang dipahami oleh terminal untuk melakukan aksi (misal: membersihkan layar, memindahkan kursor).
void clear_screen(void) {
    // ESC [ 2 J : Hapus seluruh layar.
    // ESC [ H   : Pindahkan kursor ke posisi home (baris 1, kolom 1).
    print_str("\x1B[2J"); // Mengirim escape code untuk menghapus seluruh tampilan terminal.
    print_str("\x1B[H");  // Mengirim escape code untuk memindahkan kursor ke posisi awal (pojok kiri atas).
}

// Fungsi untuk membaca satu baris input dari UART (dengan penanganan backspace dan echo).
// Hasil input akan disimpan di 'buffer' dan dikembalikan setelah Enter ditekan.
void readline(char *buffer, int max_len) {
    int count = 0; // 'count' melacak jumlah karakter yang sudah ada di buffer (sekaligus posisi kursor logis).
    char c;        // 'c' akan menyimpan karakter yang baru dibaca dari UART.

    // Inisialisasi buffer dengan null terminator. Penting agar string dapat dibandingkan dengan benar (strcmp)
    // dan tidak mengandung "sampah" memori sebelum diisi.
    for (int i = 0; i < max_len; i++) {
        buffer[i] = '\0';
    }

    while (1) { // Loop tak terbatas untuk terus membaca karakter sampai kondisi keluar terpenuhi.
        c = uart0_getc(); // Baca satu karakter dari UART.

        if (c == ASCII_CR || c == ASCII_LF) { // Jika karakter yang diterima adalah Carriage Return (Enter) atau Line Feed.
            // Cukup kirimkan '\n' saja, print_str akan otomatis mengubahnya menjadi '\r\n'
            // Ini akan memindahkan kursor ke baris baru, memastikan output OS tidak tumpang tindih dengan echo pengguna.
            uart0_putc('\n'); 
            buffer[count] = '\0'; // Tambahkan null terminator di akhir string yang tersimpan di buffer.
            return; // Keluar dari fungsi readline(), menandakan satu baris input sudah lengkap.
        } else if (c == ASCII_BS || c == ASCII_DEL) { // Jika karakter yang diterima adalah Backspace atau Delete.
            if (count > 0) { // Hanya proses backspace jika ada karakter yang sudah diketik di buffer.
                count--;     // Kurangi 'count' (secara logis menghapus karakter terakhir dari buffer).
                uart0_putc(ASCII_BS); // Kirim karakter backspace ke terminal untuk menggerakkan kursor mundur.
                uart0_putc(' ');      // Tulis spasi untuk menimpa (menghapus visual) karakter sebelumnya.
                uart0_putc(ASCII_BS); // Pindahkan kursor mundur lagi ke posisi yang baru dihapus.
                buffer[count] = '\0'; // Set karakter di posisi 'count' yang baru menjadi null terminator (menghapus secara logis dari string).
            }
        } else { // Jika karakter yang diterima adalah karakter biasa (bukan Enter atau Backspace/Delete).
            if (count < max_len - 1) { // Pastikan buffer belum penuh (-1 menyisakan ruang untuk null terminator).
                buffer[count] = c;  // Simpan karakter yang diterima ke dalam buffer.
                count++;            // Tambah 'count' untuk karakter berikutnya.
                uart0_putc(c);      // Echo karakter ke terminal agar pengguna dapat melihat apa yang diketik.
            }
        }
    }
}

// --- Fungsi Konversi Angka dan Print Angka ---

// Mengkonversi string (ASCII) menjadi integer (mendukung angka positif/negatif).
// Fungsi ini mengabaikan spasi di awal dan membaca digit hingga karakter non-digit.
// Juga dapat mengabaikan spasi setelah operator.
int atoi(const char *s) {
    int res = 0;
    int sign = 1; // Default positif
    int i = 0;

    while (s[i] == ' ') i++; // Lewati spasi di awal

    if (s[i] == '-') { // Cek tanda negatif
        sign = -1;
        i++;
    } else if (s[i] == '+') { // Cek tanda positif (opsional)
        i++;
    }

    while (s[i] >= '0' && s[i] <= '9') {
        res = res * 10 + (s[i] - '0');
        i++;
    }
    return res * sign; // Kembalikan hasil dengan tanda
}

// Mengkonversi integer menjadi string (ASCII).
// Fungsi ini akan mengisi buffer dengan representasi string dari angka.
void itoa(int n, char *s) {
    int i = 0;
    int sign = n; // Simpan tanda asli untuk penanganan negatif
    char tmp_s[16]; // Buffer sementara untuk menyimpan digit terbalik

    if (n == 0) {
        s[i++] = '0';
        s[i] = '\0';
        return;
    }

    if (sign < 0) {
        n = -n; // Buat angka positif untuk konversi digit
    }

    do { // Ubah digit terakhir angka menjadi karakter dan simpan di tmp_s
        tmp_s[i++] = n % 10 + '0';
    } while ((n /= 10) > 0);

    if (sign < 0) {
        tmp_s[i++] = '-'; // Tambahkan tanda negatif jika angka aslinya negatif
    }
    tmp_s[i] = '\0'; // Null-terminate buffer sementara

    // Membalikkan string dari tmp_s ke s
    int j = 0;
    for (j = 0, i--; i >= 0; i--, j++) {
        s[j] = tmp_s[i];
    }
    s[j] = '\0'; // Null-terminate string akhir
}

// --- Akhir Fungsi Konversi Angka dan Print Angka ---


// Fungsi inisialisasi UART0
void uart0_init(void)
{
    // 1. Aktifkan clock untuk GPIO Port A dan UART0
    // LM3S6965: Gunakan register SYSCTL_RCGC2 untuk GPIO dan SYSCTL_RCGCUART untuk UART.
    *(SYSCTL_RCGC2)    |= SYSCTL_RCGC2_GPIOA;  // Mengaktifkan clock untuk GPIO Port A. Penting sebelum mengkonfigurasi pin GPIO.
    *(SYSCTL_RCGCUART) |= SYSCTL_RCGCUART_UART0; // Mengaktifkan clock untuk modul UART0. Penting sebelum mengkonfigurasi register UART.

    // Baris dummy read dihapus karena compiler menandainya sebagai variabel tidak terpakai,
    // yang menyebabkan error karena flag -Werror.
    // Pengaktifan clock seharusnya cukup untuk emulator QEMU.
    // volatile uint32_t dummy_read; // Variabel dummy untuk memastikan instruksi tidak dioptimasi.
    // dummy_read = *(SYSCTL_RCGC2); // Pembacaan dummy untuk memastikan clock GPIOA sudah aktif.
    // dummy_read = *(SYSCTL_RCGCUART); // Pembacaan dummy untuk memastikan clock UART0 sudah aktif.

    // 2. Nonaktifkan UART0 (sebelum konfigurasi)
    // Penting: UART harus dinonaktifkan (UARTEN bit clear di UARTCTL) sebelum mengubah konfigurasi
    // register-register kontrol lainnya. Ini mencegah perilaku tak terduga.
    *(UART0_CTL) &= ~(UART0_CTL_UARTEN); // Menghapus bit UARTEN (Bit 0) di UART0_CTL untuk menonaktifkan UART.

    // 3. Konfigurasi GPIO Pin PA0 (Rx) dan PA1 (Tx) sebagai fungsi alternatif UART
    // Data sheet LM3S6965, Table 2-9. "GPIO Alternate Functions" (halaman 288)
    // PA0 (pin 26 LQFP, L3 BGA) = U0Rx (UART0 Receive)
    // PA1 (pin 27 LQFP, M3 BGA) = U0Tx (UART0 Transmit)
    *(GPIOA_AFSEL) |= ((1 << 0) | (1 << 1)); // Mengaktifkan fungsi alternatif untuk pin PA0 (bit 0) dan PA1 (bit 1) di GPIO Alternate Function Select Register.
                                             // Ini memberitahu chip bahwa pin-pin ini tidak akan berfungsi sebagai GPIO standar.
    *(GPIOA_PCTL)  |= ((1 << 0) | (1 << 4)); // Mengkonfigurasi GPIO Port Control Register (PCTL) untuk memilih fungsi alternatif UART0.
                                             // Untuk PA0 (bit 0-3) dan PA1 (bit 4-7), nilai 0x1 biasanya memilih fungsi alternatif pertama.
                                             // (1 << 0) mengatur nibble PA0_PCTL ke 0x1.
                                             // (1 << 4) mengatur nibble PA1_PCTL ke 0x1.

    *(GPIOA_DEN)   |= ((1 << 0) | (1 << 1)); // Mengaktifkan fungsi digital untuk pin PA0 (bit 0) dan PA1 (bit 1) di GPIO Digital Enable Register.
                                             // Ini memastikan pin-pin ini dapat digunakan untuk sinyal digital UART.

    // 4. Konfigurasi Baud Rate UART0 (misal: 115200 baud)
    // Clock sistem untuk LM3S6965evb di QEMU umumnya 50 MHz (sesuai spesifikasi chip).
    // Baud Rate Divisor (BRD) = System Clock / (16 * Baud Rate)
    // BRD = 50,000,000 / (16 * 115200) = 50,000,000 / 1,843,200 = 27.1267...
    // IBRD (Integer Baud Rate Divisor) = floor(BRD) = 27
    // FBRD (Fractional Baud Rate Divisor) = round(0.1267 * 64) = round(8.1088) = 8
    // Jadi, IBRD = 27, FBRD = 8.
    // Perubahan ini akan berlaku saat menulis ke UART0_LCRH.
    *(UART0_IBRD) = 27;  // Mengatur bagian integer dari pembagi baud rate (Integer Baud Rate Divisor).
    *(UART0_FBRD) = 8; // Mengatur bagian pecahan dari pembagi baud rate (Fractional Baud Rate Divisor).

    // 5. Konfigurasi Line Control Register, High (LCRH)
    // Menulis ke register ini akan memicu pembaruan IBRD dan FBRD.
    // - UART0_LCRH_WLEN_8: 8-bit data
    // - UART0_LCRH_FEN: Aktifkan FIFO
    // - No parity, 1 stop bit (default karena bit lain tidak diset)
    *(UART0_LCRH) = (UART0_LCRH_WLEN_8 | UART0_LCRH_FEN); // Mengkonfigurasi format data serial dan mengaktifkan FIFO.

    // 6. Aktifkan UART0, Transmitter, dan Receiver
    // UARTEN: Aktifkan modul UART
    // TXE: Aktifkan bagian transmitter
    // RXE: Aktifkan bagian receiver
    *(UART0_CTL) = (UART0_CTL_UARTEN | UART0_CTL_TXE | UART0_CTL_RXE); // Mengaktifkan fungsionalitas keseluruhan UART.
}


// Fungsi utama (main function) dari OS minimal Anda
void main(void)
{
    // Buffer untuk menyimpan baris input dari pengguna
    char line_buffer[MAX_LINE_LENGTH];

    // Inisialisasi UART0 (untuk input dan output serial)
    uart0_init();

    // Cetak pesan selamat datang
    print_str(greet);
    
    // Loop utama shell
    while (1) { // Loop tak terbatas untuk menjalankan shell.
        print_str("AmadeusOS> "); // Tampilkan prompt "AmadeusOS> ".
        readline(line_buffer, MAX_LINE_LENGTH); // Baca satu baris input dari pengguna, termasuk penanganan backspace.

        // --- Menganalisis Perintah dan Menerapkan Perintah Dasar ---
        // Penanganan masalah tampilan: readline sudah menambahkan CR/LF, jadi OS response akan di baris baru.
        // Sekarang kita perlu memastikan jika user mengetik sesuatu, itu tidak mengotori output.

        // Perbaikan parsing echo dan calc:
        // Cek apakah perintah dimulai dengan "echo " atau "calc " (case-insensitive awal)
        
        // Memisahkan perintah dari argumennya
        char command_name[16]; // Buffer untuk nama perintah
        const char *args_ptr = line_buffer; // Pointer ke argumen

        // Lewati spasi di awal baris input
        while (*args_ptr == ' ') args_ptr++;

        int i = 0;
        // Salin nama perintah hingga spasi pertama atau null terminator
        while (args_ptr[i] != ' ' && args_ptr[i] != '\0' && i < 15) {
            // Konversi ke huruf kecil untuk perbandingan case-insensitive
            if (args_ptr[i] >= 'A' && args_ptr[i] <= 'Z') {
                command_name[i] = args_ptr[i] + ('a' - 'A');
            } else {
                command_name[i] = args_ptr[i];
            }
            i++;
        }
        command_name[i] = '\0'; // Null-terminate nama perintah

        // Majukan args_ptr ke awal argumen (setelah spasi pertama setelah nama perintah)
        if (args_ptr[i] == ' ') { // Jika ada spasi setelah perintah
            args_ptr += i + 1; // Majukan pointer setelah nama perintah dan spasi
            while (*args_ptr == ' ') args_ptr++; // Lewati spasi berlebih
        } else { // Jika tidak ada spasi (perintah tanpa argumen)
            args_ptr += i; // Majukan pointer ke null terminator
        }
        

        if (strcmp(command_name, "help") == 0) { // Jika input adalah string "help".
            print_str("Available commands:\n"); // Cetak pesan daftar perintah.
            print_str("  help   - Display this help message\n");
            print_str("  echo   - Echoes back the input (e.g., echo hello world)\n"); // Contoh penggunaan.
            print_str("  clear  - Clears the terminal screen\n");
            print_str("  calc   - Basic calculator (e.g., calc 5 + 3, calc 10 * -2)\n"); // Update help for calc
            print_str("  exit   - Exits the QEMU emulator (requires Ctrl-A X)\n");
        } else if (strcmp(command_name, "clear") == 0) { // Jika input adalah string "clear".
            clear_screen(); // Panggil fungsi untuk membersihkan layar terminal.
        } else if (strcmp(command_name, "echo") == 0) { // Penanganan untuk perintah 'echo'.
            print_str("Echo: ");
            print_str(args_ptr); // Cetak argumen
            print_str("\n");
        } else if (strcmp(command_name, "calc") == 0) { // Penanganan untuk perintah 'calc'.
            // Format diharapkan: "<angka1> <operator> <angka2>" setelah "calc "
            
            const char *num1_start_ptr = args_ptr; // Pointer ke awal angka pertama
            char op_char = '\0';             // Karakter operator
            const char *num2_start_ptr = NULL;     // Pointer ke awal angka kedua

            // Parsing num1
            int num1 = atoi(num1_start_ptr);
            // Majukan pointer ke karakter setelah angka1 (dan mungkin tanda)
            const char *temp_num1_end = num1_start_ptr;
            if (*temp_num1_end == '-' || *temp_num1_end == '+') temp_num1_end++; // Lewati tanda jika ada
            while (*temp_num1_end >= '0' && *temp_num1_end <= '9') temp_num1_end++;
            
            // Majukan current_ptr melewati angka pertama dan spasi
            const char *current_ptr = temp_num1_end;
            while (*current_ptr == ' ') current_ptr++; // Lewati spasi

            // Parsing operator
            if (*current_ptr == '+' || *current_ptr == '-' || *current_ptr == '*' || *current_ptr == '/') {
                op_char = *current_ptr; // Simpan karakter operator
                current_ptr++;
            } else {
                print_str("Error: Invalid operator or calc format. Use: calc <num1> <op> <num2>\n");
                goto end_calc; // Lompat ke bagian akhir calc
            }

            // Lewati spasi setelah operator
            while (*current_ptr == ' ') current_ptr++;

            // Parsing num2
            num2_start_ptr = current_ptr; // num2 dimulai dari posisi current_ptr saat ini
            int num2 = atoi(num2_start_ptr); 
            
            // Lakukan perhitungan
            int result = 0;
            char result_str[16]; 
            switch (op_char) { // Gunakan op_char yang dideklarasikan
                case '+': result = num1 + num2; break;
                case '-': result = num1 - num2; break;
                case '*': result = num1 * num2; break;
                case '/':
                    if (num2 != 0) {
                        result = num1 / num2;
                    } else {
                        print_str("Error: Division by zero!\n");
                        goto end_calc; // Lompat ke bagian akhir calc
                    }
                    break;
                default:
                    print_str("Error: Unknown operator.\n"); 
                    goto end_calc; 
            }

            // Cetak hasil
            print_str("Result: ");
            itoa(result, result_str); 
            print_str(result_str);
            print_str("\n");
            
            end_calc:; // Label untuk goto
        }
        else if (strcmp(command_name, "exit") == 0) { // Menambahkan penanganan perintah 'exit'
            print_str("Exiting Amadeus OS...\n");
            // Untuk keluar dari loop utama, yang mengakhiri shell.
            // Di QEMU, ini akan membuat CPU idle di infinite loop di startup.c setelah main() selesai.
            // Anda tetap perlu Ctrl-A X untuk keluar dari emulator QEMU secara penuh.
            return; // Keluar dari main loop, main() akan berakhir
        }
        else if (command_name[0] != '\0') { // Jika nama perintah bukan string kosong, dan bukan perintah yang dikenali.
            print_str("Command not found: "); // Cetak pesan "Command not found".
            print_str(line_buffer); // Cetak seluruh baris input (termasuk argumen).
            print_str("\n"); // Tambahkan baris baru.
        }
        // --- Akhir Analisis Perintah dan Perintah Dasar ---
    }
}