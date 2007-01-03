all: pcre posix

pcre:
	make -C src -f rex_pcre.mak

posix:
	make -C src -f rex_posix.mak
