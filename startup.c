#include <stdint.h> // Header standar C untuk tipe data integer dengan ukuran tetap (misal: uint32_t, uint16_t).
                    // Penting untuk memastikan ukuran bit yang tepat saat berinteraksi dengan register hardware.
#include "reg.h"    // Header kustom yang kita bahas sebelumnya, mendefinisikan alamat-alamat register hardware.

/*
 * Definisi Bit untuk Register-Register Penting:
 * Ini adalah 'mask' (topeng bit) yang digunakan untuk mengatur atau membaca bit tertentu
 * dalam register-register hardware. Setiap angka heksadesimal merepresentasikan posisi bit.
 */

/* Bit definition for RCC_CR register (Reset and Clock Control - Clock Register) */
// RCC_CR mengontrol osilator dan status clock utama.
#define RCC_CR_HSION    ((uint32_t)0x00000001)   /*!< Internal High Speed clock enable (Mengaktifkan osilator internal) */
#define RCC_CR_HSEON    ((uint32_t)0x00010000)   /*!< External High Speed clock enable (Mengaktifkan osilator eksternal) */
#define RCC_CR_HSERDY   ((uint32_t)0x00020000)   /*!< External High Speed clock ready flag (Bendera bahwa osilator eksternal siap) */
#define RCC_CR_CSSON    ((uint32_t)0x00080000)   /*!< Clock Security System enable (Mengaktifkan sistem keamanan clock) */

/* Bit definition for RCC_CFGR register (Reset and Clock Control - Configuration Register) */
// RCC_CFGR mengkonfigurasi pemilihan clock sistem dan pembagian frekuensi (prescaler) untuk bus.
#define RCC_CFGR_SW     ((uint32_t)0x00000003) /*!< SW[1:0] bits (System clock Switch) - Bit mask untuk memilih sumber clock sistem */
#define RCC_CFGR_SW_HSE ((uint32_t)0x00000001) /*!< HSE selected as system clock - Nilai untuk memilih osilator eksternal sebagai sumber clock sistem */
#define RCC_CFGR_SWS    ((uint32_t)0x0000000C) /*!< SWS[1:0] bits (System Clock Switch Status) - Bit mask untuk membaca status sumber clock yang sedang digunakan */
#define RCC_CFGR_HPRE_DIV1 ((uint32_t)0x00000000) /*!< SYSCLK not divided (HCLK = SYSCLK) - Prescaler AHB bus: clock sistem tidak dibagi */
#define RCC_CFGR_PPRE1_DIV1 ((uint32_t)0x00000000) /*!< HCLK not divided (PCLK1 = HCLK) - Prescaler APB1 bus: clock HCLK tidak dibagi */
#define RCC_CFGR_PPRE2_DIV1 ((uint32_t)0x00000000) /*!< HCLK not divided (PCLK2 = HCLK) - Prescaler APB2 bus: clock HCLK tidak dibagi */
                                                  // Nilai 0x00000000 pada prescaler artinya 'divide by 1' (tidak dibagi).

/* Bit definition for FLASH_ACR register (Flash Access Control Register) */
// FLASH_ACR mengontrol bagaimana memori Flash diakses, terutama latensi dan prefetch buffer.
// Ini penting karena CPU berjalan pada frekuensi tinggi, sementara Flash memori lebih lambat.
// Perlu ada 'wait states' agar CPU tidak terlalu cepat membaca Flash.
#define FLASH_ACR_LATENCY   ((uint8_t)0x03)     /*!< LATENCY[2:0] bits (Latency) - Bit mask untuk pengaturan latensi (wait states) */
#define FLASH_ACR_LATENCY_0 ((uint8_t)0x00)     /*!< Bit 0 - Nilai untuk 0 wait state (digunakan jika CPU clock rendah) */
#define FLASH_ACR_PRFTBE    ((uint8_t)0x10)     /*!< Prefetch Buffer Enable - Mengaktifkan buffer untuk mempercepat pembacaan Flash */

