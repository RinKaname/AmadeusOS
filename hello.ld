// ENTRY(reset_handler) :
// Ini memberitahu linker bahwa 'reset_handler' adalah titik masuk pertama (entry point)
// untuk program Anda setelah CPU di-reset. Saat mikrokontroler booting,
// dia akan mencari alamat ini di Vector Table dan mulai eksekusi dari sini.
// (Pada ARM Cortex-M, ini biasanya elemen kedua di tabel vektor interrupt).
ENTRY(reset_handler)

// MEMORY: Blok ini mendefinisikan region-region memori fisik yang ada pada mikrokontroler target Anda.
// Anda harus tahu ini dari datasheet atau Reference Manual mikrokontroler.
MEMORY
{
	// FLASH (rx):
	// - FLASH: Nama region memori (seperti hard drive di PC, tapi ini Read-Only Memory/ROM).
	//   Ini adalah tempat kode program (executable code) dan data read-only disimpan secara permanen.
	// - (rx): Atribut akses untuk region ini:
	//   - r: readable (bisa dibaca)
	//   - x: executable (bisa dieksekusi)
	// - ORIGIN = 0x08000000: Alamat awal (base address) dari memori Flash.
	//   Ini adalah alamat fisik di mana program Anda akan dimuat.
	// - LENGTH = 128K: Ukuran total memori Flash yang tersedia (128 Kilobyte).
	FLASH (rx) : ORIGIN = 0x08000000, LENGTH = 128K

	// RAM (rwx):
	// - RAM: Nama region memori (seperti RAM di PC, ini Random Access Memory).
	//   Ini adalah tempat data yang bisa diubah (variabel global, stack, heap) disimpan saat program berjalan.
	// - (rwx): Atribut akses untuk region ini:
	//   - r: readable (bisa dibaca)
	//   - w: writable (bisa ditulis/diubah)
	//   - x: executable (bisa dieksekusi - kadang RAM bisa dieksekusi untuk self-modifying code atau bootloader tahap 2, tapi tidak selalu)
	// - ORIGIN = 0x20000000: Alamat awal dari memori RAM (SRAM).
	// - LENGTH = 40K: Ukuran total memori RAM yang tersedia (40 Kilobyte).
	RAM (rwx) : ORIGIN = 0x20000000, LENGTH = 40K
}

