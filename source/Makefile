#
#	Makefile
#

CC       = "gcc"
AS       = "nasm"
CFLAGS   = "-O2 -m32 -std=gnu99 -Wall -Wextra -Wno-unused-parameter -Wno-unused-function \
            -ffreestanding -fno-pic -fno-stack-protector -fleading-underscore \
            -nostartfiles -nostdinc"
ASFLAGS  = "-f elf32"
LD       = ld
LDSCRIPT = linker.ld
LDFLAGS  = -T $(LDSCRIPT) -melf_i386 -nostdlib -z noexecstack
MAKE     = make CC=$(CC) CFLAGS=$(CFLAGS) AS=$(AS) ASFLAGS=$(ASFLAGS) --no-print-directory

DEPS     = core/boot.obj core/main.obj core/memory.obj core/task.obj \
           core/interrupt/gdt.obj core/interrupt/idt.obj core/interrupt/isr.obj core/interrupt/pic.obj \
           devices/cpu.obj devices/keyboard.obj devices/mouse.obj devices/pci.obj devices/pit.obj devices/serial.obj \
		   devices/blkdev/harddisk.obj \
           libc/string.obj libc/stdio.obj \
           ui/builtin.obj ui/cga.obj ui/font.obj ui/framebuf.obj ui/graphic.obj ui/layer.obj ui/palette.obj \
           utilities/fatfs.obj utilities/fifo.obj utilities/rtc.obj utilities/timer.obj
TARGETS  = ClassiX.sys

.PHONY : default
default :
	@$(MAKE) -C core
	@$(MAKE) -C core/interrupt
	@$(MAKE) -C devices
	@$(MAKE) -C devices/blkdev
	@$(MAKE) -C libc
	@$(MAKE) -C ui
	@$(MAKE) -C utilities
	@$(MAKE) $(TARGETS)

$(TARGETS) : $(DEPS)
	@$(LD) $(LDFLAGS) $^ -o $@
	@echo "\tLD\t$@"

PHONY : clean
clean:
	@$(MAKE) -C core            clean
	@$(MAKE) -C core/interrupt  clean
	@$(MAKE) -C devices         clean
	@$(MAKE) -C devices/blkdev  clean
	@$(MAKE) -C libc            clean
	@$(MAKE) -C ui              clean
	@$(MAKE) -C utilities       clean
	@rm -f $(TARGETS)
	@echo "\tRM\t$@"
