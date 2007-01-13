/* lposix.c - Lua binding of POSIX regular expressions library */
/* See Copyright Notice in the file LICENSE */

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "lua.h"
#include "lauxlib.h"
#include "common.h"

#ifndef REX_POSIX_INCLUDE
#  include <regex.h>
#else
#  include REX_POSIX_INCLUDE
#endif

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

#define SUB_BEG(ud,n)   ud->match[n].rm_so
#define SUB_END(ud,n)   ud->match[n].rm_eo
#define SUB_LEN(ud,n)   (SUB_END(ud,n) - SUB_BEG(ud,n))
#define SUB_VALID(ud,n) (SUB_BEG(ud,n) >= 0)
#define NSUB(ud)        ((int)ud->r.re_nsub)

#define PUSH_SUB(L,ud,text,n) \
  lua_pushlstring (L, (text) + SUB_BEG(ud,n), SUB_LEN(ud,n))

#define PUSH_SUB_OR_FALSE(L,ud,text,n) \
  (SUB_VALID(ud,n) ? PUSH_SUB (L,ud,text,n) : lua_pushboolean (L,0))

#define PUSH_START(L,ud,offs,n)   lua_pushinteger(L, (offs) + SUB_BEG(ud,n) + 1)
#define PUSH_END(L,ud,offs,n)     lua_pushinteger(L, (offs) + SUB_END(ud,n))
#define PUSH_OFFSETS(L,ud,offs,n) (PUSH_START(L,ud,offs,n), PUSH_END(L,ud,offs,n))


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
  int          reptype;       /* used with gsub */
} TArgExec;

/*  Functions
 ******************************************************************************
 */

static TPosix* check_ud (lua_State *L, int stackpos) {
  return (TPosix *)luaL_checkudata (L, stackpos, posix_handle);
}

static void checkarg_new (lua_State *L, TArgComp *argC) {
  argC->pattern = luaL_checklstring (L, 1, &argC->patlen);
  argC->cflags = luaL_optint (L, 2, REG_EXTENDED);
}

/* function gsub (s, patt, f, [n], [cf], [ef]) */
static void checkarg_gsub (lua_State *L, TArgComp *argC, TArgExec *argE) {
  argE->text = luaL_checklstring (L, 1, &argE->textlen);
  argC->pattern = luaL_checklstring (L, 2, &argC->patlen);
  lua_tostring (L, 3);    /* converts number (if any) to string */
  argE->reptype = lua_type (L, 3);
  if (argE->reptype != LUA_TSTRING && argE->reptype != LUA_TTABLE &&
      argE->reptype != LUA_TFUNCTION) {
    luaL_argerror (L, 3, "must be string, table or function");
  }
  argE->funcpos = 3;
  argE->maxmatch = OptLimit (L, 4);
  argC->cflags = luaL_optint (L, 5, REG_EXTENDED);
  argE->eflags = luaL_optint (L, 6, EFLAGS_DEFAULT);
}

/* method r:tfind (s, [st], [ef]) */
/* method r:exec  (s, [st], [ef]) */
static void checkarg_tfind (lua_State *L, TArgExec *argE) {
  argE->ud = check_ud (L, 1);
  argE->text = luaL_checklstring (L, 2, &argE->textlen);
  argE->startoffset = get_startoffset (L, 3, argE->textlen);
  argE->eflags = luaL_optint (L, 4, EFLAGS_DEFAULT);
}

/* function find  (s, patt, [st], [cf], [ef]) */
/* function match (s, patt, [st], [cf], [ef]) */
static void checkarg_find_f (lua_State *L, TArgComp *argC, TArgExec *argE) {
  argE->text = luaL_checklstring (L, 1, &argE->textlen);
  argC->pattern = luaL_checklstring (L, 2, &argC->patlen);
  argE->startoffset = get_startoffset (L, 3, argE->textlen);
  argC->cflags = luaL_optint (L, 4, REG_EXTENDED);
  argE->eflags = luaL_optint (L, 5, 0);
}

