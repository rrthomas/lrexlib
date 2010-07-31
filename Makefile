# Makefile for lrexlib

# See src/*.mak for user-definable settings

GNU = src/gnu
PCRE = src/pcre
POSIX = src/posix
ONIG = src/oniguruma
TRE = src/tre

all: build test

build: build_gnu build_pcre build_posix build_onig build_tre

test: test_gnu test_pcre test_posix test_onig test_tre

clean: clean_gnu clean_pcre clean_posix clean_onig clean_tre

build_gnu:
	make -C $(GNU)

build_pcre:
	make -C $(PCRE)

build_posix:
	make -C $(POSIX)

build_onig:
	make -C $(ONIG)

build_tre:
	make -C $(TRE)

test_gnu:
	cd test && lua ./runtest.lua -d../$(GNU) gnu

test_pcre:
	cd test && lua ./runtest.lua -d../$(PCRE) pcre

test_posix:
	cd test && lua ./runtest.lua -d../$(POSIX) posix

test_onig:
	cd test && lua ./runtest.lua -d../$(ONIG) onig

test_tre:
	cd test && lua ./runtest.lua -d../$(TRE) tre

clean_gnu:
	make -C $(GNU) clean

clean_pcre:
	make -C $(PCRE) clean

clean_posix:
	make -C $(POSIX) clean

clean_onig:
	make -C $(ONIG) clean

clean_tre:
	make -C $(TRE) clean
