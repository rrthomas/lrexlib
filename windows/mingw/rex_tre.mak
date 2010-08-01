# Project: rex_tre

# User Settings ------------------------------------------------------------
# path of Lua include files
LUAINC = s:\progr\work\system\include

# path of TRE include files
REGEXINC = s:\progr\work\system\include

# path of Lua DLL and tre.dll
DLLPATH = c:\exe

# name of Lua DLL to link to (.dll should be omitted)
LUADLL = lua5.1

# path to install rex_tre.dll
INSTALLPATH = s:\exe\lib\lua\5.1
# --------------------------------------------------------------------------

PROJECT     = rex_tre
MYINCS      = -I$(REGEXINC) -I$(LUAINC) 
MYLIBS      = -L$(DLLPATH) -ltre -l$(LUADLL)
OBJ         = ltre.o common.o
MYCFLAGS    = -W -Wall -O2
EXPORTED    = 'luaopen_$(PROJECT)'
SRCPATH     = ..\..\src;..\..\src\tre
TESTPATH    = ..\..\test
TESTNAME    = tre

include _mingw.mak

ltre.o    : common.h algo.h
common.o  : common.h

