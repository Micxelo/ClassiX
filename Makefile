#
#	Makefile
#

CC			= gcc
AS			= nasm
LD			= ld
OBJCOPY		= objcopy
CCHK		= cppcheck

CFLAGS		= -O2 -m32 -std=gnu11 \
			  -Wall -Wextra -Wno-unused-parameter -Wno-unused-function -Werror=parentheses \
			  -ffreestanding -fleading-underscore -fno-pic -fno-stack-protector \
			  -nostartfiles -nostdinc
ASFLAGS		= -f elf32
LDSCRIPT	= ./source/linker.ld
LDFLAGS		= -T $(LDSCRIPT) -melf_i386 -nostdlib -z noexecstack --oformat binary
CCHKFLAGS	= -q --enable=all --platform=unix32 --std=c11 --language=c -I $(INCPATH) \
			  --suppress=missingIncludeSystem --suppress=unusedFunction --suppress=variableScope

INCPATH		= ./source/include

# 源文件
C_SOURCES	= $(shell find ./source/ -name "*.c" -not -path "./source/assets/*")
ASM_SOURCES	= $(shell find ./source/ -name "*.asm" -not -path "./source/assets/*")

# 资源文件
ASSET_FILES	= $(shell find ./source/assets/ -type f -not -name Makefile -not -name "*.obj")

DEPS		= $(C_SOURCES:.c=.obj) $(ASM_SOURCES:.asm=.obj) $(ASSET_FILES:%=%.obj)

TARGET		= ClassiX.sys

.PHONY : default
default : $(TARGET)

$(TARGET) : $(DEPS)
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
	@rm -f $(TARGET)
	@find . -name "cppcheck.log" -delete
	@echo "\tRM\t$(TARGET)"

.PHONY : check
check:
	@if command -v cppcheck >/dev/null 2>&1; then \
		$(CCHK) $(CCHKFLAGS) $(C_SOURCES) > cppcheck.log 2>&1 || true; \
		echo "\tCHECK\tDone"; \
	else \
		echo "\tCHECK\tcppcheck not found, skipping static analysis"; \
	fi
