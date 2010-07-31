# Makefile for lrexlib

# See src/*/Makefile and src/defaults.mak for user-definable settings

REGNAMES = gnu pcre posix oniguruma tre

all:
	@for i in $(REGNAMES); do \
	  make -C src/$$i; \
	done

check: all
	@for i in $(REGNAMES); do \
	  make -C src/$$i check; \
	done

clean:
	@for i in $(REGNAMES); do \
	  make -C src/$$i clean; \
	done

dist:
	git2cl > ChangeLog
