#
#	api/Makefile
#

CC			= gcc
AS			= nasm
AR			= ar

CFLAGS		= -O2 -m32 -std=gnu99 \
			  -Wall -Wextra -Wno-unused-parameter -Wno-unused-function -Werror=parentheses \
			  -fleading-underscore -ffreestanding -fno-pic -nostdinc
ASFLAGS		= -f elf32
ARFLAGS		= rcs

INCPATH		= ../include

# 源文件
C_SOURCES	= $(shell find . -name "*.c")
ASM_SOURCES	= $(shell find . -name "*.asm")

DEPS		= $(C_SOURCES:.c=.obj) $(ASM_SOURCES:.asm=.obj)

TARGET		= api.lib

.PHONY : default
default : $(TARGET)

$(TARGET) : $(DEPS)
	@$(AR) $(ARFLAGS) $@ $^
	@echo "\tAR\t$@"

# 编译规则
%.obj : %.c
	@$(CC) -c $(CFLAGS) -I $(INCPATH) $< -o $@
	@echo "\tCC\t$@"

%.obj : %.asm
	@$(AS) $(ASFLAGS) $< -o $@
	@echo "\tAS\t$@"

.PHONY : clean
clean:
	@find . -name "*.obj" -delete
	@rm -f $(TARGET)
	@echo "\tRM\t$(TARGET)"
