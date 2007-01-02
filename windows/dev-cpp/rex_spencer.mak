# rex_spencer.dll

# 1. Specify root directory of Dev-Cpp (MinGW) installation
# ---------------------------------------------------------
ROOT = C:/Dev-Cpp

# 2. Specify Lua include directory and full name of Lua library
# -------------------------------------------------------------
INC_LUA = C:/Progr/lib/lua/lua_5.1
LIB_LUA = C:/Progr/lib/lua/lua_5.1/liblua5.1.a

# 3. Specify POSIX include directory and full name of POSIX library
# -----------------------------------------------------------------
INC_REX = C:/Progr/lib/henry_spencer
LIB_REX = C:/Progr/lib/henry_spencer/librxspencer.dll.a

# 4. Specify target name without extension
# ----------------------------------------
TRG = rex_spencer

# --------------------
# END OF USER SETTINGS
# --------------------

DEFS = -DREX_OPENLIB=luaopen_$(TRG) -DREX_LIBNAME=\"$(TRG)\"
CC   = gcc.exe
OBJ  = ./lposix.o ./common.o
LINKOBJ  = ./lposix.o ./common.o
LIBS =  -L"$(ROOT)/lib" --no-export-all-symbols --add-stdcall-alias $(LIB_REX) $(LIB_LUA) -s
INCS =  -I"$(ROOT)/include"  -I"$(INC_LUA)"  -I"$(INC_REX)" 
BIN  = $(TRG).dll
#CFLAGS = $(INCS) $(DEFS) -DREX_API=__declspec(dllexport)  
CFLAGS = $(INCS) $(DEFS)
RM = rm -f

.PHONY: all all-before all-after clean clean-custom

all: all-before $(BIN) all-after


clean: clean-custom
	${RM} $(OBJ) $(BIN)

DLLWRAP=dllwrap.exe
DEFFILE=lib$(TRG).def
STATICLIB=lib$(TRG).a

$(BIN): $(LINKOBJ)
	echo EXPORTS > $(DEFFILE)
	echo luaopen_$(TRG) >> $(DEFFILE)
	$(DLLWRAP) --def $(DEFFILE) --implib $(STATICLIB) $(LINKOBJ) $(LIBS) -o $(BIN)

./lposix.o: ../../src/lposix.c
	$(CC) -c ../../src/lposix.c -o ./lposix.o $(CFLAGS)

./common.o: ../../src/common.c
	$(CC) -c ../../src/common.c -o ./common.o $(CFLAGS)
