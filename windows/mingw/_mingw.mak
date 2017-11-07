# Use with GNU Make.

# Lrexlib version
VERSION = 2.9.0

# User Settings ------------------------------------------------------------

# Target Lua version (51 for Lua 5.1, etc.)
LUAVERSION = 51
LUADOTVERSION = $(subst 5,5.,$(LUAVERSION))

# Target bitness: 32 or 64
DIRBIT = 32
# GCC location (GCC32 and GCC64 are defined environment variables)
PATH = $(GCC$(DIRBIT))

# INSTALLPATH : Path to install the built DLL.
# LUADLL      : Lua DLL to link to (.dll should be omitted).
# LUAEXE      : Lua interpreter.
# LUAINC      : Path of Lua include files.
# LIBPATH     : Path of lua51.dll, lua52.dll, pcre.dll, etc.

INSTALLPATH = s:\exe\lib$(DIRBIT)\lua\$(LUADOTVERSION)
LUADLL      = lua$(LUAVERSION)
LUAINC      = $(PATH_SYSTEM)\include\lua\$(LUADOTVERSION)
LIBPATH     = $(CROOT)\Programs\EXE$(DIRBIT)

ifeq ($(LUAVERSION),51)
  LUAEXE = $(LIBPATH)\lua.exe
  CREATEGLOBAL = -DREX_CREATEGLOBALVAR
else
  LUAEXE = $(LIBPATH)\lua$(LUAVERSION).exe
endif

ifeq ($(LUAVERSION),53)
  LUA_COMPAT = -DLUA_COMPAT_5_2
endif
# --------------------------------------------------------------------------

BIN        = $(PROJECT).dll
BININSTALL = $(INSTALLPATH)\$(BIN)
CC         = gcc
AR         = ar rcu
RANLIB     = ranlib
CFLAGS     = -W -Wall -O2 $(INCS) -DREX_OPENLIB=luaopen_$(PROJECT) \
             -DREX_LIBNAME=\"$(PROJECT)\" -DVERSION=\"$(VERSION)\" \
             -m$(DIRBIT) $(CREATEGLOBAL) $(LUA_COMPAT) $(MYCFLAGS)
DEFFILE    = $(PROJECT).def
EXPORTED   = luaopen_$(PROJECT)
INCS       = -I$(LUAINC) $(MYINCS)
LIBS       = -l$(LUADLL) -m$(DIRBIT) -s $(MYLIBS)
SRCPATH    = ..\..\src
TESTPATH   = ..\..\test

.PHONY: all install test vtest clean

vpath %.c $(SRCPATH);$(SRCPATH)\$(PROJDIR)
vpath %.h $(SRCPATH);$(SRCPATH)\$(PROJDIR)

all: $(BIN)

clean:
	del $(OBJ) $(BIN) $(DEFFILE)

install: $(BININSTALL)

test:
	cd $(TESTPATH) && $(LUAEXE) runtest.lua $(TESTNAME) -d$(CURDIR)

vtest:
	cd $(TESTPATH) && $(LUAEXE) runtest.lua -v $(TESTNAME) -d$(CURDIR)

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
