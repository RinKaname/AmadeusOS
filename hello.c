#include <stdint.h> // Standard C header for fixed-width integer types (e.g., uint32_t, uint16_t).
#include "reg.h"    // Custom header defining hardware register addresses, now updated for LM3S6965evb.
#include <stddef.h> // Diperlukan untuk definisi NULL (size_t) dan NULL pointer.

// Definisi karakter kontrol umum
#define ASCII_CR    0x0D // Carriage Return (Enter)
#define ASCII_LF    0x0A // Line Feed (Newline)
#define ASCII_BS    0x08 // Backspace
#define ASCII_DEL   0x7F // Delete

// Ukuran buffer untuk input baris
#define MAX_LINE_LENGTH 128

// String pesan yang akan ditampilkan.
static char greet[] = "Welcome to Amadeus OS v0.7.5! ^_^\n";

// ============================================================================
// Deklarasi Eksternal dari Linker Script untuk Heap
// ============================================================================
extern uint32_t __heap_start__;
extern uint32_t __heap_end__;

// ============================================================================
// Deklarasi Prototype Fungsi-fungsi Kustom
// ============================================================================
void uart0_putc(char c);
char uart0_getc(void);
void print_str(const char *str);
int strcmp(const char *s1, const char *s2);
int strlen(const char *s);
void strncpy(char *dest, const char *src, int n);
void clear_screen(void);
void readline(char *buffer, int max_len);
int atoi(const char *s, const char **endptr);
void itoa(int n, char *s);
void uart0_init(void);

// Helper function prototypes
void* memset(void *s, int c, size_t n);
void* memcpy(void *dest, const void *src, size_t n);
void print_hex(uint32_t n);
uint32_t htoi(const char *s, const char **endptr);
void panic(const char* message);

// Deklarasi Prototype untuk Heap Manager
void malloc_init(void);
void* malloc(size_t size);
void free(void* ptr);
void* calloc(size_t num, size_t size);
void* realloc(void* ptr, size_t new_size);
void print_heap_map(void);


// ============================================================================
// Implementasi Heap Manager (malloc/free/calloc/realloc)
// ============================================================================

#define HEAP_MAGIC 0xCAFEBABE // Magic number untuk validasi blok

typedef struct BlockHeader {
    size_t size;
    int free;
    uint32_t magic; // Magic number untuk validasi
} BlockHeader;

#define ALIGN_SIZE(size) (((size) + (sizeof(uint32_t) - 1)) & ~(sizeof(uint32_t) - 1))
#define MIN_BLOCK_SIZE   (ALIGN_SIZE(sizeof(BlockHeader)))

void malloc_init(void) {
    BlockHeader* head = (BlockHeader*)&__heap_start__;
    head->size = (uint32_t)&__heap_end__ - (uint32_t)&__heap_start__;
    head->free = 1;
    head->magic = 0; // Blok awal tidak memiliki magic number
}

void* malloc(size_t size) {
    if (size == 0) return NULL;

    size_t required_size = ALIGN_SIZE(size + sizeof(BlockHeader));
    if (required_size < MIN_BLOCK_SIZE) {
        required_size = MIN_BLOCK_SIZE;
    }

    BlockHeader* current = (BlockHeader*)&__heap_start__;
    BlockHeader* best_fit = NULL;

    while((uint32_t)current < (uint32_t)&__heap_end__ && current->size > 0) {
        if (current->free && current->size >= required_size) {
            if (best_fit == NULL || current->size < best_fit->size) {
                 best_fit = current;
            }
        }
        current = (BlockHeader*)((uint8_t*)current + current->size);
    }
    
    current = best_fit;

    if (current != NULL) {
        if (current->size - required_size >= MIN_BLOCK_SIZE) {
            BlockHeader* new_block = (BlockHeader*)((uint8_t*)current + required_size);
            new_block->size = current->size - required_size;
            new_block->free = 1;
            new_block->magic = 0; // Blok bebas baru tidak punya magic
            current->size = required_size;
        }
        current->free = 0;
        current->magic = HEAP_MAGIC; // Set magic number saat alokasi
        return (void*)((uint8_t*)current + sizeof(BlockHeader));
    }
    return NULL;
}


