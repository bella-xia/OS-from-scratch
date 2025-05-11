#ifndef STRING_H
#define STRING_H
#include "stdbool.h"


int strlen(const char *ptr);
int strnlen(const char *ptr, int bound);
bool isdigit(char c);
int tonumericdigit(char c);
char *strcpy(char *dest, const char *src);
int istrnmp(const char *str1, const char *str2, int n);
int strncmp(const char *str1, const char *str2, int n);
int strnlen_terminator(const char *str, int max, char terminator);

#endif