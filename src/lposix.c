/* lposix.c - POSIX regular expression library */
/* POSIX regexs can use Spencer extensions for matching NULs if available */
/* (c) Reuben Thomas 2000-2006 */
/* (c) Shmuel Zeigerman 2004-2006 */

#include <stdlib.h>
#include <string.h>
#ifndef REX_POSIX_INCLUDE
#  include <regex.h>
#else
#  include REX_POSIX_INCLUDE
#endif

#include "lua.h"
#include "lauxlib.h"
#include "common.h"

/* These 2 settings may be redefined from the command-line or the makefile.
 * They should be kept in sync between themselves and with the target name.
 */
#ifndef REX_LIBNAME
#  define REX_LIBNAME "rex_posix"
#endif
#ifndef REX_OPENLIB
#  define REX_OPENLIB luaopen_rex_posix
#endif

/* Test if regex.h corresponds to the extended POSIX library, i.e. H.Spencer's.
   This test may not work as intended if regex.h introduced REG_BASIC, etc.
   via enum rather than #define.
   If that's the case, add -DREX_POSIX_EXT in the makefile/command line.
*/
#ifndef REX_POSIX_EXT
#  if defined(REG_BASIC) && defined(REG_STARTEND)
#    define REX_POSIX_EXT
#  endif
#endif

#ifdef REX_POSIX_EXT
#  define EFLAGS_DEFAULT REG_STARTEND
#else
#  define EFLAGS_DEFAULT 0
#endif

const char posix_typename[] = REX_LIBNAME"_regex";
const char *posix_handle = posix_typename;

/*  Data Types
 ******************************************************************************
 */

typedef struct {
  regex_t      r;
  regmatch_t * match;
  int          freed;
} TPosix;

typedef struct {            /* regcomp arguments */
  const char * pattern;
  size_t       patlen;
  int          cflags;
} TArgComp;

typedef struct {            /* regexec arguments */
  TPosix     * ud;
  const char * text;
  size_t       textlen;
  int          startoffset;
  int          eflags;
  int          funcpos;
  int          maxmatch;
} TArgExec;

/*  Functions
 ******************************************************************************
 */

static TPosix* CheckUD (lua_State *L, int stackpos) {
  return (TPosix *)luaL_checkudata (L, stackpos, posix_handle);
}

static void Checkarg_new (lua_State *L, TArgComp *argC) {
  argC->pattern = luaL_checklstring (L, 1, &argC->patlen);
  argC->cflags = luaL_optint (L, 2, REG_EXTENDED);
#ifdef REX_POSIX_EXT
  if ((argC->cflags & REG_EXTENDED) && (argC->cflags & REG_NOSPEC)) {
    luaL_argerror (L, 2,
      "flags REG_EXTENDED and REG_NOSPEC must not be specified together");
  }
#endif
}

/* function find (s, p, [st], [cf], [ef]) */
static void Checkarg_findmatch_func (lua_State *L, TArgComp *argC, TArgExec *argE) {
  argE->text = luaL_checklstring (L, 1, &argE->textlen);
  argC->pattern = luaL_checklstring (L, 2, &argC->patlen);
  argE->startoffset = get_startoffset (L, 3, argE->textlen);
  argC->cflags = luaL_optint (L, 4, REG_EXTENDED);
  argE->eflags = luaL_optint (L, 5, EFLAGS_DEFAULT);
}

/* method r:find (s, [st], [ef]) */
static void Checkarg_findmatch_method (lua_State *L, TArgExec *argE) {
  argE->ud = CheckUD (L, 1);
  argE->text = luaL_checklstring (L, 2, &argE->textlen);
  argE->startoffset = get_startoffset (L, 3, argE->textlen);
  argE->eflags = luaL_optint (L, 4, EFLAGS_DEFAULT);
}

/* method r:gmatch (s, [ef]) */
static void Checkarg_gmatch_method (lua_State *L, TArgExec *argE) {
  argE->ud = CheckUD (L, 1);
  argE->text = luaL_checklstring (L, 2, &argE->textlen);
  argE->eflags = luaL_optint (L, 3, EFLAGS_DEFAULT);
}

