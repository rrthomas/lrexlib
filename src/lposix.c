/* lposix.c - POSIX regular expression library */
/* POSIX regexs can use Spencer extensions for matching NULs if available */
/* (c) Reuben Thomas 2000-2004 */
/* (c) Shmuel Zeigerman 2004-2005 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <regex.h>

#include "lua.h"
#include "lauxlib.h"

#include "common.h"

/* Lua version control */
#if defined(LUA_VERSION_NUM) && (LUA_VERSION_NUM >= 501)
  #define REX_REGISTER luaL_register
#else
  #ifdef COMPAT51
    #include "compat-5.1.c"
  #endif
  #define REX_REGISTER(a,b,c) luaL_openlib((a),(b),(c),0)
#endif

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

const char posix_handle[] = "posix_regex_handle";
const char posix_typename[] = "posix_regex";

typedef struct {
  regex_t r;
  regmatch_t *match;
} posix2;      /* a better name is needed */

static int posix_comp(lua_State *L) {
  int res;
  posix2 *p2;
  size_t clen;
  const char *pattern = luaL_checklstring(L, 1, &clen);
  int cflags = luaL_optint(L, 2, REG_EXTENDED);

#ifdef REX_POSIX_EXT
  if((cflags & REG_EXTENDED) && (cflags & REG_NOSPEC)) {
    L_lua_error(L,
      "flags REG_EXTENDED and REG_NOSPEC must not be specified together");
  }
#endif

  p2 = (posix2 *)lua_newuserdata(L, sizeof(posix2));
  p2->match = NULL;

#ifdef REX_POSIX_EXT
  if(cflags & REG_PEND)
    p2->r.re_endp = pattern + clen;
#endif

  res = regcomp(&p2->r, pattern, cflags);
  if (res) {
    size_t sz = regerror(res, &p2->r, NULL, 0);
    char *errbuf = (char *) Lmalloc(L, sz);
    regerror(res, &p2->r, errbuf, sz);
    lua_pushstring(L, errbuf);
    free(errbuf);
    lua_error(L);
  }

  p2->match =
    (regmatch_t *) Lmalloc(L, (p2->r.re_nsub + 1) * sizeof(regmatch_t));
  luaL_getmetatable(L, posix_handle);
  lua_setmetatable(L, -2);
  return 1;
}

static void posix_getargs
  (lua_State *L, posix2 **p2, const char **text, size_t *text_len)
{
  *p2 = (posix2 *)luaL_checkudata(L, 1, posix_handle);
  luaL_argcheck(L, *p2 != NULL, 1, "compiled regexp expected");
  *text = luaL_checklstring(L, 2, text_len);
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

  lua_newtable(L);
  for (i = 1; i <= ncapt; i++) {
    if (match[i].rm_so >= 0) {
      lua_pushlstring
        (L, text + match[i].rm_so, match[i].rm_eo - match[i].rm_so);
    }
    else
      lua_pushboolean(L, 0);
    lua_rawseti(L, -2, i);
  }
}

static void posix_push_offsets
  (lua_State *L, const char *text, int startoffset,
  regmatch_t *match, size_t ncapt)
{
  size_t i, j;
  (void) text; /* suppress compiler warning */

  lua_newtable(L);
  for (i=1, j=1; i <= ncapt; i++) {
    if (match[i].rm_so >= 0) {
      lua_pushnumber(L, startoffset + match[i].rm_so + 1);
      lua_rawseti(L, -2, j++);
      lua_pushnumber(L, startoffset + match[i].rm_eo);
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

static int posix_match_generic(lua_State *L, posix_push_matches push_matches)
{
  size_t elen;
  const char *text;
  posix2 *p2;
  int startoffset;
  int res;

#ifdef REX_POSIX_EXT
  int eflags = luaL_optint(L, 4, REG_STARTEND);
#else
  int eflags = luaL_optint(L, 4, 0);
#endif

  posix_getargs(L, &p2, &text, &elen);
  startoffset = get_startoffset(L, 3, elen);

#ifdef REX_POSIX_EXT
  if(eflags & REG_STARTEND) {
    p2->match[0].rm_so = startoffset;
    p2->match[0].rm_eo = elen;
    startoffset = 0;
  }
  else
    text += startoffset;
#else
  text += startoffset;
#endif

  /* execute the search */
  res = regexec(&p2->r, text, p2->r.re_nsub + 1, p2->match, eflags);
  if (res == 0) {
    lua_pushnumber(L, p2->match[0].rm_so + 1 + startoffset);
    lua_pushnumber(L, p2->match[0].rm_eo + startoffset);
    (*push_matches)(L, text, startoffset, p2->match, p2->r.re_nsub);
    lua_pushnumber(L, res);
    return 4;
  }
  lua_pushnil(L);
  lua_pushnumber(L, res);
  return 2;
}

static int posix_match(lua_State *L)
{
  return posix_match_generic(L, posix_push_substrings);
}

static int posix_exec(lua_State *L)
{
  return posix_match_generic(L, posix_push_offsets);
}

static int posix_gmatch(lua_State *L) {
  int res;
  size_t len;
  size_t nmatch = 0, limit = 0;
  const char *text;
  posix2 *p2;
  size_t maxmatch = (size_t)luaL_optnumber(L, 4, 0);

#ifdef REX_POSIX_EXT
  int eflags = luaL_optint(L, 5, REG_STARTEND);
#else
  int eflags = luaL_optint(L, 5, 0);
#endif

  posix_getargs(L, &p2, &text, &len);
  luaL_checktype(L, 3, LUA_TFUNCTION);

  if(maxmatch > 0) /* this must be stated in the docs */
    limit = 1;

  while (!limit || nmatch < maxmatch) {

#ifdef REX_POSIX_EXT
    if(eflags & REG_STARTEND) {
      p2->match[0].rm_so = 0;
      p2->match[0].rm_eo = len;
    }
#endif

    res = regexec(&p2->r, text, p2->r.re_nsub + 1, p2->match, eflags);
    if (res == 0) {
      nmatch++;
      lua_pushvalue(L, 3);
      lua_pushlstring(L, text + p2->match[0].rm_so,
                      p2->match[0].rm_eo - p2->match[0].rm_so);
      posix_push_substrings(L, text, 0, p2->match, p2->r.re_nsub);
      lua_call(L, 2, 1);
      if(lua_toboolean(L, -1))
        break;
      lua_pop(L, 1);
      text += p2->match[0].rm_eo;

#ifdef REX_POSIX_EXT
      if(eflags & REG_STARTEND)
        len -= p2->match[0].rm_eo;
#endif

    } else
      break;
  }
  lua_pushnumber(L, nmatch);
  return 1;
}

static int posix_gc (lua_State *L) {
  posix2 *p2 = (posix2 *)luaL_checkudata(L, 1, posix_handle);
  if (p2) {
    regfree(&p2->r);
    if(p2->match)
      free(p2->match);
  }
  return 0;
}

static int posix_tostring (lua_State *L) {
  return udata_tostring(L, posix_handle, posix_typename);
}

static flags_pair posix_flags[] =
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
  return get_flags(L, posix_flags);
}

static const luaL_reg posixmeta[] = {
  {"exec",       posix_exec},
  {"match",      posix_match},
  {"gmatch",     posix_gmatch},
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
  {NULL, NULL}
};

REX_LIB_API int REX_OPENLIB(lua_State *L)
{
  createmeta(L, posix_handle);
  REX_REGISTER(L, NULL, posixmeta);
  lua_pop(L, 1);
  REX_REGISTER(L, REX_LIBNAME, rexlib);
  return 1;
}
