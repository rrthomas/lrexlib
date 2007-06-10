# Makefile for lrexlib

all: build test

build:
	make -f rex_pcre.mak
	make -f rex_spencer.mak

test:
	make -f rex_pcre.mak test
	make -f rex_spencer.mak test

install:
	make -f rex_pcre.mak install
	make -f rex_spencer.mak install

clean:
	make -f rex_pcre.mak clean
	make -f rex_spencer.mak clean


.PHONY: all build test install clean