// Konstanta timeout untuk startup osilator eksternal (HSE).
// Digunakan untuk memastikan osilator sudah stabil sebelum digunakan.
#define HSE_STARTUP_TIMEOUT ((uint16_t)0x0500) /*!< Time out for HSE start up */

/*
 * Deklarasi Eksternal (Simbol dari Linker Script)
 * Ini adalah simbol-simbol (nama-nama) yang didefinisikan dalam linker script (`linker.ld`).
 * Kode C di sini tidak secara langsung mendefinisikan lokasi memori ini,
 * tetapi mengacu padanya agar bisa bekerja dengan manajemen memori.
 */
extern void main(void); // Deklarasi fungsi 'main' dari kernel Anda (kernel.c),
                        // menandakan ini adalah titik masuk utama program setelah startup.

extern uint32_t _sidata; // Alamat awal inisialisasi (.data) di memori Flash (Load Memory Address/LMA).
                         // Ini adalah nilai awal untuk variabel global yang sudah diinisialisasi.
extern uint32_t _sdata;  // Alamat awal bagian .data di RAM (Virtual Memory Address/VMA).
                         // Ke sini data akan disalin dari _sidata.
extern uint32_t _edata;  // Alamat akhir bagian .data di RAM.
                         // Digunakan untuk batas loop penyalinan.
extern uint32_t _sbss;   // Alamat awal bagian .bss di RAM.
                         // Ini adalah variabel global yang tidak diinisialisasi.
extern uint32_t _ebss;   // Alamat akhir bagian .bss di RAM.
                         // Digunakan untuk batas loop zero-filling.
extern uint32_t _estack; // Alamat akhir stack (biasanya alamat tertinggi di RAM yang dialokasikan untuk stack).
                         // Ini akan digunakan sebagai nilai awal untuk Stack Pointer (SP) CPU.

// Deklarasi fungsi untuk inisialisasi clock RCC.
// Definisi fungsi ini ada di bawah (rcc_clock_init).
void rcc_clock_init(void);

/*
 * reset_handler: Fungsi Penanganan Reset (Reset Handler)
 * Ini adalah fungsi C pertama yang dieksekusi oleh CPU setelah terjadi reset (misalnya saat power-on).
 * Alamat fungsi ini akan ditempatkan di tabel vektor interrupt.
 * Tanggung jawab utamanya adalah menyiapkan lingkungan C runtime sebelum memanggil main().
 */
void reset_handler(void)
{
    /* Copy the data segment initializers from flash to SRAM */
    // Bagian ini menyalin data dari bagian '.data' yang disimpan di memori Flash (ROM)
    // ke lokasi RAM yang sebenarnya (SRAM).
    // Variabel global yang diinisialisasi di C (seperti 'static char greet[]' di main.c)
    // disimpan di Flash sebagai bagian dari binary program. Saat startup, mereka perlu disalin ke RAM
    // agar CPU bisa mengakses dan memodifikasinya (jika perlu).
    uint32_t *idata_begin = &_sidata; // Pointer ke awal data inisialisasi di Flash (ROM).
    uint32_t *data_begin = &_sdata;   // Pointer ke awal lokasi .data di RAM.
    uint32_t *data_end = &_edata;     // Pointer ke akhir lokasi .data di RAM.
    while (data_begin < data_end) *data_begin++ = *idata_begin++; // Loop penyalinan:
                                                                // Ambil data dari Flash, salin ke RAM,
                                                                // lalu pindah ke alamat berikutnya.

    /* Zero fill the bss segment. */
    // Bagian ini mengisi semua byte di bagian '.bss' dengan nilai nol.
    // Bagian '.bss' berisi variabel global yang tidak diinisialisasi (misalnya `int counter;`).
    // Di file program yang disimpan di Flash, variabel ini tidak memakan tempat,
    // tetapi di RAM, mereka perlu dialokasikan dan diinisialisasi ke nol sesuai standar C.
    uint32_t *bss_begin = &_sbss; // Pointer ke awal bagian .bss di RAM.
    uint32_t *bss_end = &_ebss;   // Pointer ke akhir bagian .bss di RAM.
    while (bss_begin < bss_end) *bss_begin++ = 0; // Loop zero-filling:
                                                  // Tulis nilai 0 ke setiap lokasi di .bss.

    /* Clock system intitialization */
    // Memanggil fungsi untuk menginisialisasi sistem clock mikrokontroler.
    // Ini sangat penting karena tanpa clock yang benar, peripheral lain tidak akan berfungsi.
    rcc_clock_init();

    // Memanggil fungsi 'main()' dari kernel Anda.
    // Setelah semua setup dasar (copy .data, zero .bss, inisialisasi clock) selesai,
    // kontrol program akan diserahkan ke fungsi 'main' Anda, tempat OS Anda mulai berjalan.
    main();
}

