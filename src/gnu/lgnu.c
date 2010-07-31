/* lgnu.c - Lua binding of GNU regular expressions library */
/* See Copyright Notice in the file LICENSE */

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "lua.h"
#include "lauxlib.h"
#include "../common.h"

#define __USE_GNU
#ifndef REX_GNU_INCLUDE
#  include <regex.h>
#else
#  include REX_GNU_INCLUDE
#endif

/* These 2 settings may be redefined from the command-line or the makefile.
 * They should be kept in sync between themselves and with the target name.
 */
#ifndef REX_LIBNAME
#  define REX_LIBNAME "rex_gnu"
#endif
#ifndef REX_OPENLIB
#  define REX_OPENLIB luaopen_rex_gnu
#endif

#define REX_TYPENAME REX_LIBNAME"_regex"

#define ALG_CFLAGS_DFLT 0
#define ALG_EFLAGS_DFLT 0

#define ALG_GETCFLAGS(L,pos)  ALG_CFLAGS_DFLT

static void opttranslate (TArgComp *argC, lua_State *L, int pos);
#define ALG_OPTTRANSLATE(a,b,c)  opttranslate(a,b,c)

static void optsyntax (TArgComp *argC, lua_State *L, int pos);
#define ALG_OPTSYNTAX(a,b,c)  optsyntax(a,b,c)

#define ALG_NOMATCH(res)   ((res) == -1 || (res) == -2)
#define ALG_ISMATCH(res)   ((res) >= 0)
#define ALG_SUBBEG(ud,n)   ud->match.start[n]
#define ALG_SUBEND(ud,n)   ud->match.end[n]
#define ALG_SUBLEN(ud,n)   (ALG_SUBEND(ud,n) - ALG_SUBBEG(ud,n))
#define ALG_SUBVALID(ud,n) (ALG_SUBBEG(ud,n) >= 0)
#define ALG_NSUB(ud)     ((int)ud->r.re_nsub)

#define ALG_PUSHSUB(L,ud,text,n) \
  lua_pushlstring (L, (text) + ALG_SUBBEG(ud,n), ALG_SUBLEN(ud,n))

#define ALG_PUSHSUB_OR_FALSE(L,ud,text,n) \
  (ALG_SUBVALID(ud,n) ? ALG_PUSHSUB (L,ud,text,n) : lua_pushboolean (L,0))

#define ALG_PUSHSTART(L,ud,offs,n)   lua_pushinteger(L, (offs) + ALG_SUBBEG(ud,n) + 1)
#define ALG_PUSHEND(L,ud,offs,n)     lua_pushinteger(L, (offs) + ALG_SUBEND(ud,n))
#define ALG_PUSHOFFSETS(L,ud,offs,n) \
  (ALG_PUSHSTART(L,ud,offs,n), ALG_PUSHEND(L,ud,offs,n))

#define ALG_BASE(st)                  (st)

typedef struct {
  struct re_pattern_buffer r;
  struct re_registers      match;
  int                      freed;
  const char *             errmsg;
  int                      reverse;
} TGnu;

#define TUserdata TGnu

#include "../algo.h"

/*  Functions
 ******************************************************************************
 */

/* Execution flags, which we need to simulate as GNU does not use flags for this. */
#define GNU_NOTBOL  1
#define GNU_NOTEOL  2
#define GNU_REVERSE 4

static int generate_error  (lua_State *L, const TUserdata *ud, int errcode) {
  const char *errmsg;
  switch (errcode) {
  case 0:
    errmsg = ud->errmsg;
    break;
  case -1:
    errmsg = "no match";
    break;
  case -2:
    errmsg = "internal error in GNU regex";
    break;
  default:
    errmsg = "unknown error";
  }
  return luaL_error (L, "%s", errmsg);
}

