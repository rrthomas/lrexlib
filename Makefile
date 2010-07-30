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
	make -C $(GNU) -f rex_gnu.mak

build_pcre:
	make -C $(PCRE) -f rex_pcre.mak

build_posix:
	make -C $(POSIX) -f rex_posix.mak

build_onig:
	make -C $(ONIG) -f rex_onig.mak

build_tre:
	make -C $(TRE) -f rex_tre.mak

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
	make -C $(GNU) -f rex_gnu.mak clean

clean_pcre:
	make -C $(PCRE) -f rex_pcre.mak clean

clean_posix:
	make -C $(POSIX) -f rex_posix.mak clean

clean_onig:
	make -C $(ONIG) -f rex_onig.mak clean

clean_tre:
	make -C $(TRE) -f rex_tre.mak clean

.PHONY: all build test clean build_gnu test_gnu clean_gnu \
  build_pcre test_pcre clean_pcre build_posix \
  test_posix clean_posix build_onig test_onig clean_onig \
  build_tre clean_tre