void free(void* ptr) {
    if (ptr == NULL) return;

    // Cek apakah pointer berada di dalam rentang heap
    if ((uint32_t)ptr < (uint32_t)&__heap_start__ + sizeof(BlockHeader) || (uint32_t)ptr >= (uint32_t)&__heap_end__) {
        print_str("Error: Address is outside of heap boundary!\n");
        return;
    }

    BlockHeader* block_to_free = (BlockHeader*)((uint8_t*)ptr - sizeof(BlockHeader));

    // Validasi magic number
    if (block_to_free->magic != HEAP_MAGIC) {
        print_str("Error: Invalid pointer or heap corruption detected!\n");
        // Di OS nyata, ini bisa memicu panic()
        // panic("Heap corruption detected in free()");
        return;
    }

    if (block_to_free->free) {
        print_str("Warning: Double-free detected!\n");
        return;
    }

    block_to_free->free = 1;
    block_to_free->magic = 0; // Hapus magic number saat dibebaskan
    print_str("Freed memory at "); print_hex((uint32_t)ptr); print_str("\n");

    // Coalesce forward
    BlockHeader* next_block = (BlockHeader*)((uint8_t*)block_to_free + block_to_free->size);
    if ((uint32_t)next_block < (uint32_t)&__heap_end__ && next_block->free) {
        block_to_free->size += next_block->size;
    }
    
    // Coalesce backward
    BlockHeader* current = (BlockHeader*)&__heap_start__;
    while((uint32_t)current < (uint32_t)block_to_free) {
        BlockHeader* next = (BlockHeader*)((uint8_t*)current + current->size);
        if ((uint32_t)next == (uint32_t)block_to_free) {
             if(current->free){
                current->size += block_to_free->size;
                // Setelah digabung, `block_to_free` menjadi tidak relevan,
                // tapi memorinya sudah menjadi bagian dari `current`.
             }
             break;
        }
        current = next;
    }
}

void* calloc(size_t num, size_t size) {
    size_t total_size = num * size;
    if (total_size == 0) return NULL;
    void* ptr = malloc(total_size);
    if (ptr != NULL) {
        memset(ptr, 0, total_size);
    }
    return ptr;
}

void* realloc(void* ptr, size_t new_size) {
    if (ptr == NULL) return malloc(new_size);
    if (new_size == 0) {
        free(ptr);
        return NULL;
    }
    
    // Validasi magic number sebelum realloc
    BlockHeader* old_header = (BlockHeader*)((uint8_t*)ptr - sizeof(BlockHeader));
    if (old_header->magic != HEAP_MAGIC) {
        print_str("Error: Invalid pointer passed to realloc()!\n");
        return NULL;
    }

    size_t old_size = old_header->size - sizeof(BlockHeader);

    if (new_size <= old_size) return ptr;

    void* new_ptr = malloc(new_size);
    if (new_ptr == NULL) return NULL;

    memcpy(new_ptr, ptr, old_size);
    free(ptr);
    return new_ptr;
}

void print_heap_map(void) {
    const int MAP_WIDTH = 40;
    size_t total_heap_size = (uint32_t)&__heap_end__ - (uint32_t)&__heap_start__;
    
    print_str("Heap Map:\n[");

    BlockHeader* current = (BlockHeader*)&__heap_start__;
    while((uint32_t)current < (uint32_t)&__heap_end__ && current->size > 0) {
        int chars = (current->size * MAP_WIDTH) / total_heap_size;
        if (chars == 0) chars = 1;
        
        char block_char = current->free ? '-' : '#';
        for (int i = 0; i < chars; i++) {
            uart0_putc(block_char);
        }
        
        current = (BlockHeader*)((uint8_t*)current + current->size);
    }
    
    print_str("]\n");
    print_str("# = Allocated, - = Free\n");
}

// ============================================================================
// Implementasi Fungsi Helper
// ============================================================================
void panic(const char* message) {
    print_str("\n*** KERNEL PANIC ***\n");
    print_str(message);
    print_str("\nSystem halted.\n");
    while(1); // Halt the system
}

void* memset(void *s, int c, size_t n) {
    unsigned char *p = s;
    while (n--) *p++ = (unsigned char)c;
    return s;
}

void* memcpy(void *dest, const void *src, size_t n) {
    char *d = dest;
    const char *s = src;
    while (n--) *d++ = *s++;
    return dest;
}

void print_hex(uint32_t n) {
    char buffer[9]; // 8 hex digits + null terminator
    char *ptr = &buffer[8];
    *ptr = '\0';
    ptr--;
    
    if (n == 0) {
        print_str("0x0");
        return;
    }
    
    print_str("0x");
    while (n > 0) {
        uint8_t digit = n % 16;
        if (digit < 10) {
            *ptr-- = '0' + digit;
        } else {
            *ptr-- = 'a' + (digit - 10);
        }
        n /= 16;
    }
    
    print_str(ptr + 1);
}

