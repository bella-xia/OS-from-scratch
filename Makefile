# Paths
SRC_DIR = ./src
BUILD_DIR = ./build
BIN_DIR = ./bin

# Compiler and flags
CC = i686-elf-gcc
AS = nasm
LD = i686-elf-ld
CFLAGS = -std=gnu99 -c -g -ffreestanding -Wall -O0 $(INCLUDES) $(FLAGS)
ASFLAGS_ELF = -f elf -g
ASFLAGS_BIN = -f bin

# Include paths and GCC flags
INCLUDES = -I./src -Iinc
FLAGS = -ffreestanding -falign-jumps -falign-functions -falign-labels -falign-loops \
        -fstrength-reduce -fomit-frame-pointer -finline-functions -Wno-unused-function \
        -fno-builtin -Werror -Wno-unused-label -Wno-cpp -Wno-unused-parameter \
        -nostdlib -nostartfiles -nodefaultlibs
    
#core c source file
C_SRCS = kernel.c
# core ASM source files (ELF)
ASM_SRCS = kernel.asm

# interrupt discriptor 
C_SRCS += idt/idt.c
ASM_SRCS += idt/idt.asm

# io control
ASM_SRCS += io/io.asm 

# memory and paging
C_SRCS += memory/memory.c \
          memory/heap.c \
          memory/kheap.c \
          memory/paging.c
ASM_SRCS += memory/paging.asm

# fs and disk
C_SRCS += fs/pparser.c \
          string/string.c \
          disk/disk.c \
		  disk/streamer.c

# Object files
OBJS = $(addprefix $(BUILD_DIR)/, $(ASM_SRCS:.asm=.asm.o) $(C_SRCS:.c=.o))

# Targets
all: $(BIN_DIR)/boot.bin $(BIN_DIR)/kernel.bin
	rm -f $(BIN_DIR)/os.bin
	dd if=$(BIN_DIR)/boot.bin >> $(BIN_DIR)/os.bin
	dd if=$(BIN_DIR)/kernel.bin >> $(BIN_DIR)/os.bin
	dd if=/dev/zero bs=512 count=100 >> $(BIN_DIR)/os.bin

$(BIN_DIR)/kernel.bin: $(OBJS)
	$(LD) -g -relocatable $(OBJS) -o $(BUILD_DIR)/kernelfull.o
	$(CC) -T $(SRC_DIR)/linker.ld -o $@ -ffreestanding -O0 -nostdlib $(BUILD_DIR)/kernelfull.o

$(BIN_DIR)/boot.bin: $(SRC_DIR)/boot/boot.asm
	$(AS) $(ASFLAGS_BIN) $< -o $@

# Pattern rules
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/%.asm.o: $(SRC_DIR)/%.asm
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS_ELF) $< -o $@

clean:
	rm -rf $(BIN_DIR)/boot.bin $(BIN_DIR)/kernel.bin $(BIN_DIR)/os.bin ${OBJS} $(BUILD_DIR)/kernelfull.o