/* lposix.c - POSIX regular expression library */
/* POSIX regexs can use Spencer extensions for matching NULs if available */
/* (c) Reuben Thomas 2000-2006 */
/* (c) Shmuel Zeigerman 2004-2006 */

#include <stdlib.h>
#include <string.h>
#include <regex.h>

#include "lua.h"
#include "lauxlib.h"
#include "common.h"

/* These 2 settings may be redefined from the command-line or the makefile.
 * They should be kept in sync between themselves and with the target name.
 */
#ifndef REX_LIBNAME
  #define REX_LIBNAME "rex_posix"
#endif
#ifndef REX_OPENLIB
  #define REX_OPENLIB luaopen_rex_posix
#endif

/* Test if regex.h corresponds to the extended POSIX library, i.e. H.Spencer's.
   This test may not work as intended if regex.h introduced REG_BASIC, etc.
   via enum rather than #define.
   If that's the case, add -DREX_POSIX_EXT in the makefile/command line.
*/
#ifndef REX_POSIX_EXT
  #if defined(REG_BASIC) && defined(REG_STARTEND)
    #define REX_POSIX_EXT
  #endif
#endif

const char posix_handle[] = REX_LIBNAME"?regex_handle";
const char posix_typename[] = REX_LIBNAME"_regex";

typedef struct {
  regex_t r;
  regmatch_t *match;
} TPosix;

static int posix_comp (lua_State *L) {
  int res;
  TPosix *ud;
  size_t clen;
  const char *pattern = luaL_checklstring (L, 1, &clen);
  int cflags = luaL_optint (L, 2, REG_EXTENDED);

#ifdef REX_POSIX_EXT
  if ((cflags & REG_EXTENDED) && (cflags & REG_NOSPEC)) {
    return luaL_argerror (L, 2,
      "flags REG_EXTENDED and REG_NOSPEC must not be specified together");
  }
#endif

  ud = (TPosix *)lua_newuserdata (L, sizeof (TPosix));
  ud->match = NULL;

#ifdef REX_POSIX_EXT
  if (cflags & REG_PEND)
    ud->r.re_endp = pattern + clen;
#endif

  res = regcomp (&ud->r, pattern, cflags);
  if (res) {
    size_t sz = regerror (res, &ud->r, NULL, 0);
    char *errbuf = (char *) lua_newuserdata (L, sz+1);
    regerror (res, &ud->r, errbuf, sz);
    return luaL_error (L, "%s", errbuf);
  }

  ud->match =
    (regmatch_t *) Lmalloc (L, (ud->r.re_nsub + 1) * sizeof (regmatch_t));
  luaL_getmetatable (L, posix_handle);
  lua_setmetatable (L, -2);
  return 1;
}

