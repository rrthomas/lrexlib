# Project: rex_tre

# User Settings ------------------------------------------------------------
# path of TRE include files
REGEXINC = s:\progr\work\system\include
# --------------------------------------------------------------------------

PROJECT  = rex_tre
MYINCS   = -I$(REGEXINC)
MYLIBS   = -ltre
# OBJ    = ltre.o ltre_w.o common.o
OBJ      = ltre.o common.o
PROJDIR  = tre
TESTNAME = tre

include _mingw.mak

ltre.o    : common.h algo.h
ltre_w.o  : common.h algo.h
common.o  : common.h
