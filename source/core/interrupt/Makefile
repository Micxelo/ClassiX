#
#	core/interrupt/Makefile
#

INCPATH =  ../../include/
CC      =
AS      =
CFLAGS  =
ASFLAGS =

TARGETS = gdt.obj idt.obj isr.obj pic.obj

.PHONY : default
default :
	@make $(TARGETS)

%.obj : %.c Makefile
	@$(CC) -c $(CFLAGS) -I $(INCPATH) $< -o $@
	@echo "\tCC\t$@"

%.obj : %.asm Makefile
	@$(AS) $(ASFLAGS) $< -o $@
	@echo "\tAS\t$@"

.PHONY : clean
clean:
	@rm -f $(TARGETS)
	@echo "\tRM\t$@"
