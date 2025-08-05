#
#	utilities/Makefile
#

INCPATH = ../include/
CC      =
AS      =
CFLAGS  =
ASFLAGS =

TARGETS = fifo.obj rtc.obj timer.obj

.PHONY : default
default :
	@make $(TARGETS)

%.obj : %.c Makefile
	@$(CC) -c $(CFLAGS) -I $(INCPATH) $< -o $@
	@echo "\tCC\t$@"

%.obj : %.asm Makefile
	@$(AS) $(ASFLAGS) $< -o $@
	@echo "\tAS\t$@"

.PHONY : mrproper
mrproper:
	@rm -f $(TARGETS)
	@echo "\tRM\t$@"
	@echo "\tRM\t$@"
