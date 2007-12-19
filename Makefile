# Makefile for lrexlib

# See src/*.mak for user-definable settings

all: build_pcre test_pcre build_posix test_posix

build_pcre:
	make -C src -f rex_pcre.mak

build_posix:
	make -C src -f rex_posix.mak

test_pcre:
	cd test && lua ./runtest.lua pcre

test_posix:
	cd test && lua ./runtest.lua posix

clean_pcre:
	make -C src -f rex_pcre.mak clean

clean_posix:
	make -C src -f rex_posix.mak clean

.PHONY: all build_pcre test_pcre build_posix test_posix clean_pcre clean_posix

