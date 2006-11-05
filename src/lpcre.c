/* lpcre.c - PCRE regular expression library */
/* (c) Reuben Thomas 2000-2006 */
/* (c) Shmuel Zeigerman 2004-2006 */

#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <pcre.h>

#include "lua.h"
#include "lauxlib.h"
#include "common.h"

/* These 2 settings may be redefined from the command-line or the makefile.
 * They should be kept in sync between themselves and with the target name.
 */
#ifndef REX_LIBNAME
  #define REX_LIBNAME "rex_pcre"
#endif
#ifndef REX_OPENLIB
  #define REX_OPENLIB luaopen_rex_pcre
#endif

/* helper macro */
#define DIM(array) (sizeof(array)/sizeof(array[0]))

const char pcre_handle[] = REX_LIBNAME"?regex_handle";
const char pcre_typename[] = REX_LIBNAME"_regex";

typedef struct {
  pcre *pr;
  pcre_extra *extra;
  int *match;
  int ncapt;
  const unsigned char *tables;
} TPcre;

static const unsigned char *Lpcre_maketables(lua_State *L, int stackpos)
{
  const unsigned char *tables;
  char old_locale[256];
  const char *locale = luaL_checkstring(L, stackpos);

  strcpy(old_locale, setlocale(LC_CTYPE, NULL)); /* store the locale */
  if(NULL == setlocale(LC_CTYPE, locale))        /* set new locale */
    luaL_error(L, "cannot set locale");
  tables = pcre_maketables();              /* make tables with new locale */
  setlocale(LC_CTYPE, old_locale);         /* restore the old locale */
  return tables;
}

static int Lpcre_comp(lua_State *L)
{
  const char *error;
  int erroffset;
  TPcre *ud;
  size_t clen;  /* clen isn't used in PCRE */
  const char *pattern = luaL_checklstring(L, 1, &clen);
  int cflags = luaL_optint(L, 2, 0);
  const unsigned char *tables = NULL;

  if(lua_gettop(L) > 2 && !lua_isnil(L, 3))
    tables = Lpcre_maketables(L, 3);

  ud = (TPcre*)lua_newuserdata(L, sizeof(TPcre));
  luaL_getmetatable(L, pcre_handle);
  lua_setmetatable(L, -2);
  ud->match = NULL;
  ud->extra = NULL;
  ud->tables = tables; /* keep this for eventual freeing */

  ud->pr = pcre_compile(pattern, cflags, &error, &erroffset, tables);
  if(!ud->pr) {
    return luaL_error(L, "%s (pattern offset: %d)", error, erroffset+1);
                          /* show offset 1-based as it's common in Lua */
  }

  ud->extra = pcre_study(ud->pr, 0, &error);
  if(error) return luaL_error(L, "%s", error);

  pcre_fullinfo(ud->pr, ud->extra, PCRE_INFO_CAPTURECOUNT, &ud->ncapt);
  /* need (2 ints per capture, plus one for substring match) * 3/2 */
  ud->match = (int *) Lmalloc(L, (ud->ncapt + 1) * 3 * sizeof(int));

  return 1;
}

static void Lpcre_getargs(lua_State *L, TPcre **pud, const char **text,
                          size_t *text_len)
{
  *pud = (TPcre *)luaL_checkudata(L, 1, pcre_handle);
  luaL_argcheck(L, *pud != NULL, 1, "compiled regexp expected");
  *text = luaL_checklstring(L, 2, text_len);
}

typedef void (*Lpcre_push_matches) (lua_State *L, const char *text, TPcre *ud);

static void Lpcre_push_substrings (lua_State *L, const char *text, TPcre *ud)
{
  int i;
  int namecount;
  unsigned char *name_table;
  int name_entry_size;
  unsigned char *tabptr;
  const int *match = ud->match;

  lua_newtable(L);
  for (i = 1; i <= ud->ncapt; i++) {
    int j = i * 2;
    if (match[j] >= 0)
      lua_pushlstring(L, text + match[j], match[j + 1] - match[j]);
    else
      lua_pushboolean(L, 0);
    lua_rawseti(L, -2, i);
  }

  /* now do named subpatterns - NJG */
  pcre_fullinfo(ud->pr, ud->extra, PCRE_INFO_NAMECOUNT, &namecount);
  if (namecount <= 0)
    return;
  pcre_fullinfo(ud->pr, ud->extra, PCRE_INFO_NAMETABLE, &name_table);
  pcre_fullinfo(ud->pr, ud->extra, PCRE_INFO_NAMEENTRYSIZE, &name_entry_size);
  tabptr = name_table;
  for (i = 0; i < namecount; i++) {
    int n = (tabptr[0] << 8) | tabptr[1]; /* number of the capturing parenthesis */
    if (n > 0 && n <= ud->ncapt) {   /* check range */
      int j = n * 2;
      lua_pushstring(L, tabptr + 2); /* name of the capture, zero terminated */
      if (match[j] >= 0)
        lua_pushlstring(L, text + match[j], match[j + 1] - match[j]);
      else
        lua_pushboolean(L, 0);
      lua_rawset(L, -3);
    }
    tabptr += name_entry_size;
  }
}

