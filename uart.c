#include "reg.h" // For UART register definitions
#include <stddef.h> // For NULL

// Define common ASCII control characters
#define ASCII_CR    0x0D // Carriage Return (Enter key)
#define ASCII_LF    0x0A // Line Feed (Newline)
#define ASCII_BS    0x08 // Backspace
#define ASCII_DEL   0x7F // Delete key (alternative to backspace)

/**
 * @brief Sends a single character over UART0.
 *
 * This function waits until the UART transmit buffer (FIFO) is not full
 * before sending the character.
 * @param c The character to send.
 */
void uart0_putc(char c)
{
    // Wait until the Transmit FIFO is not full (TXFF flag is 0).
    while ((*(UART0_FR) & UART0_FR_TXFF) != 0);
    // Write the character to the UART Data Register.
    *(UART0_DR) = c;
}

/**
 * @brief Receives a single character from UART0.
 *
 * This function waits until the UART receive buffer (FIFO) is not empty
 * before reading the character. This is a blocking call.
 * @return The character received.
 */
char uart0_getc(void)
{
    // Wait until the Receive FIFO is not empty (RXFE flag is 0).
    while ((*(UART0_FR) & UART0_FR_RXFE) != 0);
    // Read the character from the UART Data Register.
    return *(UART0_DR);
}

/**
 * @brief Prints a null-terminated string to the UART0 console.
 * @param str Pointer to the string to print.
 */
void print_str(const char *str)
{
    while (*str != '\0') {
        // Replace newline '\n' with carriage return + line feed for proper terminal display
        if (*str == '\n') {
            uart0_putc(ASCII_CR);
            uart0_putc(ASCII_LF);
        } else {
            uart0_putc(*str);
        }
        str++;
    }
}

/**
 * @brief Clears the terminal screen using ANSI escape codes.
 */
void clear_screen(void)
{
    // "\033[2J" clears the entire screen.
    // "\033[H" moves the cursor to the home position (top-left).
    print_str("\033[2J\033[H");
}

/**
 * @brief Reads a line of text from the user, handling backspace and echoing.
 *
 * @param buffer The buffer to store the read line.
 * @param max_len The maximum length of the buffer.
 */
void readline(char *buffer, int max_len)
{
    int i = 0;
    char c;

    while (1) {
        c = uart0_getc();

        // If Enter key (Carriage Return) is pressed
        if (c == ASCII_CR) {
            print_str("\n"); // Echo a newline to the terminal
            break;
        }
        // If Backspace or Delete key is pressed
        else if ((c == ASCII_BS || c == ASCII_DEL) && i > 0) {
            i--;
            // Echo backspace, space, backspace to erase character on the screen
            uart0_putc(ASCII_BS);
            uart0_putc(' ');
            uart0_putc(ASCII_BS);
        }
        // If a printable character is pressed and buffer is not full
        else if (c >= ' ' && c <= '~' && i < (max_len - 1)) {
            buffer[i++] = c;
            uart0_putc(c); // Echo the character back to the terminal
        }
    }
    // Null-terminate the string in the buffer
    buffer[i] = '\0';
}