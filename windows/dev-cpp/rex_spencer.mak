# Project: rex_spencer

# User Settings ------------------------------------------------------------
# -1- path of Lua include files
LUAINC = s:\progr\lib\lua\lua_5.1

# -2- path of Spencer's include files
REGEXINC = s:\progr\lib\henry_spencer

# -3- path of lua5.1.dll and rxspencer.dll
DLLPATH = c:\exe

# -4- path to install rex_spencer.dll
INSTALLPATH = s:\exe\lib\lua\5.1
# --------------------------------------------------------------------------

PROJECT     = rex_spencer
MYINCS      = -I$(REGEXINC) -I$(LUAINC) 
MYLIBS      = -L$(DLLPATH) -lrxspencer -llua5.1
OBJ         = lposix.o common.o
MYCFLAGS    = -W -Wall -O2
EXPORTED    = 'luaopen_$(PROJECT)'
SRCPATH     = ..\..\src
TESTPATH    = ..\..\test
TESTNAME    = spencer

include _devcpp.mak

lposix.o  : common.h algo.h
common.o  : common.h

