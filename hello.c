#include <stdint.h> // Standard C header for fixed-width integer types (e.g., uint32_t, uint16_t).
                    // Crucial for precise bit-width interactions with hardware registers.
#include "reg.h"    // Custom header defining hardware register addresses, now updated for LM3S6965evb.
#include <stddef.h> // Diperlukan untuk definisi NULL (size_t) dan NULL pointer.
// #include <stdlib.h> // DIHAPUS: Menyebabkan konflik deklarasi dengan atoi() dan itoa() kustom kita.

// Definisi karakter kontrol umum
#define ASCII_CR    0x0D // Carriage Return (Enter) - Karakter ASCII untuk Enter.
#define ASCII_LF    0x0A // Line Feed (Newline) - Karakter ASCII untuk baris baru.
#define ASCII_BS    0x08 // Backspace - Karakter ASCII untuk backspace.
#define ASCII_DEL   0x7F // Delete (Alternatif Backspace pada beberapa terminal/keyboard) - Karakter ASCII alternatif untuk delete/backspace.

// Ukuran buffer untuk input baris
#define MAX_LINE_LENGTH 128 // Mendefinisikan ukuran maksimum untuk buffer input baris (128 karakter).

// Clock sistem untuk LM3S6965evb (biasanya 50 MHz)
#define SYSTEM_CLOCK_HZ 50000000U // 50,000,000 Hertz (50 MHz)

// String pesan yang akan ditampilkan.
static char greet[] = "Welcome to Amadeus OS on LM3S6965evb! ^_^\n"; // Pesan selamat datang untuk OS.

// ============================================================================
// Deklarasi Eksternal dari Linker Script untuk Heap
// ============================================================================
// Ini adalah simbol-simbol yang didefinisikan dalam linker script (hello.ld)
// yang menandai awal dan akhir dari wilayah memori 'heap' di RAM.
extern uint32_t __heap_start__; // Alamat awal heap (setelah .bss)
extern uint32_t __heap_end__;   // Alamat akhir heap (sama dengan _estack)

// ============================================================================
// Deklarasi Prototype Fungsi-fungsi Kustom
// ============================================================================
// Penting: Fungsi-fungsi ini harus dideklarasikan sebelum dipanggil di main() atau fungsi lain.
void uart0_putc(char c);
char uart0_getc(void);
void print_str(const char *str);
int strcmp(const char *s1, const char *s2);
int strlen(const char *s);
void strncpy(char *dest, const char *src, int n);
void clear_screen(void);
void readline(char *buffer, int max_len);
int atoi(const char *s, const char **endptr); // Prototype atoi dengan endptr
void itoa(int n, char *s); // Prototype itoa
void uart0_init(void);

// Deklarasi Prototype untuk Heap Manager
void malloc_init(void);
void* malloc(size_t size);
void free(void* ptr);


// ============================================================================
// Implementasi Heap Manager (malloc/free)
// Referensi: Simple Free List Allocator for Embedded Systems
// ============================================================================

// Header untuk setiap blok memori yang dialokasikan atau bebas di heap.
// Ukuran blok harus sejajar (aligned) ke batas 4 byte untuk ARM Cortex-M.
typedef struct BlockHeader {
    size_t size;          // Ukuran blok ini (termasuk ukuran header itu sendiri)
    struct BlockHeader* next; // Pointer ke blok bebas berikutnya dalam free list.
                              // Hanya relevan jika blok ini bebas.
    int free;             // Flag: 1 jika blok bebas, 0 jika dialokasikan.
} BlockHeader;

// 'free_list_head' akan menunjuk ke kepala dari linked list semua blok memori bebas.
static BlockHeader* free_list_head = NULL;

// Makro untuk menyelaraskan (align) ukuran ke kelipatan 4 byte.
// Ukuran setiap blok memori di heap harus kelipatan 4 byte (ukuran word pada ARM).
#define ALIGN_SIZE(size) (((size) + (sizeof(uint32_t) - 1)) & ~(sizeof(uint32_t) - 1))
// Ukuran minimum blok yang dialokasikan harus setidaknya sebesar BlockHeader itu sendiri.
#define MIN_BLOCK_SIZE   (ALIGN_SIZE(sizeof(BlockHeader)))