/* function gmatch (s, patt, [cf], [ef]) */
/* function split  (s, patt, [cf], [ef]) */
static void checkarg_gmatch_split (lua_State *L, TArgComp *argC, TArgExec *argE) {
  argE->text = luaL_checklstring (L, 1, &argE->textlen);
  argC->pattern = luaL_checklstring (L, 2, &argC->patlen);
  argC->cflags = luaL_optint (L, 3, REG_EXTENDED);
  argE->eflags = luaL_optint (L, 4, EFLAGS_DEFAULT);
}

static int compile_regex (lua_State *L, const TArgComp *argC, TPosix **pud) {
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
    char *errbuf = (char *) lua_newuserdata (L, sz + 1);
    regerror (res, &ud->r, errbuf, sz);
    return luaL_error (L, "%s", errbuf);
  }

  if (argC->cflags & REG_NOSUB)
    ud->r.re_nsub = 0;
  ud->match = (regmatch_t *) Lmalloc (L, (NSUB(ud) + 1) * sizeof (regmatch_t));
  luaL_getmetatable (L, posix_handle);
  lua_setmetatable (L, -2);

  if (pud) *pud = ud;
  return 1;
}

static void push_substrings (lua_State *L, TPosix *ud, const char *text) {
  int i;
  CheckStack (L, NSUB(ud));
  for (i = 1; i <= NSUB(ud); i++) {
    PUSH_SUB_OR_FALSE (L, ud, text, i);
  }
}

static void push_substring_table (lua_State *L, TPosix *ud, const char *text) {
  int i;
  lua_newtable (L);
  for (i = 1; i <= NSUB(ud); i++) {
    PUSH_SUB_OR_FALSE (L, ud, text, i);
    lua_rawseti (L, -2, i);
  }
}