// SECTIONS: Blok ini mendefinisikan bagaimana bagian-bagian (sections) dari program Anda
// akan disusun dan ditempatkan ke dalam region memori yang sudah didefinisikan di atas.
// Setiap file .c atau .s yang Anda kompilasi akan menghasilkan beberapa section
// (misalnya .text, .data, .rodata, .bss).
SECTIONS
{
	// .text : Bagian ini berisi kode program yang bisa dieksekusi (instruksi CPU).
	.text :
	{
		// KEEP(*(.isr_vector)):
		// - .isr_vector: Ini adalah section khusus yang berisi Tabel Vektor Interrupt.
		//   Tabel ini sangat penting untuk startup ARM Cortex-M karena berisi alamat reset_handler
		//   dan handler-handler penting lainnya.
		// - KEEP(*(...)): Instruksi ini memberitahu linker untuk "menyimpan" section ini.
		//   Artinya, meskipun linker mungkin tidak melihat referensi langsung dari kode C lainnya
		//   ke setiap entri di .isr_vector, ia tidak boleh menghapusnya (tidak boleh dioptimasi).
		//   Ini krusial agar Tabel Vektor Interrupt tidak hilang dari binary final.
		KEEP(*(.isr_vector))

		// *(.text): Mengumpulkan semua section .text dari semua file objek (.o).
		// *(.text.*): Mengumpulkan semua sub-section dari .text (misalnya .text.main, .text.print_str).
		//   Ini memastikan semua kode program dimasukkan.
		*(.text)
		*(.text.*)

		// *(.rodata): Mengumpulkan semua section .rodata (read-only data).
		//   Ini termasuk string literal (seperti "Welcome to Amadeus OS.\n" jika tidak dideklarasikan sebagai variabel global).
		//   Di linker script ini, .rodata ditempatkan LANGSUNG SETELAH kode program (.text) di Flash.
		*(.rodata)

		// _sromdev, _eromdev, _sidata:
		// Ini adalah simbol (variabel) yang didefinisikan oleh linker.
		// Tanda '.' (dot) merepresentasikan lokasi saat ini dari linker counter.
		// - _sromdev = .; : Menandai alamat awal dari area data di ROM (Read-Only Memory)
		//   (sebagai "ROM device").
		// - _eromdev = .; : Menandai alamat akhir dari area data di ROM.
		// - _sidata = .; : Ini adalah ALAMAT MUAT (Load Memory Address / LMA)
		//   untuk section .data. Ini adalah tempat nilai awal variabel .data
		//   disimpan di Flash (ROM) sebelum disalin ke RAM.
		_sromdev = .;
		_eromdev = .;
		_sidata = .; // Ini adalah titik krusial yang menghubungkan .text ke .data.
	} >FLASH // Simbol >FLASH berarti semua section yang didefinisikan di dalam kurung kurawal ini
	         // (yaitu .isr_vector, .text, .rodata, dan simbol-simbolnya)
	         // akan DITEMPATKAN di region memori FLASH.

	// .data : Bagian ini berisi variabel global yang diinisialisasi (punya nilai awal).
	.data : AT(_sidata) // AT(_sidata) adalah bagian TERPENTING di sini.
	{                   // Ini adalah ALAMAT MUATAN (Load Memory Address / LMA).
	                    // Ini memberitahu linker bahwa nilai awal dari variabel-variabel di section .data
	                    // akan ditemukan pada alamat _sidata (yaitu di dalam Flash, setelah .rodata).
	                    // Ini adalah "lokasi penyimpanan" mereka di file binary.

		_sdata = .; // _sdata: Alamat awal NYATA (Virtual Memory Address / VMA) dari section .data di RAM.
		            // Ini adalah tempat variabel .data akan BERADA saat program berjalan.
		*(.data)    // Mengumpulkan semua section .data dari semua file objek.
		*(.data*)   // Mengumpulkan semua sub-section .data.
		_edata = .; // _edata: Alamat akhir NYATA dari section .data di RAM.
	} >RAM // Simbol >RAM berarti section .data ini (setelah disalin oleh startup.c)
	       // akan BERADA dan berjalan di region memori RAM.

	// .bss : Bagian ini berisi variabel global yang TIDAK diinisialisasi (nilainya nol secara default).
	// Contoh: `int counter;`
	// Section ini tidak disimpan di Flash untuk menghemat ruang, hanya dialokasikan di RAM
	// dan diisi dengan nol oleh `reset_handler` di `startup.c`.
	.bss :
	{
		_sbss = .; // _sbss: Alamat awal section .bss di RAM.
		*(.bss)    // Mengumpulkan semua section .bss dari semua file objek.
		_ebss = .; // _ebss: Alamat akhir section .bss di RAM.
	} >RAM // Simbol >RAM berarti section .bss ini akan BERADA dan berjalan di region memori RAM.

	// _estack: Simbol ini menandai alamat puncak stack (alamat memori tertinggi dari stack).
	// Stack tumbuh ke bawah (dari alamat tinggi ke alamat rendah) pada ARM Cortex-M.
	// ORIGIN(RAM) adalah alamat awal RAM, LENGTH(RAM) adalah panjang RAM.
	// Jadi, _estack = (Alamat Awal RAM + Panjang RAM) = Alamat Puncak RAM.
	// Nilai ini akan digunakan oleh CPU untuk menginisialisasi Stack Pointer (SP) saat reset.
	_estack = ORIGIN(RAM) + LENGTH(RAM);
}