# Project: rex_pcre

# User Settings ------------------------------------------------------------
# -1- path of Lua include files
LUAINC = s:\progr\lib\lua\lua_5.1

# -2- path of PCRE include files
REGEXINC = s:\progr\lib\pcre\pcre_7.1

# -3- path of lua5.1.dll and pcre.dll
DLLPATH = c:\exe

# -4- path to install rex_pcre.dll
INSTALLPATH = s:\exe\lib\lua\5.1
# --------------------------------------------------------------------------

PROJECT     = rex_pcre
MYINCS      = -I$(REGEXINC) -I$(LUAINC) 
MYLIBS      = -L$(DLLPATH) -lpcre -llua5.1
OBJ         = lpcre.o lpcre_f.o common.o
MYCFLAGS    = -W -Wall -O2
EXPORTED    = 'luaopen_$(PROJECT)'
SRCPATH     = ..\..\src
TESTPATH    = ..\..\test
TESTNAME    = pcre

include _devcpp.mak

lpcre.o   : common.h algo.h
lpcre_f.o : common.h
common.o  : common.h

