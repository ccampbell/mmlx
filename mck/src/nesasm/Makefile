# MAGICKIT MSDOS MAKEFILE
#

RM       = del
CC       = gcc

CFLAGS   = -O -Wall

OBJS     = main.o input.o assemble.o expr.o code.o command.o\
           macro.o func.o proc.o symbol.o pcx.o output.o crc.o\
           pce.o map.o mml.o nes.o

EXEDIR   = ../../bin

TARGPCE  = $(EXEDIR)/pceas.exe
TARGNES  = $(EXEDIR)/nesasm.exe
TARGETS  = $(TARGPCE) $(TARGNES)

#
#

all: $(TARGETS)

# CLEAN
#

clean:
	$(RM) *.o
	$(RM) ..\..\bin\pceas.exe
	$(RM) ..\..\bin\nesasm.exe

# ASSEMBLER
#

$(OBJS)  : defs.h externs.h protos.h
main.o   : inst.h vars.h
expr.o   : expr.h
pce.o    : pce.h
nes.o    : nes.h
%.o : %.c
	$(CC) $(CFLAGS) -o $@ -c $<

# EXE
#

$(TARGPCE) : $(OBJS)
	$(CC) -o $(TARGPCE) -s $(OBJS)
$(TARGNES) : $(OBJS)
	$(CC) -o $(TARGNES) -s $(OBJS)