// Fungsi untuk menginisialisasi heap manager.
// Dipanggil sekali saat startup OS.
void malloc_init(void) {
    // Tentukan ukuran total heap yang tersedia dari simbol linker script.
    size_t total_heap_size = (size_t)(&__heap_end__ - &__heap_start__);

    // Inisialisasi blok pertama heap sebagai satu blok besar yang bebas.
    free_list_head = (BlockHeader*)&__heap_start__;
    free_list_head->size = total_heap_size;
    free_list_head->free = 1; // Tandai sebagai bebas.
    free_list_head->next = NULL; // Tidak ada blok bebas lain awalnya.
}

// Fungsi untuk mengalokasikan memori dari heap.
// Menggunakan algoritma 'first-fit': mencari blok bebas pertama yang cukup besar.
void* malloc(size_t size) {
    // Sesuaikan ukuran permintaan dengan kebutuhan header dan alignment.
    size_t aligned_size = ALIGN_SIZE(size + sizeof(BlockHeader));
    if (aligned_size < MIN_BLOCK_SIZE) {
        aligned_size = MIN_BLOCK_SIZE; // Pastikan minimal sebesar header.
    }

    BlockHeader* current = free_list_head;
    BlockHeader* prev = NULL;

    while (current != NULL) {
        if (current->free && current->size >= aligned_size) {
            // Ditemukan blok yang cukup besar dan bebas.

            // Cek apakah blok bisa dipecah.
            // Jika sisa blok setelah alokasi cukup besar untuk menjadi blok bebas baru,
            // maka blok bisa dipecah.
            if (current->size - aligned_size >= MIN_BLOCK_SIZE) {
                // Pecah blok: buat blok baru untuk sisa memori.
                BlockHeader* new_free_block = (BlockHeader*)((uint8_t*)current + aligned_size);
                new_free_block->size = current->size - aligned_size;
                new_free_block->free = 1;
                new_free_block->next = current->next;

                // Sesuaikan ukuran blok yang dialokasikan dan next pointer.
                current->size = aligned_size;
                current->next = new_free_block; // Update current->next to point to the new free block
            }
            
            // Tandai blok sebagai dialokasikan.
            current->free = 0;

            // Hapus blok ini dari free list.
            // Jika blok yang ditemukan adalah kepala free list
            if (prev == NULL) { 
                free_list_head = current->next; // Kepala free list menjadi blok berikutnya
            } else {
                prev->next = current->next; // Lewati blok yang baru dialokasikan
            }

            // Kembalikan pointer ke data (setelah header).
            return (void*)((uint8_t*)current + sizeof(BlockHeader));

        }
        prev = current;
        current = current->next;
    }

    // Jika tidak ditemukan blok yang cocok.
    return NULL;
}

// Fungsi untuk membebaskan memori yang sebelumnya dialokasikan oleh malloc.
void free(void* ptr) {
    if (ptr == NULL) {
        return; // Tidak bisa membebaskan pointer NULL.
    }

    // Dapatkan pointer ke header blok dari pointer yang diberikan.
    BlockHeader* block_to_free = (BlockHeader*)((uint8_t*)ptr - sizeof(BlockHeader));

    // Validasi sederhana: Pastikan ini adalah blok yang valid dan belum dibebaskan
    // (diperlukan implementasi yang lebih robust untuk production OS)
    if (block_to_free->free) { // Jika sudah bebas, ini adalah double-free atau korupsi
        print_str("Warning: Attempted to double-free or free invalid block!\n");
        return; 
    }

    block_to_free->free = 1; // Tandai blok sebagai bebas.

    // Masukkan blok ke free list dan coba gabungkan (coalesce)
    // Coalescing adalah menggabungkan blok bebas yang berdekatan menjadi satu blok yang lebih besar.
    // Ini membantu mengurangi fragmentasi memori.

    // Cari posisi yang benar di free list (urut berdasarkan alamat untuk coalescing mudah)
    BlockHeader* current = free_list_head;
    BlockHeader* prev = NULL;
    while (current != NULL && current < block_to_free) { // Cari tempat block_to_free di free list
        prev = current;
        current = current->next;
    }

    if (prev == NULL) { // Jika block_to_free harus di awal list
        free_list_head = block_to_free;
    } else {
        prev->next = block_to_free;
    }
    block_to_free->next = current; // block_to_free menunjuk ke blok setelahnya

    // Lakukan coalescing
    // Gabungkan dengan blok setelahnya jika bebas dan berdekatan
    if (block_to_free->next != NULL && block_to_free->next->free &&
        (uint8_t*)block_to_free + block_to_free->size == (uint8_t*)block_to_free->next) {
        
        block_to_free->size += block_to_free->next->size;
        block_to_free->next = block_to_free->next->next;
    }

    // Gabungkan dengan blok sebelumnya jika bebas dan berdekatan
    if (prev != NULL && prev->free && 
        (uint8_t*)prev + prev->size == (uint8_t*)block_to_free) {

        prev->size += block_to_free->size;
        prev->next = block_to_free->next;
    }
}

