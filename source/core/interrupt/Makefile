#
#	core/interrupt/Makefile
#

INCPATH		= ../../include

# 源文件
C_SOURCES = $(wildcard *.c)
ASM_SOURCES = $(wildcard *.asm)
TARGETS = $(C_SOURCES:.c=.obj) $(ASM_SOURCES:.asm=.obj)

.PHONY : default
default : $(TARGETS)

%.obj : %.c
	@$(CC) -c $(CFLAGS) -I $(INCPATH) $< -o $@
	@echo "\tCC\t$@"

%.obj : %.asm
	@$(AS) $(ASFLAGS) $< -o $@
	@echo "\tAS\t$@"

.PHONY : clean
clean:
	@rm -f $(TARGETS)
	@echo "\tRM\t$(TARGETS)"
