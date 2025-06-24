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
static char greet[] = "Welcome to Amadeus OS v0.7.2! ^_^\n";

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

// Deklarasi Prototype untuk Heap Manager
void malloc_init(void);
void* malloc(size_t size);
void free(void* ptr);
void* calloc(size_t num, size_t size);
void* realloc(void* ptr, size_t new_size);


// ============================================================================
// Implementasi Heap Manager (malloc/free/calloc/realloc)
// ============================================================================

typedef struct BlockHeader {
    size_t size;
    int free;
} BlockHeader;

#define ALIGN_SIZE(size) (((size) + (sizeof(uint32_t) - 1)) & ~(sizeof(uint32_t) - 1))
#define MIN_BLOCK_SIZE   (ALIGN_SIZE(sizeof(BlockHeader)))

void malloc_init(void) {
    BlockHeader* head = (BlockHeader*)&__heap_start__;
    head->size = (uint32_t)&__heap_end__ - (uint32_t)&__heap_start__;
    head->free = 1;
}

void* malloc(size_t size) {
    if (size == 0) return NULL;

    size_t required_size = ALIGN_SIZE(size + sizeof(BlockHeader));
    if (required_size < MIN_BLOCK_SIZE) {
        required_size = MIN_BLOCK_SIZE;
    }

    BlockHeader* current = (BlockHeader*)&__heap_start__;
    BlockHeader* best_fit = NULL;

    while((uint32_t)current < (uint32_t)&__heap_end__) {
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
            current->size = required_size;
        }
        current->free = 0;
        return (void*)((uint8_t*)current + sizeof(BlockHeader));
    }
    return NULL;
}


void free(void* ptr) {
    if (ptr == NULL || (uint32_t)ptr < (uint32_t)&__heap_start__ || (uint32_t)ptr >= (uint32_t)&__heap_end__) {
        return;
    }

    BlockHeader* block_to_free = (BlockHeader*)((uint8_t*)ptr - sizeof(BlockHeader));

    if (block_to_free->free) {
        print_str("Warning: Double-free or corrupted pointer detected!\n");
        return;
    }

    block_to_free->free = 1;

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

    BlockHeader* block_header = (BlockHeader*)((uint8_t*)ptr - sizeof(BlockHeader));
    size_t old_size = block_header->size - sizeof(BlockHeader);

    if (new_size <= old_size) return ptr;

    void* new_ptr = malloc(new_size);
    if (new_ptr == NULL) return NULL;

    memcpy(new_ptr, ptr, old_size);
    free(ptr);
    return new_ptr;
}

// ============================================================================
// Implementasi Fungsi Helper
// ============================================================================
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
    
    while (n > 0) {
        uint8_t digit = n % 16;
        if (digit < 10) {
            *ptr-- = '0' + digit;
        } else {
            *ptr-- = 'a' + (digit - 10);
        }
        n /= 16;
    }
    
    print_str("0x");
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
            print_str("  alloc <size>       - Allocate memory from heap\n");
            print_str("  calloc <n> <size>  - Allocate and zero-initialize memory\n");
            print_str("  realloc <addr> <sz>- Reallocate memory\n");
            print_str("  free <addr>        - Free memory from heap (e.g., free 0x20000100)\n");
            print_str("  peek <addr>        - Read a 32-bit value from a memory address\n");
            print_str("  poke <addr> <val>  - Write a 32-bit value to a memory address\n");
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
            while((uint32_t)current < (uint32_t)&__heap_end__) {
                print_str("  Block at "); print_hex((uint32_t)current);
                print_str(" | Size: "); itoa(current->size, temp_str); print_str(temp_str);
                print_str(" | Free: "); print_str(current->free ? "Yes\n" : "No\n");
                current = (BlockHeader*)((uint8_t*)current + current->size);
            }
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
                    print_str("Realloc failed.\n");
                 }
             } else {
                 print_str("Usage: realloc <hex_address> <new_size>\n");
             }
        } else if (strcmp(command_name, "free") == 0) {
             uint32_t addr = htoi(args_ptr, NULL);
             if (addr > 0) {
                free((void*)addr);
                print_str("Freed memory at "); print_hex(addr); print_str("\n");
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