/* function gmatch (s, p, [cf], [lo], [ef]) */
static void Checkarg_gmatch_func (lua_State *L, TArgComp *argC, TArgExec *argE) {
  argE->text = luaL_checklstring (L, 1, &argE->textlen);
  argC->pattern = luaL_checklstring (L, 2, &argC->patlen);
  argC->cflags = luaL_optint (L, 3, REG_EXTENDED);
  argE->eflags = luaL_optint (L, 4, EFLAGS_DEFAULT);
}

/* method r:oldgmatch (s, f, [n], [ef]) */
static void Checkarg_oldgmatch_method (lua_State *L, TArgExec *argE) {
  argE->ud = CheckUD (L, 1);
  argE->text = luaL_checklstring (L, 2, &argE->textlen);
  argE->funcpos = CheckFunction (L, 3);
  argE->maxmatch = luaL_optint (L, 4, 0);
  argE->eflags = luaL_optint (L, 5, EFLAGS_DEFAULT);
}

static int posix_comp (lua_State *L, const TArgComp *argC, TPosix **pud) {
  int res;
  TPosix *ud;

  ud = (TPosix *)lua_newuserdata (L, sizeof (TPosix));
  memset (ud, 0, sizeof (TPosix));          /* initialize all members to 0 */

#ifdef REX_POSIX_EXT
  if (argC->cflags & REG_PEND)
    ud->r.re_endp = argC->pattern + argC->patlen;
#endif

  res = regcomp (&ud->r, argC->pattern, argC->cflags);
  if (res != 0) {
    size_t sz = regerror (res, &ud->r, NULL, 0);
    char *errbuf = (char *) lua_newuserdata (L, sz+1);
    regerror (res, &ud->r, errbuf, sz);
    return luaL_error (L, "%s", errbuf);
  }

  ud->match =
    (regmatch_t *) Lmalloc (L, (ud->r.re_nsub + 1) * sizeof (regmatch_t));
  luaL_getmetatable (L, posix_handle);
  lua_setmetatable (L, -2);

  if (pud) *pud = ud;
  return 1;
}

typedef void (*posix_push_matches)
  (lua_State *L, const char *text, int startoffset,
  regmatch_t *match, size_t ncapt);

static void posix_push_substrings
  (lua_State *L, const char *text, int startoffset,
  regmatch_t *match, size_t ncapt)
{
  size_t i;
  (void) startoffset; /* suppress compiler warning */

  lua_newtable (L);
  for (i = 1; i <= ncapt; i++) {
    if (match[i].rm_so >= 0) {
      lua_pushlstring
        (L, text + match[i].rm_so, match[i].rm_eo - match[i].rm_so);
    }
    else
      lua_pushboolean (L, 0);
    lua_rawseti (L, -2, i);
  }
}

static void posix_push_offsets
  (lua_State *L, const char *text, int startoffset,
  regmatch_t *match, size_t ncapt)
{
  size_t i, j;
  (void) text; /* suppress compiler warning */

  lua_newtable (L);
  for (i=1, j=1; i <= ncapt; i++) {
    if (match[i].rm_so >= 0) {
      lua_pushinteger (L, startoffset + match[i].rm_so + 1);
      lua_rawseti (L, -2, j++);
      lua_pushinteger (L, startoffset + match[i].rm_eo);
      lua_rawseti (L, -2, j++);
    }
    else {
      lua_pushboolean (L, 0);
      lua_rawseti (L, -2, j++);
      lua_pushboolean (L, 0);
      lua_rawseti (L, -2, j++);
    }
  }
}

static void CheckStartEnd (TArgExec *argE) {
#ifdef REX_POSIX_EXT
  if (argE->eflags & REG_STARTEND) {
    argE->ud->match[0].rm_so = argE->startoffset;
    argE->ud->match[0].rm_eo = argE->textlen;
    argE->startoffset = 0;
  }
  else
    argE->text += argE->startoffset;
#else
  argE->text += argE->startoffset;
#endif
}