#define ALG_TRANSLATE_SIZE (UCHAR_MAX + 1)
static void opttranslate (TArgComp *argC, lua_State *L, int pos) {
  if (!lua_isnoneornil (L, pos)) {
    unsigned i;

    argC->translate = (const unsigned char *) Lmalloc (L, ALG_TRANSLATE_SIZE);
    memset ((unsigned char *) argC->translate, 0, ALG_TRANSLATE_SIZE); /* initialize all members to 0 */
    for (i = 0; i <= UCHAR_MAX; i++) {
      lua_pushinteger (L, i);
      lua_gettable (L, pos);
      if (lua_tostring (L, -1))
        ((unsigned char *) argC->translate)[i] = *lua_tostring (L, -1);
      lua_pop (L, 1);
    }
  } else
    argC->translate = NULL;
}

typedef struct {
  const char * name;
  int value;
} EncPair;

/* ATTENTION:
   This array must always be kept alphabetically sorted, as it's used in the
   binary search, so take care when manually inserting new elements.
 */
static EncPair Syntaxes[] = {
  { "AWK",                    RE_SYNTAX_AWK },
  { "ED",                     RE_SYNTAX_ED },
  { "EGREP",                  RE_SYNTAX_EGREP },
  { "EMACS",                  RE_SYNTAX_EMACS },
  { "GNU_AWK",                RE_SYNTAX_GNU_AWK },
  { "GREP",                   RE_SYNTAX_GREP },
  { "POSIX_AWK",              RE_SYNTAX_POSIX_AWK },
  { "POSIX_BASIC",            RE_SYNTAX_POSIX_BASIC },
  { "POSIX_EGREP",            RE_SYNTAX_POSIX_EGREP },
  { "POSIX_EXTENDED",         RE_SYNTAX_POSIX_EXTENDED },
  { "POSIX_MINIMAL_BASIC",    RE_SYNTAX_POSIX_MINIMAL_BASIC },
  { "POSIX_MINIMAL_EXTENDED", RE_SYNTAX_POSIX_MINIMAL_EXTENDED },
  { "SED",                    RE_SYNTAX_SED },
};

static int fcmp (const void *p1, const void *p2) {
  return strcmp (((EncPair*) p1)->name, ((EncPair*) p2)->name);
}

static int getsyntax (lua_State *L, int pos) {
  EncPair key, *found;
  if ((key.name = luaL_optstring (L, pos, NULL)) == NULL)
    return -1;
  found = (EncPair*) bsearch (&key, Syntaxes, sizeof (Syntaxes) / sizeof (EncPair),
          sizeof (EncPair), fcmp);
  if (found == NULL)
    luaL_argerror (L, pos, "invalid or unsupported syntax string");
  return found->value;
}

static void optsyntax (TArgComp *argC, lua_State *L, int pos) {
  argC->gnusyn = getsyntax (L, pos);
}

static void seteflags (TGnu *ud, TArgExec *argE) {
  ud->r.not_bol = (argE->eflags & GNU_NOTBOL) != 0;
  ud->r.not_eol = (argE->eflags & GNU_NOTEOL) != 0;
  ud->reverse = (argE->eflags & GNU_REVERSE) != 0;
}

/*
   rex.setsyntax (syntax)
   @param syntax: one of the predefined strings listed in array 'Syntaxes'
   @return: nothing
*/
static int LGnu_setsyntax (lua_State *L) {
  (void) luaL_checkstring (L, 1);
  re_set_syntax (getsyntax (L, 1));
  return 0;
}

static int compile_regex (lua_State *L, const TArgComp *argC, TGnu **pud) {
  const char *res;
  TGnu *ud;
  reg_syntax_t old_syntax;
  int ret;

  ud = (TGnu *)lua_newuserdata (L, sizeof (TGnu));
  memset (ud, 0, sizeof (TGnu));          /* initialize all members to 0 */

  if (argC->gnusyn >= 0)
    old_syntax = re_set_syntax (argC->gnusyn);

  /* translate table is never written to, so this cast is safe */
  ud->r.translate = (unsigned char *) argC->translate;

  res = re_compile_pattern (argC->pattern, argC->patlen, &ud->r);
  if (res != NULL) {
      ud->errmsg = res;
      ret = generate_error (L, ud, 0);
  } else {
    if (argC->cflags & REG_NOSUB)
      ud->r.no_sub = 1;

    lua_pushvalue (L, LUA_ENVIRONINDEX);
    lua_setmetatable (L, -2);

    if (pud) *pud = ud;
    ret = 1;
  }

  if (argC->gnusyn >= 0)
    re_set_syntax (old_syntax);
  return ret;
}

