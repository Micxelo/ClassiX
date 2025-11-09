#
#	Makefile
#

CC			= gcc
AS			= nasm
LD			= ld
OBJCOPY		= objcopy

CFLAGS		= -O2 -m32 -std=gnu99 \
			  -Wall -Wextra -Wno-unused-parameter -Wno-unused-function \
			  -ffreestanding -fleading-underscore -fno-pic -fno-stack-protector \
			  -nostartfiles -nostdinc
ASFLAGS		= -f elf32
LDSCRIPT	= linker.ld
LDFLAGS		= -T $(LDSCRIPT) -melf_i386 -nostdlib -z noexecstack

INCPATH		= ./include

# 源文件
C_SOURCES	= $(shell find . -name "*.c" -not -path "./assets/*")
ASM_SOURCES	= $(shell find . -name "*.asm" -not -path "./assets/*")

# 资源文件
ASSET_FILES	= $(shell find assets -type f -not -name Makefile -not -name "*.obj")

DEPS		= $(C_SOURCES:.c=.obj) $(ASM_SOURCES:.asm=.obj) $(ASSET_FILES:%=%.obj)

TARGETS		= ClassiX.sys

.PHONY : default
default : $(TARGETS)

$(TARGETS) : $(DEPS)
	@$(LD) $(LDFLAGS) $^ -o $@
	@echo "\tLD\t$@"

# 编译规则
%.obj : %.c
	@$(CC) -c $(CFLAGS) -I $(INCPATH) $< -o $@
	@echo "\tCC\t$@"

%.obj : %.asm
	@$(AS) $(ASFLAGS) $< -o $@
	@echo "\tAS\t$@"

# 资源文件转换规则
%.obj : %
	@$(OBJCOPY) -I binary -O elf32-i386 -B i386 \
		--rename-section .data=.asset,contents,alloc,load,readonly,data \
		$< $@
	@echo "\tOBJCOPY\t$@"

.PHONY : clean
clean:
	@find . -name "*.obj" -delete
	@rm -f $(TARGETS)
	@echo "\tRM\t$(TARGETS)"
