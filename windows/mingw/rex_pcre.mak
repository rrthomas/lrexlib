# Project: rex_pcre

# User Settings ------------------------------------------------------------
# path of PCRE include files
REGEXINC = $(PATH_WORK)\system\include\pcre
# --------------------------------------------------------------------------

PROJECT  = rex_pcre
MYINCS   = -I$(REGEXINC)
MYLIBS   = -lpcre
OBJ      = lpcre.o lpcre_f.o common.o
PROJDIR  = pcre
TESTNAME = pcre

include _mingw.mak

lpcre.o   : common.h algo.h
lpcre_f.o : common.h
common.o  : common.h
