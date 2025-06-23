# Makefile ini digunakan untuk mengkompilasi, me-link, dan menjalankan program bare-metal/kernel
# untuk arsitektur ARM Cortex-M3 (terutama untuk board STM32-P103) menggunakan QEMU.

# ==============================================================================
# 1. Konfigurasi Toolchain
# ==============================================================================

# CROSS_COMPILE:
# Ini mendefinisikan prefix (awalan) untuk nama-nama toolchain (compiler, linker, objcopy, dll.).
# 'arm-none-eabi-' adalah prefix standar untuk GNU cross-toolchain yang menargetkan
# arsitektur ARM, tanpa sistem operasi (none), dan menggunakan antarmuka binary ELF (eabi).
# Ini penting karena Anda mengkompilasi kode untuk ARM di mesin yang mungkin x86-64 atau ARM lain.
CROSS_COMPILE ?= arm-none-eabi-

# CC (C Compiler):
# Ini mendefinisikan perintah lengkap untuk compiler C.
# '$ (CROSS_COMPILE)gcc' akan menjadi 'arm-none-eabi-gcc'.
# Ini adalah cross-compiler yang akan mengubah kode C menjadi instruksi ARM.
CC := $(CROSS_COMPILE)gcc

# CFLAGS (C Compiler Flags):
# Ini adalah opsi-opsi yang akan dilewatkan ke compiler C saat kompilasi.
# Setiap flag memiliki tujuan spesifik untuk pengembangan bare-metal.
CFLAGS = -fno-common \
         -ffreestanding \
         -O0 \
         -gdwarf-2 -g3 \
         -Wall -Werror \
         -mcpu=cortex-m3 \
         -mthumb \
         -Wl,-Thello.ld \
         -nostartfiles

    # -fno-common:
    #   Memastikan variabel global yang tidak diinisialisasi (common symbols) ditempatkan
    #   di bagian .bss (Block Started by Symbol) memori, bukan di area "common" yang bisa bermasalah
    #   di lingkungan bare-metal.
    # -ffreestanding:
    #   Memberitahu compiler bahwa kode ini akan berjalan di lingkungan 'freestanding' (tanpa OS).
    #   Ini berarti compiler tidak akan secara otomatis menyertakan library standar C (seperti glibc)
    #   atau asumsi-asumsi OS lainnya. Anda harus menyediakan semua yang dibutuhkan sendiri.
    # -O0:
    #   Tingkat optimasi 0 (tidak ada optimasi).
    #   Ini sangat berguna selama tahap pengembangan dan debugging karena kode yang dihasilkan
    #   akan lebih mudah dipetakan kembali ke kode sumber C. (Kebalikan dari -O2 yang ada di Makefile Otternix)
    # -gdwarf-2 -g3:
    #   Menghasilkan informasi debugging dalam format DWARF (versi 2 dan tingkat 3).
    #   Ini penting agar debugger (misalnya GDB) bisa memahami kode sumber Anda,
    #   melihat nilai variabel, dan melacak alur eksekusi saat debugging.
    # -Wall -Werror:
    #   -Wall: Mengaktifkan hampir semua peringatan (warnings) yang direkomendasikan.
    #   -Werror: Memperlakukan semua peringatan sebagai kesalahan (errors).
    #     Jika ada peringatan, kompilasi akan gagal. Ini adalah praktik bagus untuk memastikan
    #     kode bersih dari masalah potensial.
    # -mcpu=cortex-m3:
    #   Menentukan arsitektur CPU target adalah ARM Cortex-M3. Compiler akan menghasilkan
    #   instruksi yang spesifik untuk CPU ini.
    # -mthumb:
    #   Mengkompilasi kode untuk menggunakan set instruksi Thumb. Thumb adalah set instruksi 16-bit
    #   yang lebih padat (menghemat ruang kode) dibandingkan set instruksi ARM 32-bit standar.
    #   Cortex-M3 umumnya mendukung Thumb-2.
    # -Wl,-Thello.ld:
    #   'Wl,' memberitahu compiler untuk meneruskan flag '-T' ke linker.
    #   '-Thello.ld': Menentukan bahwa linker harus menggunakan file 'hello.ld' sebagai linker script.
    #     Linker script ini (mirip dengan 'linker.ld' yang sudah kita bahas) akan menentukan bagaimana
    #     kode dan data akan diatur di dalam memori mikrokontroler.
    # -nostartfiles:
    #   Memberitahu linker untuk tidak menyertakan file startup standar (seperti crt0.o).
    #   Ini penting karena Anda menyediakan file startup sendiri ('startup.c') yang menginisialisasi
    #   lingkungan runtime C (menyalin .data, mengisi .bss) sebelum memanggil main().

# ==============================================================================
# 2. Target Utama
# ==============================================================================

# TARGET:
# Mendefinisikan nama file binary akhir yang akan dihasilkan, yaitu 'hello.bin'.
TARGET= hello.bin

# all:
# Ini adalah target default. Ketika Anda mengetik 'make' tanpa argumen,
# Makefile akan mencoba membangun target 'all', yang pada gilirannya akan
# membangun '$ (TARGET)' (yaitu 'hello.bin').
all: $(TARGET)

# ==============================================================================
# 3. Aturan Build untuk TARGET
# ==============================================================================

