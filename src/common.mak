# common settings for lrexlib

# === USER SETTINGS ===
# ===========================================================================

# These are default values.
INC_LUA  =
LIB_LUA  =

# If the default settings don't work for your system,
# try to uncomment and edit the settings below.
#INC_LUA  = -I/usr/local/include
#LIB_LUA  = -llua

MYCFLAGS = -W -Wall -O2 $(INC_LUA) $(INC_PCRE)
AR = ar rcu
CC = gcc

# ===========================================================================
# === END OF USER SETTINGS ===
