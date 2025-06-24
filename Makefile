# Makefile ini digunakan untuk mengkompilasi, me-link, dan menjalankan program bare-metal/kernel
# untuk arsitektur ARM Cortex-M3 (terutama untuk board LM3S6965evb) menggunakan QEMU.

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

# ==============================================================================
# 2. Definisi File dan Target
# ==============================================================================

# TARGET:
# Mendefinisikan nama file binary akhir yang akan dihasilkan, yaitu 'hello.bin'.
TARGET = hello.bin

# SOURCES:
# Daftar semua file sumber .c yang dibutuhkan untuk proyek ini.
SOURCES = hello.c startup.c uart.c utils.c

# OBJS:
# Mengubah daftar file .c menjadi daftar file .o (objek).
# Ini adalah praktik yang lebih baik untuk Makefile yang lebih besar.
OBJS = $(SOURCES:.c=.o)

# ==============================================================================
# 3. Target Utama dan Aturan Build
# ==============================================================================

# all:
# Ini adalah target default. Ketika Anda mengetik 'make' tanpa argumen,
# Makefile akan mencoba membangun target 'all', yang pada gilirannya akan
# membangun '$ (TARGET)' (yaitu 'hello.bin').
all: $(TARGET)

# Aturan utama untuk membangun 'hello.bin'.
# Target: hello.bin
# Prasyarat: hello.elf (file ELF harus ada terlebih dahulu)
$(TARGET): hello.elf
	# Langkah 2: Konversi ELF ke Raw Binary
	$(CROSS_COMPILE)objcopy -Obinary hello.elf $(TARGET)
	# Langkah 3: Disassemble ELF ke Listing File
	$(CROSS_COMPILE)objdump -S hello.elf > hello.list

# Aturan untuk me-link semua file objek menjadi satu file ELF.
# Target: hello.elf
# Prasyarat: Semua file objek (.o) yang didefinisikan dalam $(OBJS).
hello.elf: $(OBJS)
	# Langkah 1: Link semua file objek menjadi hello.elf
	$(CC) $(CFLAGS) $^ -o $@

# Aturan Pola (Pattern Rule) untuk mengkompilasi setiap file .c menjadi file .o.
# Ini memberitahu 'make' cara membuat file .o dari file .c yang sesuai.
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# ==============================================================================
# 4. Target Utility: QEMU
# ==============================================================================

# qemu: $(TARGET)
# Target ini digunakan untuk menjalankan 'hello.bin' di emulator QEMU.
# Prasyarat: $(TARGET) (yaitu 'hello.bin' harus sudah berhasil dibuat).
qemu: $(TARGET)
	# Memeriksa apakah QEMU mendukung board lm3s6965evb
	@qemu-system-arm -M ? | grep lm3s6965evb >/dev/null || exit
	# Pesan informatif untuk pengguna cara keluar dari QEMU.
	@echo "Press Ctrl-A and then X to exit QEMU"
	@echo
	# Menjalankan emulator QEMU.
	qemu-system-arm -M lm3s6965evb -nographic -kernel $(TARGET)

# ==============================================================================
# 5. Target Utility: Clean
# ==============================================================================

# clean:
# Target ini untuk membersihkan semua file yang dihasilkan selama proses build.
clean:
	# Menghapus semua file objek, binary, ELF, dan list.
	rm -f *.o *.bin *.elf *.list