uint32_t htoi(const char *s, const char **endptr) {
    uint32_t result = 0;
    if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) {
        s += 2;
    }
    while (*s) {
        char c = *s;
        if (c >= '0' && c <= '9') {
            result = result * 16 + (c - '0');
        } else if (c >= 'a' && c <= 'f') {
            result = result * 16 + (c - 'a' + 10);
        } else if (c >= 'A' && c <= 'F') {
            result = result * 16 + (c - 'A' + 10);
        } else {
            break;
        }
        s++;
    }
    if (endptr) *endptr = s;
    return result;
}


void uart0_init(void) {
    *(SYSCTL_RCGC2) |= SYSCTL_RCGC2_GPIOA;
    *(SYSCTL_RCGCUART) |= SYSCTL_RCGCUART_UART0;
    *(UART0_CTL) &= ~(UART0_CTL_UARTEN);
    *(GPIOA_AFSEL) |= ((1 << 0) | (1 << 1));
    *(GPIOA_PCTL) &= ~0x000000FF;
    *(GPIOA_PCTL) |= 0x00000011;
    *(GPIOA_DEN) |= ((1 << 0) | (1 << 1));
    *(UART0_IBRD) = 27;
    *(UART0_FBRD) = 8;
    *(UART0_LCRH) = (UART0_LCRH_WLEN_8 | UART0_LCRH_FEN);
    *(UART0_CTL) = (UART0_CTL_UARTEN | UART0_CTL_TXE | UART0_CTL_RXE);
}

