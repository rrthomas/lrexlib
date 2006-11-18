# Configuration file.

#==========================================================================
#   POSIX1          --> the library by:   Henry Spencer
#   POSIX2          --> the library by:   John Maddock
#   PCRE, PCRE_NR   --> the library by:   Philip Hazel
#==========================================================================

#  Definitions common for all the targets
#==========================================================================
DIR_LUA      = \progr\lib\lua\lua_5.1
LIB_LUA      = lua5.1.lib
DIR_BCB      = "C:\Program Files\Borland\CBuilder5"

#  1. PCRE
#==========================================================================
DIR_PCRE     = \progr\lib\pcre\pcre_6.7
LIB_PCRE     = pcre.lib
TARG_PCRE    = rex_pcre

#  2. PCRE with recursion disabled
#==========================================================================
LIB_PCRE_NR  = pcre_nr.lib
TARG_PCRE_NR = rex_pcre_nr

#  3. POSIX1
#==========================================================================
DIR_POSIX1   = \progr\lib\henry_spencer
LIB_POSIX1   = libregex-bcc.lib
TARG_POSIX1  = rex_posix1

#  4. POSIX2
#==========================================================================
DIR_POSIX2   = E:\boost_1_31_0
LIB_POSIX2   = libboost_regex-bcb-mt-s-1_31.lib
TARG_POSIX2  = rex_posix2

