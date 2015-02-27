# Use with GNU Make.

VERSION = 2.8.0

# User Settings ------------------------------------------------------------

# Target Lua version (51 for Lua 5.1; 52 for Lua 5.2).
LUAVERSION = 51
LUADOTVERSION = $(subst 5,5.,$(LUAVERSION))

# INSTALLPATH : Path to install the built DLL.
# LUADLL      : Name of Lua DLL to link to (.dll should be omitted).
# LUAEXE      : Name of Lua interpreter.
# LUAINC      : Path of Lua include files.
# LIBPATH     : Path of lua51.dll, lua52.dll, pcre.dll, etc.

INSTALLPATH = s:\exe\lib32\lua\$(LUADOTVERSION)
LUADLL      = lua$(LUAVERSION)
LUAINC      = s:\progr\work\system\include\lua\$(LUADOTVERSION)
LIBPATH     = c:\exe32

ifeq ($(LUAVERSION),51)
  LUAEXE = lua.exe
  CREATEGLOBAL = -DREX_CREATEGLOBALVAR
else
  LUAEXE = lua$(LUAVERSION).exe
endif

ifeq ($(LUAVERSION),53)
  LUA_COMPAT = -DLUA_COMPAT_5_2
endif
# --------------------------------------------------------------------------

BIN        = $(PROJECT).dll
BININSTALL = $(INSTALLPATH)\$(BIN)
CC         = mingw32-gcc
AR         = ar rcu
RANLIB     = ranlib
CFLAGS     = -W -Wall -O2 $(INCS) -DREX_OPENLIB=luaopen_$(PROJECT) \
             -DREX_LIBNAME=\"$(PROJECT)\" -DVERSION=\"$(VERSION)\" \
             $(CREATEGLOBAL) $(LUA_COMPAT) $(MYCFLAGS)
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

lib$(PROJECT)$(LUAVERSION).a: $(OBJ)
	$(AR) $@ $?
	$(RANLIB) $@

$(DEFFILE):
	echo EXPORTS > $@
	for %%d in ($(EXPORTED)) do echo   %%d>> $@

$(BININSTALL): $(BIN)
	copy /Y $< $@
