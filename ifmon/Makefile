LDLIBS   ?= 
CFLAGS   ?= -std=gnu11 -Wall -Wextra -Wpedantic \
            -Wno-gnu-zero-variadic-macro-arguments
PREFIX   ?= /usr/local

## Target if make is invoked without any parameters (goal)
.DEFAULT_GOAL: all

## "Virtual" targets without actual files to update/create
.PHONY: all clean distclean install


all: ifmon

ifmon: consumer.o ifmon.o main.o monitor.o
	$(LINK.c) $^ $(LOADLIBES) $(LDLIBS) -o $@

clean:
	rm -f consumer.o
	rm -f ifmon.o
	rm -f main.o
	rm -f monitor.o

distclean: clean
	rm -f ifmon

install: ifmon
	install ifmon $(PREFIX)/bin/