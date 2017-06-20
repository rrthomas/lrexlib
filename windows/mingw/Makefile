# Makefile for lrexlib

ifeq ($(DIRBIT),64)
MKFILES = \
  rex_onig.mak    \
  rex_pcre.mak    \
  rex_pcre2.mak   \
  rex_tre.mak
else
MKFILES = \
  rex_gnu.mak     \
  rex_onig.mak    \
  rex_pcre.mak    \
  rex_pcre2.mak   \
  rex_spencer.mak \
  rex_tre.mak
endif

LOOP = @for %%d in ($(MKFILES)) do $(MAKE) -f %%d

all: build test

build:
	$(LOOP)

test:
	$(LOOP) test

install:
	$(LOOP) install

clean:
	del *.o *.def *.dll

.PHONY: all build test install clean
