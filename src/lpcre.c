/* lpcre.c - Lua binding of PCRE library */
/* See Copyright Notice in the file LICENSE */

#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <ctype.h>
#include <pcre.h>

#include "lua.h"
#include "lauxlib.h"
#include "common.h"
extern int Lpcre_get_flags (lua_State *L);
extern int Lpcre_config (lua_State *L);
extern flag_pair pcre_error_flags[];

/* These 2 settings may be redefined from the command-line or the makefile.
 * They should be kept in sync between themselves and with the target name.
 */
#ifndef REX_LIBNAME
#  define REX_LIBNAME "rex_pcre"
#endif
#ifndef REX_OPENLIB
#  define REX_OPENLIB luaopen_rex_pcre
#endif

#define CFLAGS_DEFAULT 0
#define EFLAGS_DEFAULT 0

#define CODE_NOMATCH    PCRE_ERROR_NOMATCH
#define IS_MATCH(res)   ((res) >= 0)
#define SUB_BEG(ud,n)   ud->match[n+n]
#define SUB_END(ud,n)   ud->match[n+n+1]
#define SUB_LEN(ud,n)   (SUB_END(ud,n) - SUB_BEG(ud,n))
#define SUB_VALID(ud,n) (SUB_BEG(ud,n) >= 0)
#define NSUB(ud)        ((int)ud->ncapt)

#define PUSH_SUB(L,ud,text,n) \
  lua_pushlstring (L, (text) + SUB_BEG(ud,n), SUB_LEN(ud,n))

#define PUSH_SUB_OR_FALSE(L,ud,text,n) \
  (SUB_VALID(ud,n) ? PUSH_SUB (L,ud,text,n) : lua_pushboolean (L,0))

#define PUSH_START(L,ud,offs,n)   lua_pushinteger(L, (offs) + SUB_BEG(ud,n) + 1)
#define PUSH_END(L,ud,offs,n)     lua_pushinteger(L, (offs) + SUB_END(ud,n))
#define PUSH_OFFSETS(L,ud,offs,n) (PUSH_START(L,ud,offs,n), PUSH_END(L,ud,offs,n))

#define BASE(st)               0
#define PULL(st,from)          (st = (from))

#if PCRE_MAJOR >= 4
#  define DO_NAMED_SUBPATTERNS 1
#endif


typedef struct {
  pcre       * pr;
  pcre_extra * extra;
  int        * match;
  int          ncapt;
  const unsigned char * tables;
  int          freed;
} TPcre;

#define TUserdata TPcre
#define USE_RETRY

const char pcre_typename[] = REX_LIBNAME"_regex";

#include "algo.h"

const unsigned char *DefaultTables;
const char tables_typename[] = "pcre_tables";
#define push_tables_env(L) (lua_pushinteger(L,1), lua_rawget(L,LUA_ENVIRONINDEX))

/*  Functions
 ******************************************************************************
 */

static int generate_error (lua_State *L, const TPcre *ud, int errcode) {
  const char *key = get_flag_key (pcre_error_flags, errcode);
  (void) ud;
  if (key)
    return luaL_error (L, "error PCRE_%s", key);
  else
    return luaL_error (L, "PCRE error code %d", errcode);
}

#if PCRE_MAJOR >= 6
/* method r:dfa_exec (s, [st], [ef], [ovecsize], [wscount]) */
static void checkarg_dfa_exec (lua_State *L, TArgExec *argE, TPcre **ud) {
  *ud = check_ud (L);
  argE->text = luaL_checklstring (L, 2, &argE->textlen);
  argE->startoffset = get_startoffset (L, 3, argE->textlen);
  argE->eflags = luaL_optint (L, 4, EFLAGS_DEFAULT);
  argE->ovecsize = luaL_optint (L, 5, 100);
  argE->wscount = luaL_optint (L, 6, 50);
}
#endif

static int Lpcre_maketables (lua_State *L) {
  *(const void**)lua_newuserdata (L, sizeof(void**)) = pcre_maketables();
  push_tables_env (L);
  lua_setmetatable (L, -2);
  return 1;
}

