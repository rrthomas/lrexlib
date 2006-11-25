/* lpcre.c - PCRE regular expression library */
/* (c) Reuben Thomas 2000-2006 */
/* (c) Shmuel Zeigerman 2004-2006 */

#include <pcre.h>
#include "lua.h"
#include "common.h"

static flag_pair pcre_flags[] = {
  { "MAJOR",                         PCRE_MAJOR },
  { "MINOR",                         PCRE_MINOR },
/*---------------------------------------------------------------------------*/
  { "CASELESS",                      PCRE_CASELESS },
  { "MULTILINE",                     PCRE_MULTILINE },
  { "DOTALL",                        PCRE_DOTALL },
  { "EXTENDED",                      PCRE_EXTENDED },
  { "ANCHORED",                      PCRE_ANCHORED },
  { "DOLLAR_ENDONLY",                PCRE_DOLLAR_ENDONLY },
  { "EXTRA",                         PCRE_EXTRA },
  { "NOTBOL",                        PCRE_NOTBOL },
  { "NOTEOL",                        PCRE_NOTEOL },
  { "UNGREEDY",                      PCRE_UNGREEDY },
  { "NOTEMPTY",                      PCRE_NOTEMPTY },
  { "UTF8",                          PCRE_UTF8 },
#if PCRE_MAJOR >= 4
  { "NO_AUTO_CAPTURE",               PCRE_NO_AUTO_CAPTURE },
  { "NO_UTF8_CHECK",                 PCRE_NO_UTF8_CHECK },
#endif
#if PCRE_MAJOR >= 5
  { "AUTO_CALLOUT",                  PCRE_AUTO_CALLOUT },
  { "PARTIAL",                       PCRE_PARTIAL },
#endif
#if PCRE_MAJOR >= 6
  { "DFA_SHORTEST",                  PCRE_DFA_SHORTEST },
  { "DFA_RESTART",                   PCRE_DFA_RESTART },
  { "FIRSTLINE",                     PCRE_FIRSTLINE },
#  if PCRE_MAJOR >= 7 || PCRE_MINOR >= 7
  { "DUPNAMES",                      PCRE_DUPNAMES },
  { "NEWLINE_CR",                    PCRE_NEWLINE_CR },
  { "NEWLINE_LF",                    PCRE_NEWLINE_LF },
  { "NEWLINE_CRLF",                  PCRE_NEWLINE_CRLF },
#  endif
#endif
/*---------------------------------------------------------------------------*/
  { "ERROR_NOMATCH",                 PCRE_ERROR_NOMATCH },
  { "ERROR_NULL",                    PCRE_ERROR_NULL },
  { "ERROR_BADOPTION",               PCRE_ERROR_BADOPTION },
  { "ERROR_BADMAGIC",                PCRE_ERROR_BADMAGIC },
  { "ERROR_UNKNOWN_NODE",            PCRE_ERROR_UNKNOWN_NODE },
  { "ERROR_NOMEMORY",                PCRE_ERROR_NOMEMORY },
  { "ERROR_NOSUBSTRING",             PCRE_ERROR_NOSUBSTRING },
#if PCRE_MAJOR >= 4
  { "ERROR_MATCHLIMIT",              PCRE_ERROR_MATCHLIMIT },
  { "ERROR_CALLOUT",                 PCRE_ERROR_CALLOUT },
  { "ERROR_BADUTF8",                 PCRE_ERROR_BADUTF8 },
  { "ERROR_BADUTF8_OFFSET",          PCRE_ERROR_BADUTF8_OFFSET },
#endif
#if PCRE_MAJOR >= 5
  { "ERROR_PARTIAL",                 PCRE_ERROR_PARTIAL },
  { "ERROR_BADPARTIAL",              PCRE_ERROR_BADPARTIAL },
  { "ERROR_INTERNAL",                PCRE_ERROR_INTERNAL },
  { "ERROR_BADCOUNT",                PCRE_ERROR_BADCOUNT },
#endif
#if PCRE_MAJOR >= 6
  { "ERROR_DFA_UITEM",               PCRE_ERROR_DFA_UITEM },
  { "ERROR_DFA_UCOND",               PCRE_ERROR_DFA_UCOND },
  { "ERROR_DFA_UMLIMIT",             PCRE_ERROR_DFA_UMLIMIT },
  { "ERROR_DFA_WSSIZE",              PCRE_ERROR_DFA_WSSIZE },
  { "ERROR_DFA_RECURSE",             PCRE_ERROR_DFA_RECURSE },
#  if PCRE_MAJOR >= 7 || PCRE_MINOR >= 7
  { "ERROR_RECURSIONLIMIT",          PCRE_ERROR_RECURSIONLIMIT },
#  endif
#endif
/*---------------------------------------------------------------------------*/
  { "INFO_OPTIONS",                  PCRE_INFO_OPTIONS },
  { "INFO_SIZE",                     PCRE_INFO_SIZE },
  { "INFO_CAPTURECOUNT",             PCRE_INFO_CAPTURECOUNT },
  { "INFO_BACKREFMAX",               PCRE_INFO_BACKREFMAX },
#if PCRE_MAJOR >= 4
  { "INFO_FIRSTBYTE",                PCRE_INFO_FIRSTBYTE },
#endif
  { "INFO_FIRSTCHAR",                PCRE_INFO_FIRSTCHAR },
  { "INFO_FIRSTTABLE",               PCRE_INFO_FIRSTTABLE },
  { "INFO_LASTLITERAL",              PCRE_INFO_LASTLITERAL },
#if PCRE_MAJOR >= 4
  { "INFO_NAMEENTRYSIZE",            PCRE_INFO_NAMEENTRYSIZE },
  { "INFO_NAMECOUNT",                PCRE_INFO_NAMECOUNT },
  { "INFO_NAMETABLE",                PCRE_INFO_NAMETABLE },
  { "INFO_STUDYSIZE",                PCRE_INFO_STUDYSIZE },
#endif
#if PCRE_MAJOR >= 5
  { "INFO_DEFAULT_TABLES",           PCRE_INFO_DEFAULT_TABLES },
#endif
/*---------------------------------------------------------------------------*/
#if PCRE_MAJOR >= 4
  { "CONFIG_UTF8",                   PCRE_CONFIG_UTF8 },
  { "CONFIG_NEWLINE",                PCRE_CONFIG_NEWLINE },
  { "CONFIG_LINK_SIZE",              PCRE_CONFIG_LINK_SIZE },
  { "CONFIG_POSIX_MALLOC_THRESHOLD", PCRE_CONFIG_POSIX_MALLOC_THRESHOLD },
  { "CONFIG_MATCH_LIMIT",            PCRE_CONFIG_MATCH_LIMIT },
  { "CONFIG_STACKRECURSE",           PCRE_CONFIG_STACKRECURSE },
#endif
#if PCRE_MAJOR >= 5
  { "CONFIG_UNICODE_PROPERTIES",     PCRE_CONFIG_UNICODE_PROPERTIES },
#endif
#ifdef PCRE_CONFIG_MATCH_LIMIT_RECURSION
  { "CONFIG_MATCH_LIMIT_RECURSION",  PCRE_CONFIG_MATCH_LIMIT_RECURSION },
#endif
/*---------------------------------------------------------------------------*/
#if PCRE_MAJOR >= 4
  { "EXTRA_STUDY_DATA",              PCRE_EXTRA_STUDY_DATA },
  { "EXTRA_MATCH_LIMIT",             PCRE_EXTRA_MATCH_LIMIT },
  { "EXTRA_CALLOUT_DATA",            PCRE_EXTRA_CALLOUT_DATA },
#endif
#if PCRE_MAJOR >= 5
  { "EXTRA_TABLES",                  PCRE_EXTRA_TABLES },
#endif
#ifdef PCRE_EXTRA_MATCH_LIMIT_RECURSION
  { "EXTRA_MATCH_LIMIT_RECURSION",   PCRE_EXTRA_MATCH_LIMIT_RECURSION },
#endif
/*---------------------------------------------------------------------------*/
  { NULL, 0 }
};

int Lpcre_get_flags (lua_State *L) {
  return get_flags (L, pcre_flags);
}