static int posix_oldmatch_generic (lua_State *L, posix_push_matches push_matches)
{
  size_t elen;
  int res;
  TArgExec argE;
  TPosix *ud;

  Checkarg_findmatch_method (L, &argE);
  CheckStartEnd (&argE);

  /* execute the search */
  ud = argE.ud; /* avoiding too many redirections */
  res = regexec (&ud->r, argE.text, ud->r.re_nsub + 1, ud->match, argE.eflags);
  if (res == 0) {
    lua_pushinteger (L, ud->match[0].rm_so + 1 + argE.startoffset);
    lua_pushinteger (L, ud->match[0].rm_eo + argE.startoffset);
    (*push_matches)(L, argE.text, argE.startoffset, ud->match, ud->r.re_nsub);
    lua_pushinteger (L, res);
    return 4;
  }
  lua_pushnil (L);
  lua_pushinteger (L, res);
  return 2;
}

static int posix_oldmatch_method (lua_State *L)
{
  return posix_oldmatch_generic (L, posix_push_substrings);
}

static int posix_exec_method (lua_State *L)
{
  return posix_oldmatch_generic (L, posix_push_offsets);
}

static int posix_gmatch_iter (lua_State *L)
{
  TPosix *ud;
  const char *text;
  size_t textlen;
  int eflags, startoffset;
  int res, i, incr;

  ud = (TPosix*) lua_touserdata (L, lua_upvalueindex (1));
  text = lua_tolstring (L, lua_upvalueindex (2), &textlen);
  textlen = lua_tointeger (L, lua_upvalueindex (3));
  eflags = lua_tointeger (L, lua_upvalueindex (4));
  startoffset = lua_tointeger (L, lua_upvalueindex (5));

#ifdef REX_POSIX_EXT
  if (eflags & REG_STARTEND) {
    ud->match[0].rm_so = 0;
    ud->match[0].rm_eo = textlen;
  }
#endif

  /* execute the search */
  text += startoffset;
  res = regexec (&ud->r, text, ud->r.re_nsub + 1, ud->match, eflags);
  if (res == 0) {
    /* push either captures or entire match */
    if (ud->r.re_nsub) {
      CheckStack (L, ud->r.re_nsub);
      for (i = 1; i <= (int)ud->r.re_nsub; i++) {
        if (ud->match[i].rm_so >= 0)
          lua_pushlstring (L, text + ud->match[i].rm_so,
            ud->match[i].rm_eo - ud->match[i].rm_so);
        else
          lua_pushboolean (L, 0);
      }
    }
    else {
      lua_pushlstring (L, text + ud->match[0].rm_so,
        ud->match[0].rm_eo - ud->match[0].rm_so);
    }
    incr = (ud->match[0].rm_so == ud->match[0].rm_eo) ? 1 : 0; /* prevent endless loop */
#ifdef REX_POSIX_EXT
    /* update length */
    if (eflags & REG_STARTEND) {
      lua_pushinteger (L, textlen - ud->match[0].rm_eo - incr);
      lua_replace (L, lua_upvalueindex (3));
    }
#endif
    /* update start offset */
    lua_pushinteger (L, startoffset + ud->match[0].rm_eo + incr);
    lua_replace (L, lua_upvalueindex (5));
    /* return */
    return ud->r.re_nsub ? ud->r.re_nsub : 1;
  }
  return 0;
}

/* method r:gmatch (s, [ef]) */
static int posix_gmatch_method (lua_State *L) {
  TArgExec argE;
  Checkarg_gmatch_method (L, &argE);
  lua_pushvalue (L, 1);                          /* ud */
  lua_pushlstring (L, argE.text, argE.textlen);  /* s  */
  lua_pushinteger (L, argE.textlen);             /* textlen */
  lua_pushinteger (L, argE.eflags);              /* ef */
  lua_pushinteger (L, 0);                        /* start offset */
  lua_pushcclosure (L, posix_gmatch_iter, 5);
  return 1;
}