static void **check_tables (lua_State *L, int pos) {
  void **q;
  /* Compare the metatable against the C function environment. */
  if (lua_getmetatable(L, pos)) {
    push_tables_env (L);
    if (lua_rawequal(L, -1, -2) &&
        (q = (void **)lua_touserdata(L, pos)) != NULL) {
      lua_pop(L, 2);
      return q;
    }
  }
  luaL_argerror(L, pos, "not pcre_tables");
  return NULL;
}

/* function settables ([tables]) */
/*   Create tables for the current active locale and set them as the default
 *   tables, instead of the PCRE built-in tables.
 *   For proper clean-up, create a pcre_tables userdata, and put it into the
 *   function environment at a constant key.
 */
static int Lpcre_settables (lua_State *L)
{
  lua_settop (L, 1);
  if (!lua_toboolean (L, 1)) {
    DefaultTables = NULL;   /* use the built-in tables */
    lua_pushnil (L);
  }
  else {
    DefaultTables = *check_tables (L, 1);
  }
  /* get old value (to be returned) */
  push_tables_env (L);
  lua_pushlightuserdata (L, &DefaultTables);
  lua_rawget (L, -2);
  /* set new value */
  lua_pushlightuserdata (L, &DefaultTables);
  lua_pushvalue (L, 1);
  lua_rawset (L, -4);
  return 1;
}

static int tables_gc (lua_State *L) {
  void **ud = check_tables (L, 1);
  if (*ud) {
    pcre_free (*ud);
    *ud = NULL;
  }
  return 0;
}

static void OPTLOCALE (TArgComp *argC, lua_State *L, int pos) {
  argC->locale = argC->tables = NULL;
  if (!lua_isnoneornil (L, pos)) {
    if (lua_isstring (L, pos))
      argC->locale = lua_tostring (L, pos);
    else
      argC->tables = *check_tables (L, pos);
  }
}

static int make_tables (const char* locale, const unsigned char ** tables) {
  char old_locale[256];
  strcpy (old_locale, setlocale (LC_CTYPE, NULL)); /* store the locale */
  if (NULL == setlocale (LC_CTYPE, locale))        /* set new locale */
    return 0;
  *tables = pcre_maketables ();             /* make tables with new locale */
  setlocale (LC_CTYPE, old_locale);         /* restore the old locale */
  return 1;
}

static int compile_regex (lua_State *L, const TArgComp *argC, TPcre **pud) {
  const char *error;
  int erroffset;
  TPcre *ud;
  const unsigned char *tables = DefaultTables;

  ud = (TPcre*)lua_newuserdata (L, sizeof (TPcre));
  memset (ud, 0, sizeof (TPcre));           /* initialize all members to 0 */
  lua_pushvalue (L, LUA_ENVIRONINDEX);
  lua_setmetatable (L, -2);

  if (argC->locale) {
    if (!make_tables (argC->locale, &ud->tables))
      return luaL_error (L, "cannot set locale");
    tables = ud->tables;
  }
  else if (argC->tables)
    tables = argC->tables;

  ud->pr = pcre_compile (argC->pattern, argC->cflags, &error, &erroffset, tables);
  if (!ud->pr)
    return luaL_error (L, "%s (pattern offset: %d)", error, erroffset + 1);

  ud->extra = pcre_study (ud->pr, 0, &error);
  if (error) return luaL_error (L, "%s", error);

  pcre_fullinfo (ud->pr, ud->extra, PCRE_INFO_CAPTURECOUNT, &ud->ncapt);
  /* need (2 ints per capture, plus one for substring match) * 3/2 */
  ud->match = (int *) Lmalloc (L, (NSUB(ud) + 1) * 3 * sizeof (int));

  if (pud) *pud = ud;
  return 1;
}

