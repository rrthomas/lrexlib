# Makefile for lrexlib

# See src/*.mak for user-definable settings

all:
	make -C src -f rex_pcre.mak
	make -C src -f rex_posix.mak

test:
	cd test && lua ./runtest.lua pcre posix

clean:
	make -C src -f rex_pcre.mak clean
	make -C src -f rex_posix.mak clean


.PHONY: all test clean