/* function gmatch (s, p, [cf], [ef]) */
static int posix_gmatch_func (lua_State *L) {
  TArgComp argC;
  TArgExec argE;
  Checkarg_gmatch_func (L, &argC, &argE);
  posix_comp (L, &argC, NULL);                   /* ud */
  lua_pushlstring (L, argE.text, argE.textlen);  /* s */
  lua_pushinteger (L, argE.textlen);             /* textlen */
  lua_pushinteger (L, argE.eflags);              /* ef */
  lua_pushinteger (L, 0);                        /* start offset */
  lua_pushcclosure (L, posix_gmatch_iter, 5);
  return 1;
}

static int posix_new (lua_State *L) {
  TArgComp argC;
  Checkarg_new (L, &argC);
  return posix_comp (L, &argC, NULL);
}

static int posix_find_generic (lua_State *L, TArgExec *argE, int find) {
  int i, res;
  TPosix *ud = argE->ud;

  CheckStartEnd (argE);
  res = regexec (&ud->r, argE->text, ud->r.re_nsub + 1, ud->match, argE->eflags);
  if (res == 0) {
    if (find) {
      lua_pushinteger (L, ud->match[0].rm_so + 1 + argE->startoffset);
      lua_pushinteger (L, ud->match[0].rm_eo + argE->startoffset);
    }
    if (ud->r.re_nsub) {  /* push captures */
      CheckStack (L, ud->r.re_nsub);
      for (i = 1; i <= (int)ud->r.re_nsub; i++) {
        if (ud->match[i].rm_so >= 0) {
          lua_pushlstring (L, argE->text + ud->match[i].rm_so,
            ud->match[i].rm_eo - ud->match[i].rm_so);
        }
        else
          lua_pushboolean (L, 0);
      }
    }
    else if (!find) {  /* push entire match */
      lua_pushlstring (L, argE->text + ud->match[0].rm_so,
        ud->match[0].rm_eo - ud->match[0].rm_so);
      return 1;
    }
    return find ? ud->r.re_nsub + 2 : ud->r.re_nsub;
  }
  lua_pushnil (L);
  lua_pushinteger (L, res);
  return 2;
}

static int posix_findmatch_method (lua_State *L, int find) {
  TArgExec argE;
  Checkarg_findmatch_method (L, &argE);
  return posix_find_generic (L, &argE, find);
}

static int posix_find_method (lua_State *L) {
  return posix_findmatch_method (L, 1);
}

static int posix_match_method (lua_State *L) {
  return posix_findmatch_method (L, 0);
}

static int posix_findmatch_func (lua_State *L, int find) {
  TArgComp argC;
  TArgExec argE;
  Checkarg_findmatch_func (L, &argC, &argE);
  posix_comp (L, &argC, &argE.ud);
  return posix_find_generic (L, &argE, find);
}

static int posix_find_func (lua_State *L) {
  return posix_findmatch_func (L, 1);
}

static int posix_match_func (lua_State *L) {
  return posix_findmatch_func (L, 0);
}

static int posix_oldgmatch_method (lua_State *L) {
  int res, nmatch=0, limit=0;
  TArgExec argE;
  TPosix *ud;

  Checkarg_oldgmatch_method (L, &argE);
  ud = argE.ud;

  if (argE.maxmatch > 0) /* this must be stated in the docs */
    limit = 1;

  while (!limit || nmatch < argE.maxmatch) {

#ifdef REX_POSIX_EXT
    if(argE.eflags & REG_STARTEND) {
      ud->match[0].rm_so = 0;
      ud->match[0].rm_eo = argE.textlen;
    }
#endif

    res = regexec(&ud->r, argE.text, ud->r.re_nsub + 1, ud->match, argE.eflags);
    if (res == 0) {
      nmatch++;
      lua_pushvalue(L, argE.funcpos);
      lua_pushlstring(L, argE.text + ud->match[0].rm_so,
                      ud->match[0].rm_eo - ud->match[0].rm_so);
      posix_push_substrings(L, argE.text, 0, ud->match, ud->r.re_nsub);
      lua_call(L, 2, 1);
      if(lua_toboolean(L, -1))
        break;
      lua_pop(L, 1);
      argE.text += ud->match[0].rm_eo;

#ifdef REX_POSIX_EXT
      if(argE.eflags & REG_STARTEND)
        argE.textlen -= ud->match[0].rm_eo;
#endif

    } else
      break;
  }
  lua_pushinteger(L, nmatch);
  return 1;
}