static void push_offset_table (lua_State *L, TPosix *ud, int startoffset) {
  int i, j;

  lua_newtable (L);
  for (i=1, j=1; i <= NSUB(ud); i++) {
    if (SUB_VALID (ud,i)) {
      PUSH_START (L, ud, startoffset, i);
      lua_rawseti (L, -2, j++);
      PUSH_END (L, ud, startoffset, i);
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
    SUB_BEG(argE->ud,0) = argE->startoffset;
    SUB_END(argE->ud,0) = argE->textlen;
    argE->startoffset = 0;
  }
  else
    argE->text += argE->startoffset;
#else
  argE->text += argE->startoffset;
#endif
}

static int generic_tfind (lua_State *L, int tfind) {
  int res;
  TArgExec argE;
  TPosix *ud;

  checkarg_tfind (L, &argE);
  CheckStartEnd (&argE);

  /* execute the search */
  ud = argE.ud; /* avoid too many redirections */
  res = regexec (&ud->r, argE.text, NSUB(ud) + 1, ud->match, argE.eflags);
  if (res == 0) {
    PUSH_OFFSETS (L, ud, argE.startoffset, 0);
    if (tfind)
      push_substring_table (L, ud, argE.text);
    else
      push_offset_table (L, ud, argE.startoffset);
    return 3;
  }
  lua_pushnil (L);
  lua_pushinteger (L, res);
  return 2;
}

static int Posix_tfind (lua_State *L) {
  return generic_tfind (L, 1);
}

static int Posix_exec (lua_State *L) {
  return generic_tfind (L, 0);
}

static int gmatch_iter (lua_State *L) {
  size_t textlen;
  int incr;
  TPosix *ud       = (TPosix*) lua_touserdata (L, lua_upvalueindex (1));
  const char *text = lua_tolstring (L, lua_upvalueindex (2), &textlen);
  int eflags       = lua_tointeger (L, lua_upvalueindex (3));
  int startoffset  = lua_tointeger (L, lua_upvalueindex (4));

  if (startoffset > (int)textlen)
    return 0;

#ifdef REX_POSIX_EXT
  if (eflags & REG_STARTEND) {
    SUB_BEG(ud,0) = 0;
    SUB_END(ud,0) = textlen - startoffset;
  }
#endif

  /* execute the search */
  text += startoffset;
  if (0 == regexec (&ud->r, text, NSUB(ud) + 1, ud->match, eflags)) {
    /* push either captures or entire match */
    if (NSUB(ud))
      push_substrings (L, ud, text);
    else
      PUSH_SUB (L, ud, text, 0);
    incr = (SUB_LEN(ud,0) == 0) ? 1 : 0;      /* prevent endless loop */
    PUSH_END (L, ud, startoffset + incr, 0);  /* update start offset */
    lua_replace (L, lua_upvalueindex (4));
    return NSUB(ud) ? NSUB(ud) : 1;
  }
  return 0;
}

static int split_iter (lua_State *L) {
  size_t textlen;
  int newoffset;
  TPosix *ud       = (TPosix*) lua_touserdata (L, lua_upvalueindex (1));
  const char *text = lua_tolstring (L, lua_upvalueindex (2), &textlen);
  int eflags       = lua_tointeger (L, lua_upvalueindex (3));
  int startoffset  = lua_tointeger (L, lua_upvalueindex (4));

  if (startoffset >= (int)textlen)
    return 0;
  for (newoffset = startoffset; newoffset < (int)textlen; ++newoffset) {
#ifdef REX_POSIX_EXT
    if (eflags & REG_STARTEND) {
      SUB_BEG(ud,0) = 0;
      SUB_END(ud,0) = textlen - newoffset;
    }
#endif

    /* execute the search */
    if (0 == regexec (&ud->r, text + newoffset, NSUB(ud) + 1, ud->match, eflags)) {
      if (SUB_LEN(ud,0)) {
        PUSH_END (L, ud, newoffset, 0);             /* update start offset */
        lua_replace (L, lua_upvalueindex (4));
        /* push text preceding the match */
        lua_pushlstring (L, text + startoffset, SUB_BEG(ud,0) + newoffset - startoffset);
        /* push either captures or entire match */
        if (NSUB(ud))
          push_substrings (L, ud, text + newoffset);
        else
          PUSH_SUB (L, ud, text + newoffset, 0);
        return NSUB(ud) ? NSUB(ud)+1 : 2;
      }
    }
    else
      break;
  }
  lua_pushinteger (L, textlen);             /* mark as last iteration */
  lua_replace (L, lua_upvalueindex (4));    /* update start offset */
  lua_pushlstring (L, text + startoffset, textlen - startoffset);
  return 1;
}

static int generic_gmatch (lua_State *L, lua_CFunction iter)
{
  TArgComp argC;
  TArgExec argE;
  checkarg_gmatch_split (L, &argC, &argE);
  compile_regex (L, &argC, &argE.ud);             /* 1-st upvalue: ud */
#ifdef REX_POSIX_EXT
  if (argE.eflags & REG_STARTEND)
    lua_pushlstring (L, argE.text, argE.textlen); /* 2-nd upvalue: s  */
  else
    lua_pushlstring (L, argE.text, strlen (argE.text));
#else
    lua_pushlstring (L, argE.text, strlen (argE.text));
#endif
  lua_pushinteger (L, argE.eflags);           /* 3-rd upvalue: ef */
  lua_pushinteger (L, 0);                     /* 4-th upvalue: startoffset */
  lua_pushcclosure (L, iter, 4);
  return 1;
}

static int Posix_gmatch (lua_State *L) {
  return generic_gmatch (L, gmatch_iter);
}

static int Posix_split (lua_State *L) {
  return generic_gmatch (L, split_iter);
}

static int Posix_new (lua_State *L) {
  TArgComp argC;
  checkarg_new (L, &argC);
  return compile_regex (L, &argC, NULL);
}

static int generic_find (lua_State *L, int find) {
  int res;
  TPosix *ud;
  TArgComp argC;
  TArgExec argE;

  checkarg_find_f (L, &argC, &argE);
  compile_regex (L, &argC, &ud);
  CheckStartEnd (&argE);

  res = regexec (&ud->r, argE.text, NSUB(ud) + 1, ud->match, argE.eflags);
  if (res == 0) {
    if (find)
      PUSH_OFFSETS (L, ud, argE.startoffset, 0);
    if (NSUB(ud))      /* push captures */
      push_substrings (L, ud, argE.text);
    else if (!find) {  /* push entire match */
      PUSH_SUB (L, ud, argE.text, 0);
      return 1;
    }
    return find ? NSUB(ud) + 2 : NSUB(ud);
  }
  lua_pushnil (L);
  lua_pushinteger (L, res);
  return 2;
}

static int Posix_find (lua_State *L) {
  return generic_find (L, 1);
}

static int Posix_match (lua_State *L) {
  return generic_find (L, 0);
}

static int Posix_gsub (lua_State *L) {
  TPosix *ud;
  TArgComp argC;
  TArgExec argE;
  int reps = 0, st = 0;
  TBuffer BufOut, BufRep;
  TFreeList freelist;
  /*--------------------------------------------------------------------------*/
  checkarg_gsub (L, &argC, &argE);
  compile_regex (L, &argC, &ud);
  argE.ud = ud;
  freelist_init (&freelist);
  /*--------------------------------------------------------------------------*/
  if (argE.reptype == LUA_TSTRING) {
    char dbuf[] = { 0, 0 };
    size_t replen;
    const char *p = lua_tolstring (L, argE.funcpos, &replen);
    const char *end = p + replen;
    buffer_init (&BufRep, 256, L, &freelist);
    while (p < end) {
      const char *q;
      for (q = p; q < end && *q != '%'; ++q)
        {}
      if (q != p)
        bufferZ_addlstring (&BufRep, p, q - p);
      if (q < end) {
        if (++q < end) {  /* skip % */
          if (isdigit (*q)) {
            int num;
            *dbuf = *q;
            num = atoi (dbuf);
            if (num == 1 && NSUB(ud) == 0)
              num = 0;
            else if (num > NSUB(ud)) {
              freelist_free (&freelist);
              return luaL_error (L, "invalid capture index");
            }
            bufferZ_addnum (&BufRep, num);
          }
          else bufferZ_addlstring (&BufRep, q, 1);
        }
        p = q + 1;
      }
      else break;
    }
  }
  else if (argE.reptype == LUA_TFUNCTION)
    lua_pushliteral (L, "break");
  /*--------------------------------------------------------------------------*/
  buffer_init (&BufOut, 1024, L, &freelist);
  while ((argE.maxmatch < 0 || reps < argE.maxmatch) && st <= (int)argE.textlen) {
    int from, to, res;
#ifdef REX_POSIX_EXT
    if(argE.eflags & REG_STARTEND) {
      SUB_BEG(ud,0) = 0;
      SUB_END(ud,0) = argE.textlen - st;
    }
#endif
    res = regexec (&ud->r, argE.text+st, NSUB(ud)+1, ud->match, argE.eflags);
    if (res != 0)
      break;
    ++reps;
    from = st + SUB_BEG(ud,0);
    to = st + SUB_END(ud,0);
    if (from > st)
      buffer_addlstring (&BufOut, argE.text + st, from - st);
    /*------------------------------------------------------------------------*/
    if (argE.reptype == LUA_TSTRING) {
      size_t iter = 0, num;
      const char *str;
      while (bufferZ_next (&BufRep, &iter, &num, &str)) {
        if (str == NULL) {  /* got number in variable 'num' */
          if (num == 0)         /* %0 : add the entire match    */
            buffer_addlstring (&BufOut, argE.text + from, to - from);
          else {                /* add captured substring */
            if (SUB_VALID (ud,num))
              buffer_addlstring (&BufOut, argE.text + st + SUB_BEG(ud,num),
                                 SUB_LEN(ud,num));
          }
        }
        else buffer_addlstring (&BufOut, str, num);
      }
    }
    /*------------------------------------------------------------------------*/
    else if (argE.reptype == LUA_TTABLE) {
      if (NSUB(ud) > 0)
        PUSH_SUB_OR_FALSE (L, ud, argE.text + st, 1);
      else
        lua_pushlstring (L, argE.text + from, to - from);
      lua_gettable (L, argE.funcpos);
    }
    /*------------------------------------------------------------------------*/
    else if (argE.reptype == LUA_TFUNCTION) {
      int narg;
      lua_pushvalue (L, argE.funcpos);
      if (NSUB(ud) > 0) {
        push_substrings (L, ud, argE.text + st);
        narg = NSUB(ud);
      }
      else {
        lua_pushlstring (L, argE.text + from, to - from);
        narg = 1;
      }
      if (0 != lua_pcall (L, narg, 2, 0)) {
        freelist_free (&freelist);
        luaL_error (L, lua_tostring (L, -1));
      }
    }
    /*------------------------------------------------------------------------*/
    if (argE.reptype != LUA_TSTRING) {
      int pos = (argE.reptype == LUA_TFUNCTION) ? -2 : -1;
      if (lua_tostring (L, pos))
        buffer_addvalue (&BufOut, pos);
      else if (!lua_toboolean (L, pos))
        buffer_addlstring (&BufOut, argE.text + from, to - from);
      else {
        freelist_free (&freelist);
        luaL_error (L, "invalid replacement value (a %s)", luaL_typename (L, pos));
      }
      if (argE.reptype == LUA_TFUNCTION && lua_equal (L, -1, -3))
        argE.maxmatch = 0;  /* signal break from the loop */
      lua_pop (L, -pos);
    }
    /*------------------------------------------------------------------------*/
    if (from < to)
      st = to;
    else if (st < (int)argE.textlen) {  /* advance by 1 char (not replaced) */
      buffer_addlstring (&BufOut, argE.text + st, 1);
      ++st;
    }
    else break;
  }
  /*--------------------------------------------------------------------------*/
  buffer_addlstring (&BufOut, argE.text + st, argE.textlen - st);
  buffer_pushresult (&BufOut);
  lua_pushinteger (L, reps);
  freelist_free (&freelist);
  return 2;
}

static int Posix_gc (lua_State *L) {
  TPosix *ud = check_ud (L, 1);
  if (ud->freed == 0) {           /* precaution against "manual" __gc calling */
    ud->freed = 1;
    regfree (&ud->r);
    if (ud->match)
      free (ud->match);
  }
  return 0;
}

static int Posix_tostring (lua_State *L) {
  TPosix *ud = check_ud (L, 1);
  if (ud->freed == 0)
    lua_pushfstring (L, "%s (%p)", posix_typename, (void*)ud);
  else
    lua_pushfstring (L, "%s (deleted)", posix_typename);
  return 1;
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

static int Posix_get_flags (lua_State *L) {
  return get_flags (L, posix_flags);
}

static const luaL_reg posixmeta[] = {
  { "exec",       Posix_exec },
  { "tfind",      Posix_tfind },    /* old match */
  { "__gc",       Posix_gc },
  { "__tostring", Posix_tostring },
  { NULL, NULL}
};

static const luaL_reg rexlib[] = {
  { "match",      Posix_match },
  { "find",       Posix_find },
  { "gmatch",     Posix_gmatch },
  { "gsub",       Posix_gsub },
  { "split",      Posix_split },
  { "new",        Posix_new },
  { "flags",      Posix_get_flags },
  { "plainfind",  plainfind_func },
  { NULL, NULL }
};

/* Open the library */
REX_API int REX_OPENLIB (lua_State *L)
{
  createmeta (L, posix_handle);
  luaL_register (L, NULL, posixmeta);
  lua_pop (L, 1);
  luaL_register (L, REX_LIBNAME, rexlib);
  lua_pushliteral (L, REX_VERSION" (for POSIX regexes)");
  lua_setfield (L, -2, "_VERSION");
  return 1;
}
