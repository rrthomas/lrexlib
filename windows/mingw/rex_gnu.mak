# Project: rex_gnu

# User Settings ------------------------------------------------------------
# path of Lua include files
LUAINC = s:\progr\work\system\include

# path of GNU include files
REGEXINC = s:\progr\work\system\include\gnuregex

# path of Lua DLL and regex2.dll
DLLPATH = c:\exe

# name of Lua DLL to link to (.dll should be omitted)
LUADLL = lua5.1

# path to install rex_gnu.dll
INSTALLPATH = s:\exe\lib\lua\5.1
# --------------------------------------------------------------------------

PROJECT     = rex_gnu
MYINCS      = -I$(REGEXINC) -I$(LUAINC) 
MYLIBS      = -L$(DLLPATH) -lregex2 -l$(LUADLL)
OBJ         = lgnu.o common.o
MYCFLAGS    = -W -Wall -O2
EXPORTED    = 'luaopen_$(PROJECT)'
SRCPATH     = ..\..\src;..\..\src\gnu
TESTPATH    = ..\..\test
TESTNAME    = gnu

include _mingw.mak

lgnu.o    : common.h algo.h
common.o  : common.h