static int posix_gc (lua_State *L) {
  TPosix *ud = CheckUD (L, 1);
  if (ud->freed == 0) {           /* precaution against "manual" __gc calling */
    ud->freed = 1;
    regfree (&ud->r);
    if (ud->match)
      free (ud->match);
  }
  return 0;
}

static int posix_tostring (lua_State *L) {
  return udata_tostring (L, posix_handle, posix_typename);
}

static flag_pair posix_flags[] =
{
#ifdef REX_POSIX_EXT
  { "BASIC",    REG_BASIC },
  { "NOSPEC",   REG_NOSPEC },
  { "PEND",     REG_PEND },
  { "STARTEND", REG_STARTEND },
#endif
  { "EXTENDED", REG_EXTENDED },
  { "ICASE",    REG_ICASE },
  { "NOSUB",    REG_NOSUB },
  { "NEWLINE",  REG_NEWLINE },
  { "NOTBOL",   REG_NOTBOL },
  { "NOTEOL",   REG_NOTEOL },
/*---------------------------------------------------------------------------*/
  { "NOMATCH",  REG_NOMATCH },
  { "BADPAT",   REG_BADPAT },
  { "ECOLLATE", REG_ECOLLATE },
  { "ECTYPE",   REG_ECTYPE },
  { "EESCAPE",  REG_EESCAPE },
  { "ESUBREG",  REG_ESUBREG },
  { "EBRACK",   REG_EBRACK },
  { "EPAREN",   REG_EPAREN },
  { "EBRACE",   REG_EBRACE },
  { "BADBR",    REG_BADBR },
  { "ERANGE",   REG_ERANGE },
  { "ESPACE",   REG_ESPACE },
  { "BADRPT",   REG_BADRPT },
#ifdef REX_POSIX_EXT
  { "EMPTY",    REG_EMPTY },
  { "ASSERT",   REG_ASSERT },
  { "INVARG",   REG_INVARG },
#endif
/*---------------------------------------------------------------------------*/
  { NULL, 0 }
};

static int posix_get_flags (lua_State *L) {
  return get_flags (L, posix_flags);
}

static const luaL_reg posixmeta[] = {
  { "exec",       posix_exec_method },
  { "oldmatch",   posix_oldmatch_method },
  { "oldgmatch",  posix_oldgmatch_method },
  { "gmatch",     posix_gmatch_method },
  { "find",       posix_find_method },
  { "match",      posix_match_method },
  { "__gc",       posix_gc },
  { "__tostring", posix_tostring },
  { NULL, NULL}
};

static const luaL_reg rexlib[] = {
  { "new",         posix_new },
  { "gmatch",      posix_gmatch_func },
  { "match",       posix_match_func },
  { "find",        posix_find_func },
  { "plainfind",   plainfind_func },
  { "flags",       posix_get_flags },
  { NULL, NULL }
};

/* Open the library */
REX_API int REX_OPENLIB (lua_State *L)
{
  createmeta (L, posix_handle);
  luaL_register (L, NULL, posixmeta);
  lua_pop (L, 1);
  luaL_register (L, REX_LIBNAME, rexlib);
  lua_pushliteral (L, "Lrexlib 2.0 beta (for POSIX regexes");
  lua_setfield (L, -2, "_VERSION");
  return 1;
}
