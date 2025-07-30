#
#	Makefile
#

CC       = "gcc"
AS       = "nasm"
CFLAGS   = "-O2 -m32 -std=gnu99 -Wall -Wextra -ffreestanding -fno-pic \
		    -fno-stack-protector -fleading-underscore -nostartfiles -nostdinc"
ASFLAGS  = "-f elf32"
LD       = ld
LDSCRIPT = linker.ld
LDFLAGS  = -T $(LDSCRIPT) -melf_i386 -nostdlib -z noexecstack
MAKE     = make CC=$(CC) CFLAGS=$(CFLAGS) AS=$(AS) ASFLAGS=$(ASFLAGS)

DEPS     = core/boot.obj core/main.obj core/memory.obj \
           core/int/dsctable.obj core/int/inthandler.obj core/int/pic.obj \
           devices/keyboard.obj devices/serial.obj \
           libc/string.obj libc/stdio.obj \
		   ui/graphic.obj
TARGETS  = ClassiX.sys

.PHONY : default
default :
	$(MAKE) -C core
	$(MAKE) -C core/int
	$(MAKE) -C devices
	$(MAKE) -C libc
	$(MAKE) -C ui
	$(MAKE) $(TARGETS)

$(TARGETS) : $(DEPS)
	$(LD) $(LDFLAGS) $^ -o $@

PHONY : mrproper
mrproper:
	$(MAKE) -C core 		mrproper
	$(MAKE) -C core/int 	mrproper
	$(MAKE) -C devices 		mrproper
	$(MAKE) -C libc 		mrproper
	$(MAKE) -C ui 			mrproper
	rm -f $(TARGETS)
