# makefile for rex_posix library

# === USER SETTINGS ===
# ===========================================================================

# These are default values.
INC_LUA   =
LIB_LUA   =
INC_POSIX =
LIB_POSIX =

# If the default settings don't work for your system,
# try to uncomment and edit the settings below.
#INC_LUA   = -I/usr/local/include
#LIB_LUA   = -llua -llualib
#INC_POSIX = -I/usr/include
#LIB_POSIX = -lc

# WARNING:
#   If you want to use a POSIX regex library that is not the system
#   default, make sure you set both the INC_POSIX and LIB_POSIX
#   variables correctly, as if a header file and library are used
#   which do not match, you may well get segmentation faults (or
#   worse).

# The following lines work for the rxspencer library, when installed
# under /usr (note the above warning!)
#INC_POSIX = -I/usr/include/rxspencer
#LIB_POSIX = -lrxspencer

# Uncomment this if you wish to use Compat-5.1:
#   * copy compat-5.1.c and compat-5.1.h into this directory
#   * only has effect when building for Lua 5.0.
#COMPAT51 = -DCOMPAT51

# Common settings
CFLAGS = -W -Wall -O2 $(INC_LUA) $(INC_POSIX) $(COMPAT51)
AR = ar rcu
CC = gcc

# ===========================================================================
# === END OF USER SETTINGS ===

V = 1.20

NAME = rex_posix

OBJ    = lposix.o common.o
TRG_AR = lib$(NAME).a
TRG_SO = $(NAME).so

all: $(TRG_AR) $(TRG_SO)

# static POSIX regexp library binding
ar_posix: $(TRG_AR)

# dynamic POSIX regexp library binding
so_posix: $(TRG_SO)

$(TRG_AR): $(OBJ)
	$(AR) $@ $^

$(TRG_SO): $(OBJ)
	ld -o $@.$V -shared $^ $(LIB_POSIX) $(LIB_LUA)
	ln -fs $@.$V $@

clean:
	rm -f $(OBJ) $(TRG_AR) $(TRG_SO)*

# Dependencies
lposix.o: lposix.c common.h
common.o: common.c common.h

# (End of Makefile)
