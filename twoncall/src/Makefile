# Makefile for twoncall/src/printopt

CC	= gcc
CFLAGS	= -O -Wall

BINS = printopt.x


.PHONY: all clean

all: $(BINS)

clean:
	rm -f $(BINS)


printopt.x: printopt.c twoncall.h
	$(CC) $(CFLAGS) -o $@ $<


# EOF