# $(TARGET): hello.c startup.c
# Ini adalah aturan utama untuk membangun 'hello.bin'.
# Target: hello.bin (file yang ingin kita buat)
# Prasyarat: hello.c dan startup.c (file-file sumber yang dibutuhkan untuk membuat target)
$(TARGET): hello.c startup.c
	# @echo "Mengkompilasi dan me-link..." (Biasanya ada pesan ini, di sini tidak ada)

	# Langkah 1: Kompilasi dan Link ke file ELF
	# $(CC) $(CFLAGS) $^ -o hello.elf
	# - $(CC): Memanggil cross-compiler (arm-none-eabi-gcc).
	# - $(CFLAGS): Meneruskan semua flag kompilasi yang sudah didefinisikan di atas.
	# - $^: Ini adalah variabel otomatis Makefile yang mewakili SEMUA prasyarat ('hello.c' dan 'startup.c').
	# - -o hello.elf: Menentukan nama file output adalah 'hello.elf'.
	#   'ELF' (Executable and Linkable Format) adalah format standar untuk file executable
	#   di sistem Unix-like, termasuk Linux dan toolchain GNU.
	$(CC) $(CFLAGS) $^ -o hello.elf

	# Langkah 2: Konversi ELF ke Raw Binary
	# $(CROSS_COMPILE)objcopy -Obinary hello.elf hello.bin
	# - $(CROSS_COMPILE)objcopy: Tool dari binutils untuk menyalin (copy) dan mengkonversi format objek.
	# - -Obinary: Outputkan file dalam format raw binary (mentah), tanpa header atau metadata ELF.
	#   Ini penting karena mikrokontroler (atau QEMU dalam mode '-kernel') seringkali
	#   membutuhkan binary mentah yang langsung bisa dimuat ke memori Flash.
	# - hello.elf: File input (ELF yang baru dibuat).
	# - hello.bin: File output (binary mentah yang siap di-flash atau dijalankan QEMU).
	$(CROSS_COMPILE)objcopy -Obinary hello.elf hello.bin

	# Langkah 3: Disassemble ELF ke Listing File
	# $(CROSS_COMPILE)objdump -S hello.elf > hello.list
	# - $(CROSS_COMPILE)objdump: Tool dari binutils untuk menampilkan informasi dari file objek.
	# - -S: Menyisipkan kode sumber C/Assembly yang sudah di-disassemble (disassembler output with source).
	# - hello.elf: File input.
	# - > hello.list: Mengalihkan output ke file 'hello.list'.
	#   File '.list' ini sangat berguna untuk debugging. Anda bisa melihat kode C Anda
	#   diterjemahkan menjadi instruksi Assembly ARM, membantu Anda memahami bagaimana compiler bekerja
	#   dan jika ada masalah di level instruksi.
	$(CROSS_COMPILE)objdump -S hello.elf > hello.list

# ==============================================================================
# 4. Target Utility: QEMU
# ==============================================================================

# qemu: $(TARGET)
# Target ini digunakan untuk menjalankan 'hello.bin' di emulator QEMU.
# Prasyarat: $(TARGET) (yaitu 'hello.bin' harus sudah berhasil dibuat).
qemu: $(TARGET)
	# @qemu-system-arm -M ? | grep stm32-p103 >/dev/null || exit
	# Ini adalah baris pengecekan.
	# - qemu-system-arm -M ?: List semua jenis mesin ARM yang didukung QEMU.
	# - | grep stm32-p103: Mencari string "stm32-p103" di hasil list tersebut.
	# - >/dev/null: Mengalihkan output grep agar tidak ditampilkan di konsol.
	# - || exit: Jika 'grep' tidak menemukan string (return code bukan 0), maka perintah akan exit
	#   (keluar), artinya QEMU Anda tidak mendukung board STM32-P103.
	#   Ini memastikan Anda tidak mencoba menjalankan QEMU dengan board yang tidak dikenali.
	@qemu-system-arm -M ? | grep stm32-p103 >/dev/null || exit

	# Pesan informatif untuk pengguna cara keluar dari QEMU.
	@echo "Press Ctrl-A and then X to exit QEMU"
	@echo

	# Menjalankan emulator QEMU.
	# - qemu-system-arm: Memanggil emulator QEMU untuk sistem ARM.
	# - -M stm32-p103: Menentukan bahwa QEMU harus mengemulasikan board spesifik 'stm32-p103'.
	#   Ini penting agar QEMU mensimulasikan hardware (peripheral) yang benar
	#   sehingga kode Anda bisa berinteraksi dengannya.
	# - -nographic: Menjalankan QEMU tanpa output grafis (tidak ada jendela emulator).
	#   Output serial (seperti dari USART di kode C Anda) akan muncul di terminal yang sama.
	# - -kernel hello.bin: Memberitahu QEMU untuk memuat file 'hello.bin' sebagai kernel
	#   dan menjalankannya langsung. QEMU akan menangani bootloader minimal.
	qemu-system-arm -M stm32-p103 -nographic -kernel hello.bin

# ==============================================================================
# 5. Target Utility: Clean
# ==============================================================================

# clean:
# Target ini untuk membersihkan semua file yang dihasilkan selama proses build.
# Ini berguna untuk memastikan Anda memulai dari awal pada build berikutnya.
clean:
	# rm -f *.o *.bin *.elf *.list
	# - rm -f: Perintah untuk menghapus file secara paksa ('-f' untuk force, tidak tanya konfirmasi).
	# - *.o: Menghapus semua file objek (.o) yang dihasilkan dari kompilasi.
	# - *.bin: Menghapus file binary akhir ('hello.bin').
	# - *.elf: Menghapus file ELF ('hello.elf').
	# - *.list: Menghapus file listing ('hello.list').
	rm -f *.o *.bin *.elf *.list