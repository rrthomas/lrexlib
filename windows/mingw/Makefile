# Makefile for lrexlib

all: build test

build:
	make -f rex_pcre.mak
	make -f rex_spencer.mak
	make -f rex_onig.mak
	make -f rex_gnu.mak
	make -f rex_tre.mak

test:
	make -f rex_pcre.mak test
	make -f rex_spencer.mak test
	make -f rex_onig.mak test
	make -f rex_gnu.mak test
	make -f rex_tre.mak test

install:
	make -f rex_pcre.mak install
	make -f rex_spencer.mak install
	make -f rex_onig.mak install
	make -f rex_gnu.mak install
	make -f rex_tre.mak install

clean:
	make -f rex_pcre.mak clean
	make -f rex_spencer.mak clean
	make -f rex_onig.mak clean
	make -f rex_gnu.mak clean
	make -f rex_tre.mak clean


.PHONY: all build test install clean