/*
 * Handler Exception/Interrupt Default:
 * Ini adalah fungsi-fungsi yang akan dipanggil jika terjadi Exception (kesalahan) atau Interrupt
 * yang tidak ditangani secara spesifik.
 * Untuk OS minimal, seringkali mereka hanya berupa loop tak terbatas agar CPU berhenti
 * jika terjadi kesalahan yang tidak diharapkan.
 */
void nmi_handler(void) // Non-Maskable Interrupt (NMI) handler.
{                      // Ini adalah interrupt prioritas tinggi yang tidak bisa diabaikan.
    while (1);         // Loop tak terbatas: Jika NMI terjadi, CPU akan diam di sini.
}

void hardfault_handler(void) // Hard Fault handler.
{                            // Ini adalah kesalahan fatal yang terjadi di CPU (misalnya, akses memori ilegal).
    while (1);               // Loop tak terbatas: Jika Hard Fault terjadi, CPU akan diam di sini.
}

/*
 * Vektor Interrupt (Interrupt Service Routine - ISR Vector Table)
 * Ini adalah tabel alamat fungsi yang sangat penting untuk CPU ARM Cortex-M.
 * Ketika terjadi reset, interrupt, atau exception, CPU akan melihat tabel ini
 * untuk mengetahui fungsi mana yang harus dipanggil.
 *
 * __attribute((section(".isr_vector"))) memberitahu linker untuk menempatkan array ini
 * pada section memori khusus bernama ".isr_vector", yang harus ada di alamat tertentu
 * (biasanya 0x00000000) agar CPU bisa menemukannya saat startup.
 */
__attribute((section(".isr_vector")))
uint32_t *isr_vectors[] = {
    (uint32_t *)&_estack,         /* 0x00: Initial Stack Pointer (SP) value. CPU akan memuat nilai ini ke SP saat reset. */
    (uint32_t *)reset_handler,    /* 0x04: Reset Handler. Alamat fungsi yang dipanggil setelah reset. */
    (uint32_t *)nmi_handler,      /* 0x08: NMI Handler. Alamat fungsi yang dipanggil saat NMI terjadi. */
    (uint32_t *)hardfault_handler /* 0x0C: Hard Fault Handler. Alamat fungsi yang dipanggil saat Hard Fault terjadi. */
    // Biasanya ada banyak handler lain setelah ini (Memory Management, Bus Fault, SVC, SysTick, dll.),
    // dan juga handler untuk interrupt peripheral (USART, GPIO, Timer, dll.).
    // Untuk OS minimal, hanya beberapa yang paling krusial yang didefinisikan.
};

/*
 * rcc_clock_init: Fungsi Inisialisasi Sistem Clock RCC
 * Fungsi ini mengkonfigurasi frekuensi clock utama mikrokontroler.
 * Kode ini tampaknya mengkonfigurasi mikrokontroler untuk menggunakan
 * osilator kristal eksternal (HSE - High Speed External) sebagai sumber clock sistem.
 */