static int gmatch_exec (TUserdata *ud, TArgExec *argE) {
  seteflags (ud, argE);
  if (argE->startoffset > 0)
    ud->r.not_bol = 1;
  argE->text += argE->startoffset;
  argE->textlen -= argE->startoffset;
  return re_search (&ud->r, argE->text, argE->textlen, 0, argE->textlen, &ud->match);
}

static void gmatch_pushsubject (lua_State *L, TArgExec *argE) {
  lua_pushlstring (L, argE->text, argE->textlen);
}

static int findmatch_exec (TGnu *ud, TArgExec *argE) {
  argE->text += argE->startoffset;
  argE->textlen -= argE->startoffset;
  seteflags (ud, argE);
  return re_search (&ud->r, argE->text, argE->textlen, 0, argE->textlen, &ud->match);
}

static int gsub_exec (TGnu *ud, TArgExec *argE, int st) {
  seteflags (ud, argE);
  if (st > 0)
    ud->r.not_bol = 1;
  return re_search (&ud->r, argE->text + st, argE->textlen - st, 0, argE->textlen - st, &ud->match);
}

static int split_exec (TGnu *ud, TArgExec *argE, int offset) {
  seteflags (ud, argE);
  if (offset > 0)
    ud->r.not_bol = 1;
  return re_search (&ud->r, argE->text + offset, argE->textlen - offset, 0, argE->textlen - offset, &ud->match);
}

static int Gnu_gc (lua_State *L) {
  TGnu *ud = check_ud (L);
  if (ud->freed == 0) {           /* precaution against "manual" __gc calling */
    ud->freed = 1;
    if (ud->r.regs_allocated != REGS_UNALLOCATED) {
      free (ud->match.start);
      free (ud->match.end);
    }
  }
  return 0;
}

static int Gnu_tostring (lua_State *L) {
  TGnu *ud = check_ud (L);
  if (ud->freed == 0)
    lua_pushfstring (L, "%s (%p)", REX_TYPENAME, (void*)ud);
  else
    lua_pushfstring (L, "%s (deleted)", REX_TYPENAME);
  return 1;
}

static flag_pair gnu_flags[] =
{
  { "not_bol", GNU_NOTBOL },
  { "not_eol", GNU_NOTEOL },
  { "reverse", GNU_REVERSE },
/*---------------------------------------------------------------------------*/
  { NULL, 0 }
};

static int Gnu_get_flags (lua_State *L) {
  const flag_pair* fps[] = { gnu_flags, NULL };
  return get_flags (L, fps);
}

static const luaL_reg gnumeta[] = {
  { "exec",       ud_exec },
  { "tfind",      ud_tfind },    /* old match */
  { "find",       ud_find },
  { "match",      ud_match },
  { "__gc",       Gnu_gc },
  { "__tostring", Gnu_tostring },
  { NULL, NULL}
};

static const luaL_reg rexlib[] = {
  { "match",      match },
  { "find",       find },
  { "gmatch",     gmatch },
  { "gsub",       gsub },
  { "split",      split },
  { "new",        ud_new },
  { "flags",      Gnu_get_flags },
  { "plainfind",  plainfind_func },
  { "setsyntax",  LGnu_setsyntax },
  { NULL, NULL }
};

/* Open the library */
REX_API int REX_OPENLIB (lua_State *L)
{
  re_set_syntax (RE_SYNTAX_POSIX_EXTENDED);

  /* create a new function environment to serve as a metatable for methods */
  lua_newtable (L);
  lua_pushvalue (L, -1);
  lua_replace (L, LUA_ENVIRONINDEX);
  lua_pushvalue(L, -1); /* mt.__index = mt */
  lua_setfield(L, -2, "__index");
  luaL_register (L, NULL, gnumeta);

  /* register functions */
  luaL_register (L, REX_LIBNAME, rexlib);
  lua_pushliteral (L, REX_VERSION" (for GNU regexes)");
  lua_setfield (L, -2, "_VERSION");
  return 1;
}
