#include <stddef.h>
#include <stdint.h>
#include "kernel.h"
#include "idt/idt.h"
#include "io/io.h"
#include "memory/kheap.h"

uint16_t terminal_row, terminal_col, *video_mem;

static uint16_t terminal_makechar(char c, char color) {
    return (color << 8) | c;
}

void terminal_putchar(int x, int y, char c, char color) {
    video_mem[y * VGA_WIDTH + x] = terminal_makechar(c, color);
}

void terminal_writechar(char c, char color) {
    if (c == '\n') {
        terminal_col = 0;
        terminal_row++;
        return;
    }
    terminal_putchar(terminal_col++, terminal_row, c, color);
    if (terminal_col >= VGA_WIDTH) {
        terminal_col = 0;
        terminal_row++;
    }
}

void terminal_initialize() {
    video_mem = (uint16_t *)(0xB8000);
    terminal_col = 0;
    terminal_row = 0;
    for (int y = 0; y < VGA_HEIGHT; y++) {
        for (int x = 0; x < VGA_WIDTH; x++)
            terminal_putchar(x, y, ' ', 0);
    }
}

size_t strlen(const char *str) {
    size_t len = 0;
    while (str[len++]);
    return len;
}

void print(const char *str) {
    size_t len = strlen(str);
    for (int i = 0; i < len; i++) 
        terminal_writechar(str[i], 15);
}

void kernel_main() {
    terminal_initialize();
    print("Hello world!\ntest");

    kheap_init();

    idt_init();

    void *ptr = kmalloc(50);
    void *ptr2 = kmalloc(5000);
    void *ptr3 = kmalloc(5600);
    kfree(ptr);
    void *ptr4 = kmalloc(45);

    if (ptr || ptr2 || ptr3 || ptr4);

}