#if PCRE_MAJOR >= 4
/* the target table must be on lua stack top */
static void do_named_subpatterns (lua_State *L, TPcre *ud, const char *text) {
  int i, namecount, name_entry_size;
  unsigned char *name_table, *tabptr;

  /* do named subpatterns - NJG */
  pcre_fullinfo (ud->pr, ud->extra, PCRE_INFO_NAMECOUNT, &namecount);
  if (namecount <= 0)
    return;
  pcre_fullinfo (ud->pr, ud->extra, PCRE_INFO_NAMETABLE, &name_table);
  pcre_fullinfo (ud->pr, ud->extra, PCRE_INFO_NAMEENTRYSIZE, &name_entry_size);
  tabptr = name_table;
  for (i = 0; i < namecount; i++) {
    int n = (tabptr[0] << 8) | tabptr[1]; /* number of the capturing parenthesis */
    if (n > 0 && n <= NSUB(ud)) {   /* check range */
      lua_pushstring (L, (char *)tabptr + 2); /* name of the capture, zero terminated */
      PUSH_SUB_OR_FALSE (L, ud, text, n);
      lua_rawset (L, -3);
    }
    tabptr += name_entry_size;
  }
}
#endif /* #if PCRE_MAJOR >= 4 */

static int tfind_exec (TPcre *ud, TArgExec *argE) {
  return pcre_exec (ud->pr, ud->extra, argE->text, (int)argE->textlen,
              argE->startoffset, argE->eflags, ud->match, (ud->ncapt + 1) * 3);
}

#if PCRE_MAJOR >= 6
static int Lpcre_dfa_exec (lua_State *L)
{
  TArgExec argE;
  TPcre *ud;
  int res;
  int *buf, *ovector, *wspace;

  checkarg_dfa_exec (L, &argE, &ud);
  buf = (int*) Lmalloc (L, (argE.ovecsize + argE.wscount) * sizeof(int));
  ovector = buf;
  wspace = buf + argE.ovecsize;

  res = pcre_dfa_exec (ud->pr, ud->extra, argE.text, (int)argE.textlen,
    argE.startoffset, argE.eflags, ovector, argE.ovecsize, wspace, argE.wscount);

  if (IS_MATCH (res) || res == PCRE_ERROR_PARTIAL) {
    int i;
    int max = (res>0) ? res : (res==0) ? (int)argE.ovecsize/2 : 1;
    lua_pushinteger (L, ovector[0] + 1);         /* 1-st return value */
    lua_newtable (L);                            /* 2-nd return value */
    for (i=0; i<max; i++) {
      lua_pushinteger (L, ovector[i+i+1]);
      lua_rawseti (L, -2, i+1);
    }
    lua_pushinteger (L, res);                    /* 3-rd return value */
    free (buf);
    return 3;
  }
  else {
    free (buf);
    if (res == CODE_NOMATCH)
      return lua_pushnil (L), 1;
    else
      return generate_error (L, ud, res);
  }
}
#endif /* #if PCRE_MAJOR >= 6 */

#ifdef USE_RETRY
  static int gmatch_exec (TUserdata *ud, TArgExec *argE, int retry) {
    int eflags = retry ? (argE->eflags|PCRE_NOTEMPTY|PCRE_ANCHORED) : argE->eflags;
    return pcre_exec (ud->pr, ud->extra, argE->text, argE->textlen,
      argE->startoffset, eflags, ud->match, (NSUB(ud) + 1) * 3);
  }
#else
  static int gmatch_exec (TUserdata *ud, TArgExec *argE) {
    return pcre_exec (ud->pr, ud->extra, argE->text, argE->textlen,
      argE->startoffset, argE->eflags, ud->match, (NSUB(ud) + 1) * 3);
  }
#endif

static void gmatch_pushsubject (lua_State *L, TArgExec *argE) {
  lua_pushlstring (L, argE->text, argE->textlen);
}

static int findmatch_exec (TPcre *ud, TArgExec *argE) {
  return pcre_exec (ud->pr, ud->extra, argE->text, argE->textlen,
    argE->startoffset, argE->eflags, ud->match, (NSUB(ud) + 1) * 3);
}

#ifdef USE_RETRY
  static int gsub_exec (TPcre *ud, TArgExec *argE, int st, int retry) {
    int eflags = retry ? (argE->eflags|PCRE_NOTEMPTY|PCRE_ANCHORED) : argE->eflags;
    return pcre_exec (ud->pr, ud->extra, argE->text, (int)argE->textlen,
      st, eflags, ud->match, (NSUB(ud) + 1) * 3);
  }
