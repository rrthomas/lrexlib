# makefile for rex_tre library

include defaults.mak

# === USER SETTINGS ===
# ===========================================================================

# These are default values.
INC_TRE =
LIB_TRE = -ltre

# If the default settings don't work for your system,
# try to uncomment and edit the settings below.
#INC_TRE = -I/usr/include
#LIB_TRE = -lc

# WARNING:
#   If you want to use a TRE regex library that is not the system
#   default, make sure you set both the INC_TRE and LIB_TRE
#   variables correctly, as if a header file and library are used
#   which do not match, you may well get segmentation faults (or
#   worse).

# The following lines work for the rxspencer library, when installed
# under /usr (note the above warning!)
#INC_TRE = -I/usr/include/rxspencer
#LIB_TRE = -lrxspencer

# Target name
TRG = rex_tre

# ===========================================================================
# === END OF USER SETTINGS ===

OBJ    = ltre.o common.o

include common.mak

# static TRE regexp library binding
ar_tre: $(TRG_AR)

# dynamic TRE regexp library binding
so_tre: $(TRG_SO)

# Dependencies
ltre.o: ltre.c common.h algo.h
common.o: common.c common.h

# (End of Makefile)
