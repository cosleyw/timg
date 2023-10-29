
CC ?= gcc
CFLAGS := -O2 -g 
LIBS := -lm -lz libschrift.a

CSRC := $(wildcard src/*.c)
COBJ := $(CSRC:.c=.o)

all: timg

clean: 
	rm -f timg ${COBJ}

timg: main.c timg.a( ${COBJ} )
	${CC} -o $@ $^ ${CFLAGS} ${LIBS}

.PHONY: all clean
