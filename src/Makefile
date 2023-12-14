# Makefile for (V)TwentyOne.sys

CC	= gcc
CFLAGS	= -O -Wall -fstrength-reduce -fomit-frame-pointer -fforce-addr
AS	= has060
ASFLAGS	= -c3
LD	= hlk
LDFLAGS	= -x

OBJS = header.o patch.o func.o vfunc.o namecheck.o vnamecheck.o vfat.o u2stbl.o twon.o
BINS = twentyone.sys vtwentyone.sys twon.r
DOCS = twentyonesys.txt CHANGELOG.txt

ZIPFILE = twsys.zip


.PHONY: all clean archive

all: $(BINS)

archive: $(ZIPFILE)

$(ZIPFILE): $(DOCS) $(BINS)
	rm -f $@
	zip -9 $@ $^

clean:
	rm -f $(ZIPFILE) $(BINS) $(OBJS)


header.o: header.s twoncall.mac patchlevel.mac
	$(AS) $(ASFLAGS) -o $@ $<

patch.o: patch.s twoncall.mac
	$(AS) $(ASFLAGS) -o $@ $<

func.o: func.s twoncall.mac patchlevel.mac
	$(AS) $(ASFLAGS) -o $@ $<

vfunc.o: func.s twoncall.mac patchlevel.mac
	$(AS) $(ASFLAGS) -o $@ $< -sUSE_VFAT

namecheck.o: namecheck.c namecheck.h
	$(CC) $(CFLAGS) -o $@ -c $<

vnamecheck.o: namecheck.c namecheck.h vfat.h
	$(CC) $(CFLAGS) -o $@ -c $< -DUSE_VFAT

vfat.o: vfat.c namecheck.h vfat.h
	$(CC) $(CFLAGS) -o $@ -c $<

u2stbl.o: u2stbl.c u2stbl.h
	$(CC) $(CFLAGS) -o $@ -c $<

twentyone.sys: header.o patch.o func.o namecheck.o
	$(LD) $(LDFLAGS) -o $@ $^

vtwentyone.sys: header.o patch.o vfunc.o vnamecheck.o vfat.o u2stbl.o
	$(LD) $(LDFLAGS) -o $@ $^


twon.o: twon.s twoncall.mac
	$(AS) $(ASFLAGS) -o $@ $<

twon.r: twon.o
	$(LD) $(LDFLAGS) -o $@ -r $^

# EOF