void main(void) {
    char line_buffer[MAX_LINE_LENGTH];
    char temp_str[32];

    uart0_init();
    malloc_init();

    print_str(greet);

    while (1) {
        print_str("AmadeusOS> ");
        readline(line_buffer, MAX_LINE_LENGTH);

        char command_name[16];
        const char *args_ptr = line_buffer;

        while (*args_ptr == ' ') args_ptr++;

        int i = 0;
        while (args_ptr[i] != ' ' && args_ptr[i] != '\0' && i < 15) {
            if (args_ptr[i] >= 'A' && args_ptr[i] <= 'Z') {
                command_name[i] = args_ptr[i] + ('a' - 'A');
            } else {
                command_name[i] = args_ptr[i];
            }
            i++;
        }
        command_name[i] = '\0';

        if (args_ptr[i] == ' ') {
            args_ptr += i + 1;
            while (*args_ptr == ' ') args_ptr++;
        } else {
            args_ptr += i;
        }

        if (strcmp(command_name, "help") == 0) {
            print_str("Available commands:\n");
            print_str("  help               - Display this help message\n");
            print_str("  echo <text>        - Echoes back the input\n");
            print_str("  clear              - Clears the terminal screen\n");
            print_str("  meminfo            - Display heap memory information\n");
            print_str("  heapmap            - Display a visual map of the heap\n");
            print_str("  alloc <size>       - Allocate memory from heap\n");
            print_str("  calloc <n> <size>  - Allocate and zero-initialize memory\n");
            print_str("  realloc <addr> <sz>- Reallocate memory\n");
            print_str("  free <addr>        - Free memory from heap (e.g., free 0x20000100)\n");
            print_str("  peek <addr>        - Read a 32-bit value from a memory address\n");
            print_str("  poke <addr> <val>  - Write a 32-bit value to a memory address\n");
            print_str("  fill <addr> <val> <count> - Fill memory with a value\n");
            print_str("  panic_test         - Test the kernel panic handler\n");
            print_str("  exit               - Exits the QEMU emulator\n");
        } else if (strcmp(command_name, "clear") == 0) {
            clear_screen();
        } else if (strcmp(command_name, "echo") == 0) {
            print_str(args_ptr);
            print_str("\n");
        } else if (strcmp(command_name, "meminfo") == 0) {
            print_str("Heap Information:\n");
            print_str("  Heap Start: "); print_hex((uint32_t)&__heap_start__); print_str("\n");
            print_str("  Heap End:   "); print_hex((uint32_t)&__heap_end__); print_str("\n");
            BlockHeader* current = (BlockHeader*)&__heap_start__;
            while((uint32_t)current < (uint32_t)&__heap_end__ && current->size > 0) {
                print_str("  Block at "); print_hex((uint32_t)current);
                print_str(" | Size: "); itoa(current->size, temp_str); print_str(temp_str);
                print_str(" | Free: "); print_str(current->free ? "Yes" : "No");
                if (!current->free) {
                    print_str(" | Magic: "); print_hex(current->magic);
                }
                print_str("\n");
                current = (BlockHeader*)((uint8_t*)current + current->size);
            }
        } else if (strcmp(command_name, "heapmap") == 0) {
            print_heap_map();
        } else if (strcmp(command_name, "alloc") == 0) {
            int size_to_alloc = atoi(args_ptr, NULL);
            if (size_to_alloc > 0) {
                void* p = malloc(size_to_alloc);
                if (p) {
                    print_str("Allocated "); itoa(size_to_alloc, temp_str); print_str(temp_str);
                    print_str(" bytes at "); print_hex((uint32_t)p); print_str("\n");
                } else {
                    print_str("Allocation failed.\n");
                }
            }
        } else if (strcmp(command_name, "calloc") == 0) {
            const char *num_ptr_end = NULL;
            int num = atoi(args_ptr, &num_ptr_end);
            while (*num_ptr_end == ' ') num_ptr_end++;
            int size = atoi(num_ptr_end, NULL);
            if (num > 0 && size > 0) {
                void* p = calloc(num, size);
                if (p) {
                    print_str("Allocated "); itoa(num * size, temp_str); print_str(temp_str);
                    print_str(" bytes at "); print_hex((uint32_t)p); print_str(" (zeroed)\n");
                } else {
                    print_str("Calloc failed.\n");
                }
            } else {
                print_str("Usage: calloc <num_elements> <size_per_element>\n");
            }
        } else if (strcmp(command_name, "realloc") == 0) {
             const char* addr_ptr_end = NULL;
             uint32_t addr = htoi(args_ptr, &addr_ptr_end);
             while(*addr_ptr_end == ' ') addr_ptr_end++;
             int new_size = atoi(addr_ptr_end, NULL);
             if (addr > 0 && new_size > 0) {
                 void* p = realloc((void*)addr, new_size);
                 if (p) {
                    print_str("Reallocated to "); itoa(new_size, temp_str); print_str(temp_str);
                    print_str(" bytes at "); print_hex((uint32_t)p); print_str("\n");
                 } else {
                    // Pesan error sudah dicetak di dalam realloc
                 }
             } else {
                 print_str("Usage: realloc <hex_address> <new_size>\n");
             }
        } else if (strcmp(command_name, "free") == 0) {
             uint32_t addr = htoi(args_ptr, NULL);
             if (addr > 0) {
                free((void*)addr);
             } else {
                print_str("Usage: free <hex_address>\n");
             }
        } else if (strcmp(command_name, "peek") == 0) {
            uint32_t addr = htoi(args_ptr, NULL);
            if (addr > 0) {
                uint32_t value = *(volatile uint32_t*)addr;
                print_str("Value at "); print_hex(addr);
                print_str(" is "); print_hex(value); print_str("\n");
            } else {
                print_str("Usage: peek <hex_address>\n");
            }
        } else if (strcmp(command_name, "poke") == 0) {
            const char* addr_ptr_end = NULL;
            uint32_t addr = htoi(args_ptr, &addr_ptr_end);
            while(*addr_ptr_end == ' ') addr_ptr_end++;
            uint32_t value = htoi(addr_ptr_end, NULL);
            if (addr > 0) {
                *(volatile uint32_t*)addr = value;
                print_str("Wrote "); print_hex(value);
                print_str(" to "); print_hex(addr); print_str("\n");
            } else {
                print_str("Usage: poke <hex_address> <hex_value>\n");
            }
        } else if (strcmp(command_name, "fill") == 0) {
            const char* ptr1, *ptr2;
            uint32_t addr = htoi(args_ptr, &ptr1);
            while(*ptr1 == ' ') ptr1++;
            uint32_t value = htoi(ptr1, &ptr2);
            while(*ptr2 == ' ') ptr2++;
            int count = atoi(ptr2, NULL);
            if (addr > 0 && count > 0) {
                for (int j = 0; j < count; j++) {
                    *(uint8_t*)(addr + j) = (uint8_t)value;
                }
                print_str("Filled "); itoa(count, temp_str); print_str(temp_str);
                print_str(" bytes at "); print_hex(addr);
                print_str(" with value "); print_hex(value); print_str("\n");
            } else {
                print_str("Usage: fill <hex_addr> <hex_value> <count>\n");
            }
        } else if (strcmp(command_name, "panic_test") == 0) {
            panic("User-initiated test");
        } else if (strcmp(command_name, "exit") == 0) {
            print_str("Exiting Amadeus OS...\n");
            return;
        } else if (command_name[0] != '\0') {
            print_str("Command not found: ");
            print_str(command_name);
            print_str("\n");
        }
    }
}
