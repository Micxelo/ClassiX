#
#	devices/Makefile
#

INCPATH =  ../include/
CC      =
AS      =
CFLAGS  =
ASFLAGS =

TARGETS = keyboard.obj serial.obj

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