void rcc_clock_init(void)
{
    /* Reset the RCC clock configuration to the default reset state(for debug purpose) */
    // Baris ini mereset register RCC_CR ke nilai defaultnya.
    // Ini adalah praktik baik untuk memastikan konfigurasi bersih sebelum diubah.
    /* Set HSION bit */
    *RCC_CR |= (uint32_t)0x00000001; // Mengaktifkan osilator internal (HSI - High Speed Internal) sementara.
                                     // (0x00000001 adalah RCC_CR_HSION).
                                     // Ini mungkin dilakukan untuk memastikan ada clock yang berjalan
                                     // saat HSE (eksternal) diaktifkan dan distabilkan.

    /* Reset SW, HPRE, PPRE1, PPRE2, ADCPRE and MCO bits */
    // Mereset bit-bit tertentu di RCC_CFGR untuk mengembalikan ke nilai default.
    // Ini untuk memastikan tidak ada konfigurasi clock sebelumnya yang mengganggu.
    // 0xF8FF0000 adalah mask untuk membersihkan bit-bit yang relevan.
    *RCC_CFGR &= (uint32_t)0xF8FF0000;

    /* Reset HSEON, CSSON and PLLON bits */
    // Mereset bit-bit kontrol osilator eksternal (HSE), Clock Security System (CSS),
    // dan PLL (Phase Locked Loop). Ini memastikan mereka dalam keadaan mati sebelum dikonfigurasi ulang.
    // 0xFEF6FFFF adalah mask untuk membersihkan bit-bit tersebut.
    *RCC_CR &= (uint32_t)0xFEF6FFFF;

    /* Reset HSEBYP bit */
    // Mereset bit HSEBYP (HSE Bypass). Ini terkait dengan penggunaan osilator kristal vs. clock eksternal langsung.
    *RCC_CR &= (uint32_t)0xFFFBFFFF;

    /* Reset PLLSRC, PLLXTPRE, PLLMUL and USBPRE/OTGFSPRE bits */
    // Mereset bit-bit yang terkait dengan konfigurasi PLL (Phase Locked Loop) dan USB/OTG Prescaler.
    // PLL digunakan untuk mengalikan frekuensi clock untuk mencapai kecepatan yang lebih tinggi.
    *RCC_CFGR &= (uint32_t)0xFF80FFFF;

    /* Disable all interrupts and clear pending bits */
    // Menonaktifkan semua interrupt clock dan membersihkan semua bendera interrupt yang mungkin tertunda.
    // Ini memastikan kita memulai dengan kondisi interrupt yang bersih.
    *RCC_CIR = 0x009F0000;

    /* Configure the System clock frequency, HCLK, PCLK2 and PCLK1 prescalers */
    /* Configure the Flash Latency cycles and enable prefetch buffer */
    volatile uint32_t StartUpCounter = 0, HSEStatus = 0; // Variabel untuk menghitung startup timeout HSE.

    /* SYSCLK, HCLK, PCLK2 and PCLK1 configuration ---------------------------*/
    /* Enable HSE */
    // Mengaktifkan osilator kristal eksternal (HSE).
    // Ini adalah sumber clock yang lebih akurat dan seringkali lebih cepat dari HSI (internal).
    *RCC_CR |= (uint32_t)RCC_CR_HSEON; // Set bit HSEON di RCC_CR.

    /* Wait till HSE is ready and if Time out is reached exit */
    // Loop ini menunggu sampai osilator HSE stabil dan siap digunakan.
    // RCC_CR_HSERDY akan diset oleh hardware ketika HSE sudah stabil.
    do {
        HSEStatus = *RCC_CR & RCC_CR_HSERDY; // Membaca status HSE.
        StartUpCounter++; // Menambah counter.
    } while ((HSEStatus == 0) && (StartUpCounter != HSE_STARTUP_TIMEOUT)); // Loop selama HSE belum ready DAN belum timeout.

    // Memeriksa status akhir HSE setelah loop.
    if ((*RCC_CR & RCC_CR_HSERDY) != 0) // Jika bit HSERDY set (HSE ready)
        HSEStatus = (uint32_t)0x01;     // Set status ke 1 (berhasil).
    else
        HSEStatus = (uint32_t)0x00;     // Set status ke 0 (gagal).

    if (HSEStatus == (uint32_t)0x01) { // Jika HSE berhasil startup:
        /* Enable Prefetch Buffer */
        // Mengaktifkan Prefetch Buffer pada Flash. Ini membantu CPU mengambil instruksi/data
        // dari Flash lebih cepat, karena Flash lebih lambat dari CPU.
        *FLASH_ACR |= FLASH_ACR_PRFTBE;

        /* Flash 0 wait state */
        // Mengatur Latency (wait states) untuk Flash memori.
        // Latency 0 (0 wait state) berarti tidak ada penundaan saat mengakses Flash.
        // Ini biasanya hanya cocok untuk frekuensi CPU yang sangat rendah.
        // Untuk frekuensi yang lebih tinggi (misalnya 72MHz), biasanya butuh 2 wait states.
        // Kode ini mengatur 0 wait state, yang mungkin tidak cocok untuk frekuensi default STM32.
        *FLASH_ACR &= (uint32_t)((uint32_t)~FLASH_ACR_LATENCY); // Bersihkan bit LATENCY.
        *FLASH_ACR |= (uint32_t)FLASH_ACR_LATENCY_0; // Set bit LATENCY ke 0.

        /* HCLK = SYSCLK */
        // Mengatur prescaler untuk bus AHB (HCLK). HCLK adalah clock untuk CPU.
        // RCC_CFGR_HPRE_DIV1 (0x00000000) berarti SYSCLK tidak dibagi (divide by 1),
        // jadi HCLK sama dengan SYSCLK.
        *RCC_CFGR |= (uint32_t)RCC_CFGR_HPRE_DIV1;

        /* PCLK2 = HCLK */
        // Mengatur prescaler untuk bus APB2 (PCLK2). Peripheral di APB2 (GPIO, USART1)
        // akan berjalan pada kecepatan PCLK2.
        // RCC_CFGR_PPRE2_DIV1 (0x00000000) berarti HCLK tidak dibagi, jadi PCLK2 = HCLK.
        *RCC_CFGR |= (uint32_t)RCC_CFGR_PPRE2_DIV1;

        /* PCLK1 = HCLK */
        // Mengatur prescaler untuk bus APB1 (PCLK1). Peripheral di APB1 (USART2, Timer)
        // akan berjalan pada kecepatan PCLK1.
        // RCC_CFGR_PPRE1_DIV1 (0x00000000) berarti HCLK tidak dibagi, jadi PCLK1 = HCLK.
        *RCC_CFGR |= (uint32_t)RCC_CFGR_PPRE1_DIV1;

        /* Select HSE as system clock source */
        // Memilih HSE (osilator eksternal) sebagai sumber clock utama (System Clock / SYSCLK).
        *RCC_CFGR &= (uint32_t)((uint32_t) ~(RCC_CFGR_SW)); // Bersihkan bit SW (System Clock Switch).
        *RCC_CFGR |= (uint32_t)RCC_CFGR_SW_HSE;           // Set bit SW untuk memilih HSE.

        /* Wait till HSE is used as system clock source */
        // Loop ini menunggu sampai sistem clock benar-benar beralih menggunakan HSE.
        // RCC_CFGR_SWS akan mencerminkan sumber clock yang sedang aktif.
        // (uint32_t) 0x04 adalah nilai yang menandakan bahwa HSE sedang digunakan sebagai SYSCLK.
        while ((*RCC_CFGR & (uint32_t)RCC_CFGR_SWS) != (uint32_t)0x04);
    } else {
        /* If HSE fails to start-up, the application will have wrong clock
         * configuration. User can add here some code to deal with this error */
        // Blok ini dieksekusi jika HSE gagal startup.
        // Dalam OS produksi, Anda akan menambahkan kode di sini untuk menangani kesalahan,
        // misalnya beralih kembali ke osilator internal, atau memicu indikasi error.
        // Untuk OS minimal, ini seringkali dibiarkan kosong atau hanya loop tak terbatas.
    }
}