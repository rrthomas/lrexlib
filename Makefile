# Makefile for lrexlib

# See src/*.mak for user-definable settings

all: build test

build:
	make -C src -f rex_pcre.mak
	make -C src -f rex_posix.mak
	make -C src -f rex_tre.mak

test:
	cd test && lua ./runtest.lua pcre posix tre

clean:
	make -C src -f rex_pcre.mak clean
	make -C src -f rex_posix.mak clean
	make -C src -f rex_tre.mak clean


.PHONY: all test clean
