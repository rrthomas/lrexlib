# Makefile for lrexlib

# See src/*/Makefile and src/defaults.mak for user-definable settings

include src/defaults.mak

REGNAMES = gnu pcre posix oniguruma tre
DISTFILE = lrexlib-$(V).zip

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
	cp -a doc/index.txt README
	rm -f $(DISTFILE)
	zip $(DISTFILE) -r . -x ".git/*" "*.gitignore" "*.o" "*.a" "*.so" "*.so.*"
