#include "string/string.h"

static char tolower(char c) {
    if (c >= 65 && c <= 90) 
        c += 32;
    return c;
}

int strlen(const char *ptr) {
    int i = 0;
    while (*(ptr++) != 0)
        i++;

    return i;
}

int strnlen(const char *ptr, int bound) {
    int i = 0;
    for (; i < bound; i++) {
        if (ptr[i] == 0)
            break;
    }

    return i;
}

int strnlen_terminator(const char *str, int max, char terminator) {
    int i;
    for (i = 0; i < max; i++) {
        if (str[i] == '\0' || str[i] == terminator)
            break;
    }
    return i;
}

int istrnmp(const char *str1, const char *str2, int n) {
    unsigned char u1, u2;

    while (n-- > 0) {
        u1 = (unsigned char) *str1++;
        u2 = (unsigned char) *str2++;
        if (u1 != u2 && tolower(u1) != tolower(u2))
            return u1 - u2;  
        
        if (u1 == '\0')
            return 0;
    }
return 0;  
}

int strncmp(const char *str1, const char *str2, int n) {
    unsigned char u1, u2;

    while (n-- > 0) {
        u1 = (unsigned char) *str1++;
        u2 = (unsigned char) *str2++;
        if (u1 != u2)
            return u1 - u2;  
        
        if (u1 == '\0')
            return 0;
    }
return 0;
}

bool isdigit(char c) {
    return c >= 48 && c <= 57;
}

int tonumericdigit(char c) {
    return c - 48;
}


char *strcpy(char *dest, const char *src) {
    char *res = dest;
    while (*src != 0)
        *(dest++) = *(src++);
    
    *dest = 0x00;
    return res;
}