// ============================================================================
// Akhir Implementasi Heap Manager
// ============================================================================


// Fungsi inisialisasi UART0
void uart0_init(void)
{
    // 1. Aktifkan clock untuk GPIO Port A dan UART0
    // LM3S6965: Gunakan register SYSCTL_RCGC2 untuk GPIO dan SYSCTL_RCGCUART untuk UART.
    *(SYSCTL_RCGC2)    |= SYSCTL_RCGC2_GPIOA;  // Mengaktifkan clock untuk GPIO Port A. Penting sebelum mengkonfigurasi pin GPIO.
    *(SYSCTL_RCGCUART) |= SYSCTL_RCGCUART_UART0; // Mengaktifkan clock untuk modul UART0. Penting sebelum mengkonfigurasi register UART.

    // Baris dummy read dihapus karena compiler menandainya sebagai variabel tidak terpakai,
    // yang menyebabkan error flag -Werror.
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
    char temp_str[32]; // Buffer sementara untuk itoa

    // Inisialisasi UART0 (untuk input dan output serial)
    uart0_init();

    // Inisialisasi Heap Manager
    malloc_init();

    // Cetak pesan selamat datang
    print_str(greet);
    
    // Loop utama shell
    while (1) { // Loop tak terbatas untuk menjalankan shell.
        print_str("AmadeusOS> "); // Tampilkan prompt "AmadeusOS> ".
        readline(line_buffer, MAX_LINE_LENGTH); // Baca satu baris input dari pengguna, termasuk penanganan backspace.

        // --- Menganalisis Perintah dan Menerapkan Perintah Dasar ---
        // Penanganan masalah tampilan: readline sudah menambahkan CR/LF, jadi OS response akan di baris baru.
        // Sekarang kita perlu memastikan jika user mengetik sesuatu, itu tidak mengotori output.

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
            print_str("  meminfo- Display heap memory information\n"); // Tambahkan deskripsi meminfo
            print_str("  alloc  - Allocate memory from heap (e.g., alloc 16)\n"); // Tambahkan deskripsi alloc
            print_str("  free   - Free memory from heap (e.g., free <addr>)\n"); // Tambahkan deskripsi free
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
            const char *parsed_num1_end = NULL; // Pointer untuk menerima posisi akhir num1 dari atoi
            int num1 = atoi(num1_start_ptr, &parsed_num1_end);

            // Cek apakah atoi berhasil parsing digit untuk num1
            if (num1_start_ptr == parsed_num1_end && (*num1_start_ptr != '-' && *num1_start_ptr != '+')) {
                print_str("Error: Invalid number format for first operand. Use: calc <num1> <op> <num2>\n");
                goto end_cmd; // Lompat ke bagian akhir command handling
            }
            
            // Majukan current_ptr melewati angka pertama dan spasi
            const char *current_ptr = parsed_num1_end; // Mulai dari akhir num1 yang diparsing
            while (*current_ptr == ' ') current_ptr++; // Lewati spasi

            // Parsing operator
            if (*current_ptr == '+' || *current_ptr == '-' || *current_ptr == '*' || *current_ptr == '/') {
                op_char = *current_ptr; // Simpan karakter operator
                current_ptr++;
            } else {
                print_str("Error: Invalid operator or calc format. Use: calc <num1> <op> <num2>\n");
                goto end_cmd; // Lompat ke bagian akhir command handling
            }

            // Lewati spasi setelah operator
            while (*current_ptr == ' ') current_ptr++;

            // Parsing num2
            const char *parsed_num2_end = NULL; // Pointer untuk menerima posisi akhir num2 dari atoi
            num2_start_ptr = current_ptr; // num2 dimulai dari posisi current_ptr saat ini
            int num2 = atoi(num2_start_ptr, &parsed_num2_end);
            
            // Cek apakah atoi berhasil parsing digit untuk num2
            if (num2_start_ptr == parsed_num2_end) { // Jika pointer tidak maju, berarti tidak ada angka
                print_str("Error: Invalid number format for second operand. Use: calc <num1> <op> <num2>\n");
                goto end_cmd;
            }

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
                        goto end_cmd; // Lompat ke bagian akhir command handling
                    }
                    break;
                default:
                    print_str("Error: Unknown operator.\n"); 
                    goto end_cmd; 
            }

            // Cetak hasil
            print_str("Result: ");
            itoa(result, result_str); 
            print_str(result_str);
            print_str("\n");
            
            end_cmd:; // Label untuk goto
        }
        else if (strcmp(command_name, "meminfo") == 0) { // Perintah meminfo
            print_str("Heap Information:\n");
            print_str("  Heap Start: 0x"); itoa((int)&__heap_start__, temp_str); print_str(temp_str); print_str("\n");
            print_str("  Heap End:   0x"); itoa((int)&__heap_end__, temp_str); print_str(temp_str); print_str("\n");
            print_str("  Total Heap Size: "); itoa((int)(&__heap_end__ - &__heap_start__), temp_str); print_str(temp_str); print_str(" bytes\n");
            print_str("  Free Blocks:\n");

            BlockHeader* current = free_list_head;
            int free_count = 0;
            size_t total_free_bytes = 0;
            char addr_str[16];
            char size_str[16];

            while(current != NULL) {
                if (current->free) {
                    free_count++;
                    total_free_bytes += current->size;
                    print_str("    - Addr: 0x"); itoa((int)current, addr_str); print_str(addr_str);
                    print_str(", Size: "); itoa((int)current->size, size_str); print_str(size_str); print_str(" bytes\n");
                }
                current = current->next;
            }
            if (free_count == 0) {
                print_str("    (No free blocks)\n");
            }
            print_str("  Total Free Blocks: "); itoa(free_count, temp_str); print_str(temp_str); print_str("\n");
            print_str("  Total Free Bytes: "); itoa((int)total_free_bytes, temp_str); print_str(temp_str); print_str(" bytes\n");
            print_str("\n");
        }
        else if (strcmp(command_name, "alloc") == 0) { // Perintah alloc
            const char *size_ptr = args_ptr;
            int size_to_alloc = atoi(size_ptr, NULL); // atoi tidak perlu endptr jika hanya 1 angka

            if (size_to_alloc <= 0) {
                print_str("Error: Invalid size. Usage: alloc <bytes>\n");
            } else {
                void* allocated_ptr = malloc(size_to_alloc);
                if (allocated_ptr != NULL) {
                    print_str("Allocated "); itoa(size_to_alloc, temp_str); print_str(temp_str);
                    print_str(" bytes at 0x"); itoa((int)allocated_ptr, temp_str); print_str(temp_str); print_str("\n");
                } else {
                    print_str("Error: Memory allocation failed.\n");
                }
            }
            print_str("\n");
        }
        else if (strcmp(command_name, "free") == 0) { // Perintah free
            const char *addr_ptr_str = args_ptr;
            uint32_t addr = 0;
            char temp_char;

            // Parsing alamat heksadesimal (misal: "0x20000200")
            // Hanya untuk parsing 0x<hex_value>
            if (addr_ptr_str[0] == '0' && (addr_ptr_str[1] == 'x' || addr_ptr_str[1] == 'X')) {
                addr_ptr_str += 2; // Lewati "0x"
                while (*addr_ptr_str != '\0') {
                    temp_char = *addr_ptr_str;
                    if (temp_char >= '0' && temp_char <= '9') {
                        addr = (addr << 4) | (temp_char - '0');
                    } else if (temp_char >= 'a' && temp_char <= 'f') {
                        addr = (addr << 4) | (temp_char - 'a' + 10);
                    } else if (temp_char >= 'A' && temp_char <= 'F') {
                        addr = (addr << 4) | (temp_char - 'A' + 10);
                    } else {
                        // Karakter non-heksa yang tidak valid
                        addr = 0; // Reset alamat menjadi 0 (invalid)
                        break;
                    }
                    addr_ptr_str++;
                }
            } else {
                // Bukan format heksadesimal 0x
                print_str("Error: Invalid address format. Use hex (e.g., free 0x20000200)\n");
                goto end_cmd; // Lompat ke bagian akhir command handling
            }

            if (addr != 0) {
                // Hati-hati: free memerlukan pointer yang sama persis yang dikembalikan oleh malloc.
                // Mengkonversi alamat integer ke pointer void* untuk free.
                free((void*)addr);
                print_str("Freed memory at 0x"); itoa((int)addr, temp_str); print_str(temp_str); print_str("\n");
            } else {
                print_str("Error: Invalid address to free.\n");
            }
            print_str("\n");
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