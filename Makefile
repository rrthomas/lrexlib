# Makefile for lrexlib

# See src/*/Makefile and src/defaults.mak for user-definable settings

include src/defaults.mak

REGNAMES = gnu pcre posix oniguruma tre
DISTFILE = lrexlib-$(V).$(MINORV).zip

all:
	@for i in $(REGNAMES); do \
	  make -C src/$$i; \
	done
	@make -C doc

check: all
	@for i in $(REGNAMES); do \
	  make -C src/$$i check; \
	done

clean:
	@for i in $(REGNAMES); do \
	  make -C src/$$i clean; \
	done
	@make -C doc clean

dist: all
	git2cl > ChangeLog
	rm -f $(DISTFILE)
	zip $(DISTFILE) -r . -x ".git/*" "*.gitignore" "*.o" "*.a" "*.so" "*.so.*" "*.zip" "*SciTE.properties" "*scite.properties"
