#
#	core/int/Makefile
#

INCPATH =  ../../include/
CC      =
AS      =
CFLAGS  =
ASFLAGS =

TARGETS = dsctable.obj inthandler.obj pic.obj

.PHONY : default
default :
	make $(TARGETS)

%.obj : %.c Makefile
	$(CC) -c $(CFLAGS) -I $(INCPATH) $< -o $@

%.obj : %.asm Makefile
	$(AS) $(ASFLAGS) $< -o $@

.PHONY : mrproper
mrproper:
	rm -f $(TARGETS)