#else
  static int gsub_exec (TPcre *ud, TArgExec *argE, int st) {
    return pcre_exec (ud->pr, ud->extra, argE->text, (int)argE->textlen,
      st, argE->eflags, ud->match, (NSUB(ud) + 1) * 3);
  }
#endif

static int split_exec (TPcre *ud, TArgExec *argE, int offset) {
  return pcre_exec (ud->pr, ud->extra, argE->text, argE->textlen, offset,
                    argE->eflags, ud->match, (NSUB(ud) + 1) * 3);
}

static int Lpcre_gc (lua_State *L) {
  TPcre *ud = check_ud (L);
  if (ud->freed == 0) {           /* precaution against "manual" __gc calling */
    ud->freed = 1;
    if (ud->pr)      pcre_free (ud->pr);
    if (ud->extra)   pcre_free (ud->extra);
    if (ud->tables)  pcre_free ((void *)ud->tables);
    if (ud->match)   free (ud->match);
  }
  return 0;
}

static int Lpcre_tostring (lua_State *L) {
  TPcre *ud = check_ud (L);
  if (ud->freed == 0)
    lua_pushfstring (L, "%s (%p)", pcre_typename, (void*)ud);
  else
    lua_pushfstring (L, "%s (deleted)", pcre_typename);
  return 1;
}

static int tables_tostring (lua_State *L) {
  void **ud = check_tables (L, 1);
  lua_pushfstring (L, "%s (%p)", tables_typename, ud);
  return 1;
}

static int Lpcre_version (lua_State *L) {
  lua_pushstring (L, pcre_version ());
  return 1;
}

static const luaL_reg tablesmeta[] = {
  { "__gc",        tables_gc },
  { "__tostring",  tables_tostring },
  { NULL, NULL }
};

static const luaL_reg pcremeta[] = {
  { "exec",        ud_exec },
  { "tfind",       ud_tfind },    /* old match */
#if PCRE_MAJOR >= 6
  { "dfa_exec",    Lpcre_dfa_exec },
#endif
  { "__gc",        Lpcre_gc },
  { "__tostring",  Lpcre_tostring },
  { NULL, NULL }
};

static const luaL_reg rexlib[] = {
  { "match",       match },
  { "find",        find },
  { "gmatch",      gmatch },
  { "gsub",        gsub },
  { "split",       split },
  { "new",         ud_new },
  { "plainfind",   plainfind_func },
  { "flags",       Lpcre_get_flags },
  { "version",     Lpcre_version },
  { "settables",   Lpcre_settables },
  { "maketables",  Lpcre_maketables },
#if PCRE_MAJOR >= 4
  { "config",      Lpcre_config },
#endif
  { NULL, NULL }
};

/* Open the library */
REX_API int REX_OPENLIB (lua_State *L) {
  if (PCRE_MAJOR > atoi (pcre_version ())) {
    return luaL_error (L, "%s requires at least version %d of PCRE library",
      REX_LIBNAME, (int)PCRE_MAJOR);
  }
  /* create a new function environment to serve as a metatable for methods */
  lua_newtable (L);
  lua_pushvalue (L, -1);
  lua_replace (L, LUA_ENVIRONINDEX);
  lua_pushvalue(L, -1); /* mt.__index = mt */
  lua_setfield(L, -2, "__index");
  luaL_register (L, NULL, pcremeta);

  /* register functions */
  luaL_register (L, REX_LIBNAME, rexlib);
  lua_pushliteral (L, REX_VERSION" (for PCRE)");
  lua_setfield (L, -2, "_VERSION");

  /* register fenv[1] as a metatable for pcre_tables userdata */
  lua_pushinteger (L, 1);
  lua_newtable (L);
  lua_pushliteral (L, "access denied");
  lua_setfield (L, -2, "__metatable");
  luaL_register (L, NULL, tablesmeta);
  lua_rawset (L, LUA_ENVIRONINDEX);
  return 1;
}