static void Lpcre_push_offsets (lua_State *L, const char *text, TPcre *ud)
{
  int i, j, k;
  (void) text; /* suppress compiler warning */

  lua_newtable(L);
  for (i=1, j=1; i <= ud->ncapt; i++) {
    k = i * 2;
    if (ud->match[k] >= 0) {
      lua_pushinteger(L, ud->match[k] + 1);
      lua_rawseti(L, -2, j++);
      lua_pushinteger(L, ud->match[k+1]);
      lua_rawseti(L, -2, j++);
    }
    else {
      lua_pushboolean(L, 0);
      lua_rawseti(L, -2, j++);
      lua_pushboolean(L, 0);
      lua_rawseti(L, -2, j++);
    }
  }
}

typedef struct {
  lua_State *L;   /* lua state */
  int func_ref;   /* function reference in the registry */
} TPcreCalloutData;

static void put_integer(lua_State *L, const char *key, int value)
{
  lua_pushstring (L, key);
  lua_pushinteger (L, value);
  lua_rawset (L, -3);
}

static int Lpcre_callout (pcre_callout_block *block)
{
  TPcreCalloutData *data;
  lua_State *L;

  if (!block || !block->callout_data)
    return 0;

  data = (TPcreCalloutData*) block->callout_data;
  L = data->L;

  lua_rawgeti (L, LUA_REGISTRYINDEX, data->func_ref);
  lua_newtable (L);

  put_integer (L, "version",           block->version);
  put_integer (L, "callout_number",    block->callout_number);
  put_integer (L, "subject_length",    block->subject_length);
  put_integer (L, "start_match",       block->start_match);
  put_integer (L, "current_position",  block->current_position);
  put_integer (L, "capture_top",       block->capture_top);
  put_integer (L, "capture_last",      block->capture_last);
#if PCRE_MAJOR >= 5
  put_integer (L, "pattern_position",  block->pattern_position);
  put_integer (L, "next_item_length",  block->next_item_length);
#endif

  lua_call (L, 1, 1);
  return lua_tointeger (L, -1);
}

typedef struct {
  TPcre       *ud;                /*  1st parameter      */
  const char  *text;              /*  2nd parameter (a)  */
  size_t      textlen;            /*  2nd parameter (b)  */
  int         startoffset;        /*  3rd parameter      */
  int         eflags;             /*  4th parameter      */
  pcre_extra  own_extra;          /*  own pcre_extra block              */
  pcre_extra  *ptr_extra;         /*  pointer to used pcre_extra block  */
  TPcreCalloutData callout_data;  /*  keep the reference of the user function */
} TPcreExecParam;

static void LpcreProcessExecParams (lua_State *L, TPcreExecParam *p)
{
  Lpcre_getargs(L, &p->ud, &p->text, &p->textlen);    /* get 1st and 2nd params */
  p->startoffset = get_startoffset(L, 3, p->textlen); /* get 3rd param */
  p->eflags = luaL_optint(L, 4, 0);                   /* get 4th param */

  /* check callout and initialize if required */
  p->callout_data.func_ref = LUA_NOREF;
  p->ptr_extra = p->ud->extra;
  if((lua_gettop(L) >= 5) && lua_toboolean(L,5)) {
    luaL_checktype(L, 5, LUA_TFUNCTION);
    if(p->ptr_extra == NULL) {
      /* set up our own pcre_extra block */
      memset(&p->own_extra, 0, sizeof(pcre_extra));
      p->ptr_extra = &p->own_extra;
    }
    p->ptr_extra->flags |= PCRE_EXTRA_CALLOUT_DATA;
    p->ptr_extra->callout_data = &p->callout_data;

    lua_pushvalue(L, 5);
    p->callout_data.func_ref = luaL_ref(L, LUA_REGISTRYINDEX);
    p->callout_data.L = L;
  }
  else if (p->ptr_extra) {
    p->ptr_extra->callout_data = NULL;
  }
}

static int Lpcre_match_generic(lua_State *L, Lpcre_push_matches push_matches)
{
  TPcreExecParam P;
  int res;

  LpcreProcessExecParams (L, &P);
  res = pcre_exec(P.ud->pr, P.ptr_extra, P.text, (int)P.textlen, P.startoffset,
                  P.eflags, P.ud->match, (P.ud->ncapt + 1) * 3);

  if(P.callout_data.func_ref != LUA_NOREF) {
    luaL_unref(L, LUA_REGISTRYINDEX, P.callout_data.func_ref);
  }

  if (res >= 0) {
    lua_pushinteger(L, P.ud->match[0] + 1);
    lua_pushinteger(L, P.ud->match[1]);
    (*push_matches)(L, P.text, P.ud);
    lua_pushinteger(L, res);
    return 4;
  }
  lua_pushnil(L);
  lua_pushinteger(L, res);
  return 2;
}

