CC      = i686-elf-gcc
AS      = nasm
LD      = i686-elf-ld
OBJCOPY = i686-elf-objcopy
PYTHON  = "C:/Users/samrat/AppData/Local/Programs/Python/Python312/python.exe"
QEMU    = "C:/Program Files/qemu/qemu-system-i386.exe"

CFLAGS  = -ffreestanding -O2 -Wall -Wextra -Werror -std=c99 \
          -nostdlib -nostdinc -fno-builtin -fno-stack-protector \
          -nostartfiles -nodefaultlibs -c

ASFLAGS = -f elf32
LDFLAGS = -T linker.ld -nostdlib

C_SRC   = kernel.c gdt.c idt.c isr.c vga.c keyboard.c vfs.c shell.c \
          timer.c pmm.c heap.c process.c syscall.c doom.c \
          paging.c elf.c ata.c devtree.c security.c \
          scheduler.c signal.c net.c init.c pipe.c usb.c
ASM_SRC = entry.asm helpers.asm isr_stubs.asm ctx_switch.asm

C_OBJ   = $(C_SRC:.c=.o)
ASM_OBJ = $(ASM_SRC:.asm=.o)
OBJECTS = $(ASM_OBJ) $(C_OBJ)

KERNEL_ELF = orangex_kernel.elf
KERNEL_BIN = orangex_kernel.bin
BOOT16     = boot16.bin
IMAGE      = orangex_boot.img

.PHONY: all clean run run-gui floppy

all: $(KERNEL_BIN)

$(KERNEL_ELF): $(OBJECTS) linker.ld
	$(LD) $(LDFLAGS) -o $(KERNEL_ELF) $(OBJECTS)

$(KERNEL_BIN): $(KERNEL_ELF)
	$(OBJCOPY) -O binary $(KERNEL_ELF) $(KERNEL_BIN)

%.o: %.c
	$(CC) $(CFLAGS) $< -o $@

%.o: %.asm
	$(AS) $(ASFLAGS) $< -o $@

floppy: $(KERNEL_BIN)
	$(AS) -f bin boot16.asm -o $(BOOT16)
	$(PYTHON) create_image.py

run: $(KERNEL_ELF)
	$(QEMU) -kernel $(KERNEL_ELF) -m 128M

run-gui: floppy
	$(QEMU) -drive format=raw,file=$(IMAGE),if=floppy -m 128M

clean:
	rm -f $(OBJECTS) $(KERNEL_ELF) $(KERNEL_BIN) $(BOOT16) $(IMAGE)
