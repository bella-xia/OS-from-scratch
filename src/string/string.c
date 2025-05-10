#include "string/string.h"

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