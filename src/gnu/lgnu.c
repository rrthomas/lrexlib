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

static int getcflags (lua_State *L, int pos);
#define ALG_GETCFLAGS(L,pos)  getcflags(L, pos)

#define ALG_CFLAGS_DFLT 0
#define ALG_EFLAGS_DFLT 0
/* FIXME: Need to be able to set the following fields, so treat eflags like cflags:
  unsigned __REPB_PREFIX(no_sub) : 1;
  unsigned __REPB_PREFIX(not_bol) : 1;
  unsigned __REPB_PREFIX(not_eol) : 1;
  unsigned __REPB_PREFIX(newline_anchor) : 1;
*/

#define ALG_NOMATCH        -1 /* FIXME: -2 for internal error is also possible; take arg like ALG_ISMATCH */
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
} TGnu;

#define TUserdata TGnu

#include "../algo.h"

/*  Functions
 ******************************************************************************
 */

static int getcflags (lua_State *L, int pos) {
  switch (lua_type (L, pos)) {
    case LUA_TNONE:
    case LUA_TNIL:
      return ALG_CFLAGS_DFLT;
    default:
      return luaL_typeerror (L, pos, "FIXME: translation tables not yet implemented");
  }
}

/* Differs from ..._POSIX_BASIC only in that RE_BK_PLUS_QM becomes
   RE_LIMITED_OPS, i.e., \? \+ \| are not recognized.  Actually, this
   isn't minimal, since other operators, such as \`, aren't disabled.  */
#define RE_SYNTAX_POSIX_MINIMAL_BASIC					\
  (_RE_SYNTAX_POSIX_COMMON | RE_LIMITED_OPS)

#define RE_SYNTAX_POSIX_EXTENDED					\
  (_RE_SYNTAX_POSIX_COMMON  | RE_CONTEXT_INDEP_ANCHORS			\
   | RE_CONTEXT_INDEP_OPS   | RE_NO_BK_BRACES				\
   | RE_NO_BK_PARENS        | RE_NO_BK_VBAR				\
   | RE_CONTEXT_INVALID_OPS | RE_UNMATCHED_RIGHT_PAREN_ORD)

/* Differs from ..._POSIX_EXTENDED in that RE_CONTEXT_INDEP_OPS is
   removed and RE_NO_BK_REFS is added.  */
#define RE_SYNTAX_POSIX_MINIMAL_EXTENDED				\
  (_RE_SYNTAX_POSIX_COMMON  | RE_CONTEXT_INDEP_ANCHORS			\
   | RE_CONTEXT_INVALID_OPS | RE_NO_BK_BRACES				\
   | RE_NO_BK_PARENS        | RE_NO_BK_REFS				\
   | RE_NO_BK_VBAR          | RE_UNMATCHED_RIGHT_PAREN_ORD)

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
    errmsg = "internal error in lrexlib";
  }
  return luaL_error (L, "%s", errmsg);
}

static int compile_regex (lua_State *L, const TArgComp *argC, TGnu **pud) {
  const char *res;
  TGnu *ud;
  reg_syntax_t old_syntax;
  int ret;

  ud = (TGnu *)lua_newuserdata (L, sizeof (TGnu));
  memset (ud, 0, sizeof (TGnu));          /* initialize all members to 0 */

  /* translate table is never written to, so this cast is safe */
  /* FIXME: ud->r.translate = (unsigned char *) translate; */

  old_syntax = re_set_syntax (RE_SYNTAX_EMACS); /* FIXME: take syntax parameter in cflags */
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

  re_set_syntax (old_syntax);
  return ret;
}

static int gmatch_exec (TUserdata *ud, TArgExec *argE) {
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
  return re_search (&ud->r, argE->text, argE->textlen, 0, argE->textlen, &ud->match);
}

static int gsub_exec (TGnu *ud, TArgExec *argE, int st) {
  if (st > 0)
    ud->r.not_bol = 1;
  return re_search (&ud->r, argE->text + st, argE->textlen - st, 0, argE->textlen - st, &ud->match);
}

static int split_exec (TGnu *ud, TArgExec *argE, int offset) {
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
  { "SYNTAX_EMACS",       RE_SYNTAX_EMACS },
  { "SYNTAX_AWK",         RE_SYNTAX_AWK },
  { "SYNTAX_GNU_AWK",     RE_SYNTAX_GNU_AWK },
  { "SYNTAX_POSIX_AWK",   RE_SYNTAX_POSIX_AWK },
  { "SYNTAX_GREP",        RE_SYNTAX_GREP },
  { "SYNTAX_EGREP",       RE_SYNTAX_EGREP },
  { "SYNTAX_POSIX_EGREP", RE_SYNTAX_POSIX_EGREP },
  { "SYNTAX_ED",          RE_SYNTAX_ED },
  { "SYNTAX_SED",         RE_SYNTAX_SED },
  { "SYNTAX_POSIX_BASIC", RE_SYNTAX_POSIX_BASIC },
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
  /* { "trfind",     ud_trfind }, */
  { "find",       ud_find },
  /* { "rfind",      ud_rfind }, */
  { "match",      ud_match },
  { "__gc",       Gnu_gc },
  { "__tostring", Gnu_tostring },
  { NULL, NULL}
};

static const luaL_reg rexlib[] = {
  { "match",      match },
  { "find",       find },
  /* { "rfind",      rfind }, */
  { "gmatch",     gmatch },
  { "gsub",       gsub },
  { "split",      split },
  { "new",        ud_new },
  { "flags",      Gnu_get_flags },
  { "plainfind",  plainfind_func },
  /* { "set_syntax", set_syntax }, */
  { NULL, NULL }
};

/* Open the library */
REX_API int REX_OPENLIB (lua_State *L)
{
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
