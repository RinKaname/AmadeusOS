#include <stdint.h>
#include <stddef.h> // For NULL

/**
 * @brief Compares two strings.
 * @return 0 if equal, <0 if s1 < s2, >0 if s1 > s2.
 */
int strcmp(const char *s1, const char *s2)
{
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

/**
 * @brief Calculates the length of a string.
 * @return The number of characters in the string.
 */
int strlen(const char *s)
{
    int len = 0;
    while (s[len]) {
        len++;
    }
    return len;
}

/**
 * @brief Reverses a string in place. Helper for itoa.
 */
static void reverse(char s[])
{
    int i, j;
    char c;
    for (i = 0, j = strlen(s)-1; i<j; i++, j--) {
        c = s[i];
        s[i] = s[j];
        s[j] = c;
    }
}

/**
 * @brief Converts an integer to a null-terminated string (ASCII).
 * @param n The integer to convert.
 * @param s Buffer to store the resulting string.
 */
void itoa(int n, char *s)
{
    int i = 0, sign;

    if ((sign = n) < 0) {
        n = -n;
    }

    do {
        s[i++] = n % 10 + '0';
    } while ((n /= 10) > 0);

    if (sign < 0) {
        s[i++] = '-';
    }
    s[i] = '\0';
    reverse(s);
}

/**
 * @brief Converts a string (ASCII) to an integer. Handles negative numbers.
 * @param s The string to convert.
 * @param endptr If not NULL, it's set to point to the character after the last parsed digit.
 * @return The converted integer.
 */
int atoi(const char *s, const char **endptr)
{
    int result = 0;
    int sign = 1;

    // Skip leading whitespace
    while (*s == ' ' || *s == '\t' || *s == '\n') {
        s++;
    }

    // Handle sign
    if (*s == '-') {
        sign = -1;
        s++;
    } else if (*s == '+') {
        s++;
    }

    // Convert digits
    while (*s >= '0' && *s <= '9') {
        result = result * 10 + (*s - '0');
        s++;
    }
    
    if (endptr != NULL) {
        *endptr = s;
    }

    return result * sign;
}

/**
 * @brief Copies at most n characters from src to dest.
 */
void strncpy(char *dest, const char *src, int n)
{
    int i;
    for (i = 0; i < n && src[i] != '\0'; i++) {
        dest[i] = src[i];
    }
    for ( ; i < n; i++) {
        dest[i] = '\0';
    }
}