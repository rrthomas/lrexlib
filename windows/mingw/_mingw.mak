# Use with GNU Make.

VERSION = 2.7.2

# User Settings ------------------------------------------------------------

# Target Lua version (51 for Lua 5.1; 52 for Lua 5.2).
LUAVERSION = 51

# INSTALLPATH : Path to install the built DLL.
# LUADLL      : Name of Lua DLL to link to (.dll should be omitted).
# LUAEXE      : Name of Lua interpreter.
# LUAINC      : Path of Lua include files.
# LIBPATH     : Path of lua5.1.dll, lua52.dll, pcre.dll, etc.

LIBPATH = c:\exe32

ifeq ($(LUAVERSION),51)
  INSTALLPATH = s:\exe\lib32\lua\5.1
  LUADLL = lua5.1
  LUAEXE = lua.exe
  LUAINC = s:\progr\work\system\include\lua\5.1
  MYCFLAGS += -DREX_CREATEGLOBALVAR
else
  INSTALLPATH = s:\exe\lib32\lua\5.2
  LUADLL = lua52
  LUAEXE = lua52.exe
  LUAINC = s:\progr\work\system\include\lua\5.2
# MYCFLAGS += -DREX_CREATEGLOBALVAR
endif

# --------------------------------------------------------------------------

BIN        = $(PROJECT).dll
BININSTALL = $(INSTALLPATH)\$(BIN)
CC         = mingw32-gcc
CFLAGS     = -W -Wall -O2 $(INCS) -DREX_OPENLIB=luaopen_$(PROJECT) \
             -DREX_LIBNAME=\"$(PROJECT)\" -DVERSION=\"$(VERSION)\" $(MYCFLAGS)
DEFFILE    = $(PROJECT).def
EXPORTED   = luaopen_$(PROJECT)
INCS       = -I$(LUAINC) $(MYINCS)
LIBS       = -l$(LUADLL) $(MYLIBS) -s
SRCPATH    = ..\..\src
TESTPATH   = ..\..\test

.PHONY: all install test clean

vpath %.c $(SRCPATH);$(SRCPATH)\$(PROJDIR)
vpath %.h $(SRCPATH);$(SRCPATH)\$(PROJDIR)

all: $(BIN)

clean:
	del $(OBJ) $(BIN) $(DEFFILE)

install: $(BININSTALL)

test:
	cd $(TESTPATH) && $(LUAEXE) runtest.lua $(TESTNAME) -d$(CURDIR)

$(BIN): $(OBJ) $(DEFFILE)
	$(CC) $(DEFFILE) $(OBJ) -L$(LIBPATH) $(LIBS) -o $@ -shared

$(DEFFILE):
	echo EXPORTS > $@
	for %%d in ($(EXPORTED)) do echo   %%d>> $@

$(BININSTALL): $(BIN)
	copy /Y $< $@
