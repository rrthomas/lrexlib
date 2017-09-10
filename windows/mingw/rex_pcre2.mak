# Project: rex_pcre2

# User Settings ------------------------------------------------------------
# path of PCRE2 include files
REGEXINC = $(PATH_WORK)\system\include\pcre2
# --------------------------------------------------------------------------

PROJECT  = rex_pcre2
MYINCS   = -I$(REGEXINC)
MYCFLAGS = -DPCRE2_CODE_UNIT_WIDTH=8
MYLIBS   = -lpcre2
OBJ      = lpcre2.o lpcre2_f.o common.o
PROJDIR  = pcre2
TESTNAME = pcre2

include _mingw.mak

lpcre2.o   : common.h algo.h
lpcre2_f.o : common.h
common.o   : common.h
