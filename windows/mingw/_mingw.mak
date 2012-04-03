# Use with GNU Make.

# User Settings ------------------------------------------------------------

# Path of Lua include files.
# Name of Lua DLL to link to (.dll should be omitted).
# Name of Lua interpreter.
# Path to install the built DLL.

ifeq ($(LUAVERSION),51)
  LUAINC = s:\progr\work\system\include\lua51
  LUADLL = lua5.1
  LUAEXE = lua.exe
  INSTALLPATH = s:\exe\lib\lua\5.1
else
  LUAINC = s:\progr\work\system\include\lua52
  LUADLL = lua52
  LUAEXE = lua52.exe
  INSTALLPATH = s:\exe\lib\lua\5.2
endif

# --------------------------------------------------------------------------

LIBS       = -l$(LUADLL) $(MYLIBS) -s
INCS       = -I$(LUAINC) $(MYINCS)
BIN        = $(PROJECT).dll
DEFFILE    = $(PROJECT).def
BININSTALL = $(INSTALLPATH)\$(BIN)
CC         = gcc.exe
CFLAGS     = $(INCS) -DREX_OPENLIB=luaopen_$(PROJECT) \
  -DREX_LIBNAME=\"$(PROJECT)\" $(MYCFLAGS)

.PHONY: all install test clean

vpath %.c $(SRCPATH)
vpath %.h $(SRCPATH)

all: $(BIN)

clean:
	del $(OBJ) $(BIN) $(DEFFILE)

install: $(BININSTALL)

test:
	cd $(TESTPATH) && $(LUAEXE) runtest.lua $(TESTNAME) -d$(CURDIR)

$(BIN): $(OBJ) $(DEFFILE)
	$(CC) $(DEFFILE) $(OBJ) $(LIBS) -o $@ -shared

$(DEFFILE):
	echo EXPORTS > $@
	for %%d in ($(EXPORTED)) do echo   %%d>> $@

$(BININSTALL): $(BIN)
	copy /Y $< $@