#if PCRE_MAJOR >= 6
static int Lpcre_dfa_exec(lua_State *L)
{
  TPcreExecParam P;
  int res;
  int ovector[30];
  int wspace[40];

  LpcreProcessExecParams (L, &P);
  res = pcre_dfa_exec(P.ud->pr, P.ptr_extra, P.text, (int)P.textlen, P.startoffset,
                      P.eflags, ovector, DIM(ovector), wspace, DIM(wspace));

  if(P.callout_data.func_ref != LUA_NOREF) {
    luaL_unref(L, LUA_REGISTRYINDEX, P.callout_data.func_ref);
  }

  if (res >= 0 || res == PCRE_ERROR_PARTIAL) {
    int i;
    int max = (res>0) ? res : (res==0) ? (int)DIM(ovector)/2 : 1;
    lua_pushinteger(L, ovector[0] + 1);         /* 1-st return value */
    lua_newtable(L);                            /* 2-nd return value */
    for(i=0; i<max; i++) {
      lua_pushinteger(L, ovector[i+i+1]);
      lua_rawseti(L, -2, i+1);
    }
    lua_pushinteger(L, res);                    /* 3-rd return value */
    return 3;
  }
  lua_pushnil(L);
  lua_pushinteger(L, res);
  return 2;
}
#endif /* #if PCRE_MAJOR >= 6 */

static int Lpcre_match(lua_State *L)
{
  return Lpcre_match_generic(L, Lpcre_push_substrings);
}

static int Lpcre_exec(lua_State *L)
{
  return Lpcre_match_generic(L, Lpcre_push_offsets);
}

static int Lpcre_gc (lua_State *L)
{
  TPcre *ud = (TPcre *)luaL_checkudata(L, 1, pcre_handle);
  if (ud) {
    if(ud->pr)      pcre_free(ud->pr);
    if(ud->extra)   pcre_free(ud->extra);
    if(ud->tables)  pcre_free((void *)ud->tables);
    if(ud->match)   free(ud->match);
  }
  return 0;
}

static int Lpcre_tostring (lua_State *L) {
  return udata_tostring(L, pcre_handle, pcre_typename);
}

static int Lpcre_version (lua_State *L)
{
  lua_pushstring(L, pcre_version());
  return 1;
}

static flag_pair pcre_flags[] =
{
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
/*---------------------------------------------------------------------------*/
#if PCRE_MAJOR >= 4
  { "EXTRA_STUDY_DATA",              PCRE_EXTRA_STUDY_DATA },
  { "EXTRA_MATCH_LIMIT",             PCRE_EXTRA_MATCH_LIMIT },
  { "EXTRA_CALLOUT_DATA",            PCRE_EXTRA_CALLOUT_DATA },
#endif
#if PCRE_MAJOR >= 5
  { "EXTRA_TABLES",                  PCRE_EXTRA_TABLES },
#endif
/*---------------------------------------------------------------------------*/
  { NULL, 0 }
};

static int Lpcre_get_flags (lua_State *L) {
  return get_flags(L, pcre_flags);
}

static const luaL_reg pcremeta[] = {
  {"exec",       Lpcre_exec},
  {"match",      Lpcre_match},
  {"__gc",       Lpcre_gc},
  {"__tostring", Lpcre_tostring},
#if PCRE_MAJOR >= 6
  {"dfa_exec",   Lpcre_dfa_exec},
#endif
  {NULL, NULL}
};

/* Open the library */

static const luaL_reg rexlib[] = {
  {"new",         Lpcre_comp},
  {"newPCRE",     Lpcre_comp},          /* for backwards compatibility */
  {"flags",       Lpcre_get_flags},
  {"flagsPCRE",   Lpcre_get_flags},     /* for backwards compatibility */
  {"version",     Lpcre_version},
  {"versionPCRE", Lpcre_version},       /* for backwards compatibility */
  {NULL, NULL}
};

REX_API int REX_OPENLIB (lua_State *L)
{
  if (PCRE_MAJOR != atoi(pcre_version())) {
    return luaL_error(L, "%s requires version %d of PCRE library",
      REX_LIBNAME, (int)PCRE_MAJOR);
  }
  createmeta(L, pcre_handle);
  luaL_register(L, NULL, pcremeta);
  lua_pop(L, 1);
  pcre_callout = Lpcre_callout;
  luaL_register(L, REX_LIBNAME, rexlib);
  return 1;
}
