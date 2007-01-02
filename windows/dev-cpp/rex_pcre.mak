# rex_pcre.dll

# 1. Specify root directory of Dev-Cpp (MinGW) installation
# ---------------------------------------------------------
ROOT = C:/Dev-Cpp

# 2. Specify Lua include directory and full name of Lua library
# -------------------------------------------------------------
INC_LUA = C:/Progr/lib/lua/lua_5.1
LIB_LUA = C:/Progr/lib/lua/lua_5.1/liblua5.1.a

# 3. Specify PCRE include directory and full name of PCRE library
# ---------------------------------------------------------------
INC_REX = C:/Progr/lib/pcre/pcre_7.0
LIB_REX = C:/Progr/lib/pcre/pcre_7.0/libpcre.a

# 4. Specify target name without extension
# ----------------------------------------
TRG = rex_pcre

# --------------------
# END OF USER SETTINGS
# --------------------

DEFS = -DREX_OPENLIB=luaopen_$(TRG) -DREX_LIBNAME=\"$(TRG)\"
CC   = gcc.exe
OBJ  = ./lpcre.o ./common.o ./lpcre_f.o
LINKOBJ  = ./lpcre.o ./common.o ./lpcre_f.o
LIBS =  -L"$(ROOT)/lib" --no-export-all-symbols --add-stdcall-alias $(LIB_LUA) $(LIB_REX) -s 
INCS =  -I"$(ROOT)/include"  -I"$(INC_REX)"  -I"$(INC_LUA)" 
BIN  = $(TRG).dll
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

./lpcre.o: ../../src/lpcre.c
	$(CC) -c ../../src/lpcre.c -o ./lpcre.o $(CFLAGS)

./common.o: ../../src/common.c
	$(CC) -c ../../src/common.c -o ./common.o $(CFLAGS)

./lpcre_f.o: ../../src/lpcre_f.c
	$(CC) -c ../../src/lpcre_f.c -o ./lpcre_f.o $(CFLAGS)