static void posix_getargs
  (lua_State *L, TPosix **ud, const char **text, size_t *text_len)
{
  *ud = (TPosix *)luaL_checkudata (L, 1, posix_handle);
  luaL_argcheck (L, *ud != NULL, 1, "compiled regexp expected");
  *text = luaL_checklstring (L, 2, text_len);
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

static int posix_match_generic (lua_State *L, posix_push_matches push_matches)
{
  size_t elen;
  const char *text;
  TPosix *ud;
  int startoffset;
  int res;

#ifdef REX_POSIX_EXT
  int eflags = luaL_optint (L, 4, REG_STARTEND);
#else
  int eflags = luaL_optint (L, 4, 0);
#endif

  posix_getargs (L, &ud, &text, &elen);
  startoffset = get_startoffset (L, 3, elen);

#ifdef REX_POSIX_EXT
  if (eflags & REG_STARTEND) {
    ud->match[0].rm_so = startoffset;
    ud->match[0].rm_eo = elen;
    startoffset = 0;
  }
  else
    text += startoffset;
#else
  text += startoffset;
#endif

  /* execute the search */
  res = regexec (&ud->r, text, ud->r.re_nsub + 1, ud->match, eflags);
  if (res == 0) {
    lua_pushinteger (L, ud->match[0].rm_so + 1 + startoffset);
    lua_pushinteger (L, ud->match[0].rm_eo + startoffset);
    (*push_matches)(L, text, startoffset, ud->match, ud->r.re_nsub);
    lua_pushinteger (L, res);
    return 4;
  }
  lua_pushnil (L);
  lua_pushinteger (L, res);
  return 2;
}

static int posix_match (lua_State *L)
{
  return posix_match_generic (L, posix_push_substrings);
}

static int posix_exec (lua_State *L)
{
  return posix_match_generic (L, posix_push_offsets);
}

static int posix_gmatch_iter (lua_State *L)
{
  int res, i, incr;
  int eflags, startoffset;
  size_t len;
  TPosix *ud;
  const char *text;

  ud = (TPosix*) lua_touserdata (L, lua_upvalueindex (1));
  text = lua_tolstring (L, lua_upvalueindex (2), &len);
  len = lua_tointeger (L, lua_upvalueindex (3));
  eflags = lua_tointeger (L, lua_upvalueindex (4));
  startoffset = lua_tointeger (L, lua_upvalueindex (5));

#ifdef REX_POSIX_EXT
  if (eflags & REG_STARTEND) {
    ud->match[0].rm_so = 0;
    ud->match[0].rm_eo = len;
  }
#endif

  /* execute the search */
  text += startoffset;
  res = regexec (&ud->r, text, ud->r.re_nsub + 1, ud->match, eflags);
  if (res == 0) {
    /* push either captures or entire match */
    if (ud->r.re_nsub) {
      lua_settop (L, 0);
      if (lua_checkstack (L, ud->r.re_nsub) == 0)
        return luaL_error (L, "cannot add %d stack slots", ud->r.re_nsub);
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
    /* update length */
#ifdef REX_POSIX_EXT
    if (eflags & REG_STARTEND) {
      lua_pushinteger (L, len - ud->match[0].rm_eo - incr);
      lua_replace (L, lua_upvalueindex (3));
    }
#endif
    /* update start offset */
    lua_pushinteger (L, ud->match[0].rm_eo + incr);
    lua_replace (L, lua_upvalueindex (5));
    /* return */
    return ud->r.re_nsub ? ud->r.re_nsub : 1;
  }
  return 0;
}

static int posix_gmatch_method (lua_State *L)
{
  size_t textlen;
  TPosix *ud = (TPosix *)luaL_checkudata (L, 1, posix_handle); /* get 1st param: self */
  luaL_argcheck (L, ud != NULL, 1, "compiled regexp expected");

  (void) luaL_checklstring (L, 2, &textlen); /* get 2nd param: text */
  if (!lua_isnoneornil (L, 3)) {             /* get 3rd param: eflags */
    (void) luaL_checkint (L, 3);
    lua_settop (L, 3);
  }
  else {
    lua_settop (L, 2);
#ifdef REX_POSIX_EXT
    lua_pushinteger (L, REG_STARTEND);
#else
    lua_pushinteger (L, 0);
#endif
  }
  lua_pushinteger (L, textlen); /* 3rd upvalue */
  lua_insert (L, 3);
  lua_pushinteger (L, 0);       /* 5th upvalue: start offset */
  lua_pushcclosure (L, posix_gmatch_iter, 5);
  return 1;
}

/*
function gmatch (s, p, cf, ef)
  return new (p, cf):gmatch (s, ef)
end
*/
static int posix_gmatch_func (lua_State *L)
{
  lua_settop (L, 4);                           /* s, p, cf, ef */
  lua_insert (L, 2);                           /* s, ef, p, cf */
  lua_pushcfunction (L, posix_comp);           /* s, ef, p, cf, F */
  lua_insert (L, 3);                           /* s, ef, F, p, cf */
  lua_call (L, 2, 1);                          /* s, ef, r */
  lua_insert (L, 1);                           /* r, s, ef */
  lua_pushcfunction (L, posix_gmatch_method);  /* r, s, ef, F */
  lua_insert (L, 1);                           /* F, r, s, ef */
  lua_call (L, 3, 1);                          /* Iter */
  return 1;
}

static int posix_gc (lua_State *L) {
  TPosix *ud = (TPosix *)luaL_checkudata (L, 1, posix_handle);
  if (ud) {
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
  {"exec",       posix_exec},
  {"match",      posix_match},
  {"gmatch",     posix_gmatch_method},
  {"__gc",       posix_gc},
  {"__tostring", posix_tostring},
  {NULL, NULL}
};

/* Open the library */

static const luaL_reg rexlib[] = {
  {"new",         posix_comp},
  {"newPOSIX",    posix_comp},       /* for backwards compatibility */
  {"flags",       posix_get_flags},
  {"flagsPOSIX",  posix_get_flags},  /* for backwards compatibility */
  {"gmatch",      posix_gmatch_func},
  {NULL, NULL}
};

REX_API int REX_OPENLIB (lua_State *L)
{
  createmeta (L, posix_handle);
  luaL_register (L, NULL, posixmeta);
  lua_pop (L, 1);
  luaL_register (L, REX_LIBNAME, rexlib);
  return 1;
}
