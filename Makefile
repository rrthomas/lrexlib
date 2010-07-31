# Makefile for lrexlib

# See src/*/Makefile for user-definable settings

REGNAMES = gnu pcre posix oniguruma tre

all: build check

build:
	@for i in $(REGNAMES); do \
	  make -C src/$$i; \
	done

check:
	@for i in $(REGNAMES); do \
	  make -C src/$$i check; \
	done

clean:
	@for i in $(REGNAMES); do \
	  make -C src/$$i clean; \
	done

dist:
	git2cl > ChangeLog
