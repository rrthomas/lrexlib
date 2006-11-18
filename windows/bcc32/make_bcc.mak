# Makefile for LuaPcre.dll, LuaPcre_nr.dll, LuaPosix1.dll and LuaPosix2.dll -
# using Borland tools under Windows (make.exe, bcc32.exe, ilink32.exe)
#
# author: Shmuel Zeigerman (shmuz@actcom.co.il)
#

#==========================================================================
#   POSIX1          --> the library by:   Henry Spencer
#   POSIX2          --> the library by:   John Maddock
#   PCRE, PCRE_NR   --> the library by:   Philip Hazel
#==========================================================================

# Definitions common for all the targets
#==========================================================================
!include .\config.mak

SRCDIR       = ..\..\src
SRCS_POSIX   = $(SRCDIR)\lposix.c $(SRCDIR)\common.c
OBJS_POSIX   = lposix.obj common.obj
SRCS_PCRE    = $(SRCDIR)\lpcre.c $(SRCDIR)\common.c
OBJS_PCRE    = lpcre.obj common.obj
STARTUP      = c0d32.obj
LFLAGS       = -aa -Gi -Gn -Tpd -x

# PCRE, multi-threaded build
#==========================================================================
DEF_PCRE     = -DREX_OPENLIB=luaopen_$(TARG_PCRE) \
               -DREX_LIBNAME=\"$(TARG_PCRE)\" -D$(CMDLINE)
DEFFILE_PCRE = $(TARG_PCRE).def
INC_PCRE     = $(DIR_LUA);$(DIR_PCRE)
LIBS_PCRE    = $(LIB_LUA) $(LIB_PCRE) import32.lib cw32mt.lib
LIBD_PCRE    = $(DIR_BCB)\LIB;$(DIR_LUA);$(DIR_PCRE)
FLAGS_PCRE   = -A -tWD -tWM

# PCRE with recursion disabled, multi-threaded build
#==========================================================================
DEF_PCRE_NR  = -DREX_OPENLIB=luaopen_$(TARG_PCRE_NR) \
               -DREX_LIBNAME=\"$(TARG_PCRE_NR)\" -D$(CMDLINE)
DEFFILE_PCRE_NR = $(TARG_PCRE_NR).def
LIBS_PCRE_NR = $(LIB_LUA) $(LIB_PCRE_NR) import32.lib cw32mt.lib

# POSIX1, single-threaded build
#==========================================================================
DEF_POSIX1   = -DREX_OPENLIB=luaopen_$(TARG_POSIX1) \
               -DREX_LIBNAME=\"$(TARG_POSIX1)\" -D$(CMDLINE)
DEFFILE_POSIX1 = $(TARG_POSIX1).def
INC_POSIX1   = $(DIR_LUA);$(DIR_POSIX1)
LIBS_POSIX1  = $(LIB_LUA) $(LIB_POSIX1) import32.lib cw32.lib
LIBD_POSIX1  = $(DIR_BCB)\LIB;$(DIR_LUA);$(DIR_POSIX1)
FLAGS_POSIX1 = -A -tWD


# POSIX2, multi-threaded build
#==========================================================================
#
# File regex.hpp was copied from $(DIR_POSIX2)/boost to $(DIR_POSIX2)
#
DEF_POSIX2   = -DREX_OPENLIB=luaopen_$(TARG_POSIX2) \
               -DREX_LIBNAME=\"$(TARG_POSIX2)\" -D$(CMDLINE)
DEFFILE_POSIX2 = $(TARG_POSIX2).def
INC_POSIX2   = $(DIR_LUA);$(DIR_POSIX2);$(DIR_POSIX2)/boost
LIBS_POSIX2  = $(LIB_LUA) $(LIB_POSIX2) import32.lib cw32mt.lib
LIBD_POSIX2  = $(DIR_BCB)\LIB;$(DIR_LUA);$(DIR_POSIX2)\libs\regex\build\bcb
FLAGS_POSIX2 = -tWD -tWM -DREX_POSIX_EXT

# -------------------------------------------------------------------------
# TARGETS
# -------------------------------------------------------------------------
all     : posix1  posix2  pcre  pcre_nr
posix1  : $(TARG_POSIX1).dll
posix2  : $(TARG_POSIX2).dll
pcre    : $(TARG_PCRE).dll
pcre_nr : $(TARG_PCRE_NR).dll

$(TARG_PCRE).dll : $(SRCS_PCRE)
	lua makedef.lua $(TARG_PCRE)
	bcc32 -c $(FLAGS_PCRE) -I$(INC_PCRE) $(DEF_PCRE) $(SRCS_PCRE)
	ilink32 -L$(LIBD_PCRE) $(LFLAGS) $(STARTUP) $(OBJS_PCRE), $(TARG_PCRE),, \
          $(LIBS_PCRE), $(DEFFILE_PCRE),

$(TARG_PCRE_NR).dll : $(SRCS_PCRE)
	lua makedef.lua $(TARG_PCRE_NR)
	bcc32 -c $(FLAGS_PCRE) -I$(INC_PCRE) $(DEF_PCRE_NR) $(SRCS_PCRE)
	ilink32 -L$(LIBD_PCRE) $(LFLAGS) $(STARTUP) $(OBJS_PCRE), $(TARG_PCRE_NR),, \
          $(LIBS_PCRE_NR), $(DEFFILE_PCRE_NR),

$(TARG_POSIX1).dll : $(SRCS_POSIX)
	lua makedef.lua $(TARG_POSIX1)
	bcc32 -c $(FLAGS_POSIX1) -I$(INC_POSIX1) $(DEF_POSIX1) $(SRCS_POSIX)
	ilink32 -L$(LIBD_POSIX1) $(LFLAGS) $(STARTUP) $(OBJS_POSIX), \
          $(TARG_POSIX1),, $(LIBS_POSIX1), $(DEFFILE_POSIX1),

$(TARG_POSIX2).dll : $(SRCS_POSIX)
	lua makedef.lua $(TARG_POSIX2)
	bcc32 -c $(FLAGS_POSIX2) -I$(INC_POSIX2) $(DEF_POSIX2) $(SRCS_POSIX)
	ilink32 -L$(LIBD_POSIX2) $(LFLAGS) $(STARTUP) $(OBJS_POSIX), \
          $(TARG_POSIX2),, $(LIBS_POSIX2), $(DEFFILE_POSIX2),


clean :
	del *.obj
	del *.tds
	del *.lib
	del *.def

