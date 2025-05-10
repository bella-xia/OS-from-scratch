#include <stddef.h>
#include <stdint.h>
#include "kernel.h"
#include "idt/idt.h"
#include "io/io.h"
#include "memory/kheap.h"
#include "memory/paging.h"
#include "disk/disk.h"
#include "disk/streamer.h"
#include "fs/pparser.h"
#include "string/string.h"
#include "fs/fat16.h"

uint16_t terminal_row, terminal_col, *video_mem;
static struct paging_4gb_chunk *kernel_chunk;

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

size_t kstrlen(const char *str) {
    size_t len = 0;
    while (str[len++]);
    return len;
}

void print(const char *str) {
    size_t len = kstrlen(str);
    for (int i = 0; i < len; i++) 
        terminal_writechar(str[i], 15);
}

void kernel_main() {

    kheap_init();
    fs_init();

    disk_search_and_init();

    idt_init();

    kernel_chunk = paging_new_4gb(PAGING_IS_WRITEABLE | PAGING_IS_PRESENT | PAGING_ACCESS_FROM_ALL);
    paging_switch(kernel_chunk->directory_entry);
    
    enable_paging();

    enable_interrupts();

    char buf[20];
    strcpy(buf, "hello!");

}