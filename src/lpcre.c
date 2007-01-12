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

/* These 2 settings may be redefined from the command-line or the makefile.
 * They should be kept in sync between themselves and with the target name.
 */
#ifndef REX_LIBNAME
#  define REX_LIBNAME "rex_pcre"
#endif
#ifndef REX_OPENLIB
#  define REX_OPENLIB luaopen_rex_pcre
#endif

const char pcre_typename[] = REX_LIBNAME"_regex";
const char *pcre_handle = pcre_typename;

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

/*  Data Types
 ******************************************************************************
 */

typedef struct {
  pcre       * pr;
  pcre_extra * extra;
  int        * match;
  int          ncapt;
  const unsigned char * tables;
  int          freed;
} TPcre;

typedef struct {            /* pcre_compile arguments */
  const char * pattern;
  size_t       patlen;
  int          cflags;
  const char * locale;
} TArgComp;

typedef struct {            /* pcre_exec arguments */
  TPcre      * ud;
  const char * text;
  size_t       textlen;
  int          startoffset;
  int          eflags;
  int          funcpos;
  int          maxmatch;
  int          reptype;       /* used with gsub */
  size_t       ovecsize;      /* used with dfa_exec */
  size_t       wscount;       /* used with dfa_exec */
} TArgExec;

/*  Functions
 ******************************************************************************
 */

static TPcre* check_ud (lua_State *L, int stackpos) {
  return (TPcre *)luaL_checkudata (L, stackpos, pcre_handle);
}

static void checkarg_new (lua_State *L, TArgComp *argC) {
  argC->pattern = luaL_checkstring (L, 1);
  argC->cflags = luaL_optint (L, 2, 0);
  argC->locale = luaL_optstring (L, 3, NULL);
}

/* function gsub (s, patt, f, [n], [cf], [ef], [lo]) */
static void checkarg_gsub (lua_State *L, TArgComp *argC, TArgExec *argE) {
  argE->text = luaL_checklstring (L, 1, &argE->textlen);
  argC->pattern = luaL_checkstring (L, 2);
  lua_tostring (L, 3);    /* converts number (if any) to string */
  argE->reptype = lua_type (L, 3);
  if (argE->reptype != LUA_TSTRING && argE->reptype != LUA_TTABLE &&
      argE->reptype != LUA_TFUNCTION) {
    luaL_argerror (L, 3, "must be string, table or function");
  }
  argE->funcpos = 3;
  argE->maxmatch = OptLimit (L, 4);
  argC->cflags = luaL_optint (L, 5, 0);
  argE->eflags = luaL_optint (L, 6, 0);
  argC->locale = luaL_optstring (L, 7, NULL);
}

/* method r:tfind (s, [st], [ef]) */
/* method r:exec  (s, [st], [ef]) */
static void checkarg_tfind (lua_State *L, TArgExec *argE) {
  argE->ud = check_ud (L, 1);
  argE->text = luaL_checklstring (L, 2, &argE->textlen);
  argE->startoffset = get_startoffset (L, 3, argE->textlen);
  argE->eflags = luaL_optint (L, 4, 0);
}

/* function find  (s, patt, [st], [cf], [ef], [lo]) */
/* function match (s, patt, [st], [cf], [ef], [lo]) */
static void checkarg_find_f (lua_State *L, TArgComp *argC, TArgExec *argE) {
  argE->text = luaL_checklstring (L, 1, &argE->textlen);
  argC->pattern = luaL_checkstring (L, 2);
  argE->startoffset = get_startoffset (L, 3, argE->textlen);
  argC->cflags = luaL_optint (L, 4, 0);
  argE->eflags = luaL_optint (L, 5, 0);
  argC->locale = luaL_optstring (L, 6, NULL);
}

/* function gmatch (s, patt, [cf], [ef], [lo]) */
/* function split  (s, patt, [cf], [ef], [lo]) */
static void checkarg_gmatch_split (lua_State *L, TArgComp *argC, TArgExec *argE) {
  argE->text = luaL_checklstring (L, 1, &argE->textlen);
  argC->pattern = luaL_checkstring (L, 2);
  argC->cflags = luaL_optint (L, 3, 0);
  argE->eflags = luaL_optint (L, 4, 0);
  argC->locale = luaL_optstring (L, 5, NULL);
}

#if PCRE_MAJOR >= 6
/* method r:dfa_exec (s, [st], [ef], [ovecsize], [wscount]) */
static void checkarg_dfa_exec (lua_State *L, TArgExec *argE) {
  argE->ud = check_ud (L, 1);
  argE->text = luaL_checklstring (L, 2, &argE->textlen);
  argE->startoffset = get_startoffset (L, 3, argE->textlen);
  argE->eflags = luaL_optint (L, 4, 0);
  argE->ovecsize = luaL_optint (L, 5, 100);
  argE->wscount = luaL_optint (L, 6, 50);
}
#endif

static int make_tables (const char* locale, const unsigned char ** tables) {
  char old_locale[256];
  strcpy (old_locale, setlocale (LC_CTYPE, NULL)); /* store the locale */
  if (NULL == setlocale (LC_CTYPE, locale))        /* set new locale */
    return 1;
  *tables = pcre_maketables ();             /* make tables with new locale */
  setlocale (LC_CTYPE, old_locale);         /* restore the old locale */
  return 0;
}

static int compile_regex (lua_State *L, const TArgComp *argC, TPcre **pud) {
  const char *error;
  int erroffset;
  TPcre *ud;

  ud = (TPcre*)lua_newuserdata (L, sizeof (TPcre));
  memset (ud, 0, sizeof (TPcre));           /* initialize all members to 0 */
  luaL_getmetatable (L, pcre_handle);
  lua_setmetatable (L, -2);

  if (argC->locale) {
    if (make_tables (argC->locale, &ud->tables) != 0)
      return luaL_error (L, "cannot set locale");
  }

  ud->pr = pcre_compile (argC->pattern, argC->cflags, &error, &erroffset, ud->tables);
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

static int Lpcre_new (lua_State *L) {
  TArgComp argC;
  checkarg_new (L, &argC);
  return compile_regex (L, &argC, NULL);
}

static void push_substrings (lua_State *L, TPcre *ud, const char *text) {
  int i;
  CheckStack (L, NSUB(ud));
  for (i = 1; i <= NSUB(ud); i++) {
    PUSH_SUB_OR_FALSE (L, ud, text, i);
  }
}

static void push_substring_table (lua_State *L, TPcre *ud, const char *text) {
  int i;
  lua_newtable (L);
  for (i = 1; i <= NSUB(ud); i++) {
    PUSH_SUB_OR_FALSE (L, ud, text, i);
    lua_rawseti (L, -2, i);
  }
}

static void push_offset_table (lua_State *L, const int *offsets, int nmax) {
  int i, j, k;
  lua_newtable (L);
  for (i=1, j=1; i <= nmax; i++) {
    k = i * 2;
    if (offsets[k] >= 0) {
      lua_pushinteger (L, offsets[k] + 1);
      lua_rawseti (L, -2, j++);
      lua_pushinteger (L, offsets[k+1]);
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
      lua_pushstring (L, tabptr + 2); /* name of the capture, zero terminated */
      PUSH_SUB_OR_FALSE (L, ud, text, n);
      lua_rawset (L, -3);
    }
    tabptr += name_entry_size;
  }
}

static int generic_tfind (lua_State *L, int tfind) {
  TPcre *ud;
  TArgExec argE;
  int res;

  checkarg_tfind (L, &argE);
  ud = argE.ud;
  res = pcre_exec (ud->pr, ud->extra, argE.text, (int)argE.textlen,
                   argE.startoffset, argE.eflags, ud->match,
                   (ud->ncapt + 1) * 3);
  if (res >= 0) {
    PUSH_OFFSETS (L, ud, 0, 0);
    if (tfind)
      push_substring_table (L, ud, argE.text);
    else
      push_offset_table (L, ud->match, ud->ncapt);
    do_named_subpatterns (L, ud, argE.text);
    return 3;
  }
  lua_pushnil (L);
  lua_pushinteger (L, res);
  return 2;
}

#if PCRE_MAJOR >= 6
static int Lpcre_dfa_exec (lua_State *L)
{
  TArgExec argE;
  int res;
  int *buf, *ovector, *wspace;

  checkarg_dfa_exec (L, &argE);
  buf = (int*) Lmalloc (L, (argE.ovecsize + argE.wscount) * sizeof(int));
  ovector = buf;
  wspace = buf + argE.ovecsize;

  res = pcre_dfa_exec (argE.ud->pr, argE.ud->extra, argE.text, (int)argE.textlen,
    argE.startoffset, argE.eflags, ovector, argE.ovecsize, wspace, argE.wscount);

  if (res >= 0 || res == PCRE_ERROR_PARTIAL) {
    int i;
    int max = (res>0) ? res : (res==0) ? (int)argE.ovecsize/2 : 1;
    lua_pushinteger (L, ovector[0] + 1);         /* 1-st return value */
    lua_newtable (L);                            /* 2-nd return value */
    for (i=0; i<max; i++) {
      lua_pushinteger (L, ovector[i+i+1]);
      lua_rawseti (L, -2, i+1);
    }
    lua_pushinteger (L, res);                    /* 3-rd return value */
    res = 3;
  }
  else {
    lua_pushnil (L);
    lua_pushinteger (L, res);
    res = 2;
  }
  free (buf);
  return res;
}
#endif /* #if PCRE_MAJOR >= 6 */

static int Lpcre_tfind (lua_State *L) {
  return generic_tfind (L, 1);
}

static int Lpcre_exec (lua_State *L) {
  return generic_tfind (L, 0);
}

static int gmatch_iter (lua_State *L) {
  size_t textlen;
  TPcre *ud        = (TPcre*) lua_touserdata (L, lua_upvalueindex (1));
  const char *text = lua_tolstring (L, lua_upvalueindex (2), &textlen);
  int eflags       = lua_tointeger (L, lua_upvalueindex (3));
  int startoffset  = lua_tointeger (L, lua_upvalueindex (4));

  if (startoffset > (int)textlen)
    return 0;
  if (0 <= pcre_exec (ud->pr, ud->extra, text, textlen, startoffset, eflags,
                      ud->match, (NSUB(ud) + 1) * 3)) {
    int incr = (SUB_LEN(ud,0) == 0) ? 1 : 0;  /* prevent endless loop */
    PUSH_END (L, ud, incr, 0);                /* update start offset */
    lua_replace (L, lua_upvalueindex (4));
    /* push either captures or entire match */
    if (NSUB(ud)) {
      push_substrings (L, ud, text);
      return NSUB(ud);
    }
    else {
      PUSH_SUB (L, ud, text, 0);
      return 1;
    }
  }
  return 0;
}

static int split_iter (lua_State *L) {
  size_t textlen;
  TPcre *ud        = (TPcre*) lua_touserdata (L, lua_upvalueindex (1));
  const char *text = lua_tolstring (L, lua_upvalueindex (2), &textlen);
  int eflags       = lua_tointeger (L, lua_upvalueindex (3));
  int startoffset  = lua_tointeger (L, lua_upvalueindex (4));
  int newoffset;

  if (startoffset >= (int)textlen)
    return 0;
  for (newoffset = startoffset; newoffset < (int)textlen; ++newoffset) {
    if (0 <= pcre_exec (ud->pr, ud->extra, text, textlen, newoffset, eflags,
                        ud->match, (NSUB(ud) + 1) * 3)) {
      if (SUB_LEN(ud,0)) {
        PUSH_END (L, ud, 0, 0);                   /* update start offset */
        lua_replace (L, lua_upvalueindex (4));
        /* push text preceding the match */
        lua_pushlstring (L, text + startoffset, SUB_BEG(ud,0) - startoffset);
        /* push either captures or entire match */
        if (NSUB(ud)) {
          push_substrings (L, ud, text);
          return 1 + NSUB(ud);
        }
        else {
          PUSH_SUB (L, ud, text, 0);
          return 2;
        }
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
  compile_regex (L, &argC, &argE.ud);            /* 1-st upvalue: ud */
  lua_pushlstring (L, argE.text, argE.textlen);  /* 2-nd upvalue: s  */
  lua_pushinteger (L, argE.eflags);              /* 3-rd upvalue: ef */
  lua_pushinteger (L, 0);                        /* 4-th upvalue: startoffset */
  lua_pushcclosure (L, iter, 4);
  return 1;
}

static int Lpcre_gmatch (lua_State *L) {
  return generic_gmatch (L, gmatch_iter);
}

static int Lpcre_split (lua_State *L) {
  return generic_gmatch (L, split_iter);
}

static int generic_find (lua_State *L, int find) {
  int res;
  TPcre *ud;
  TArgComp argC;
  TArgExec argE;

  checkarg_find_f (L, &argC, &argE);
  compile_regex (L, &argC, &ud);
  res = pcre_exec (ud->pr, ud->extra, argE.text, argE.textlen, argE.startoffset,
    argE.eflags, ud->match, (NSUB(ud) + 1) * 3);
  if (res >= 0) {
    if (find)
      PUSH_OFFSETS (L, ud, 0, 0);
    if (NSUB(ud))    /* push captures */
      push_substrings (L, ud, argE.text);
    else if (!find) {
      PUSH_SUB (L, ud, argE.text, 0);
      return 1;
    }
    return find ? NSUB(ud) + 2 : NSUB(ud);
  }
  lua_pushnil (L);
  lua_pushinteger (L, res);
  return 2;
}

static int Lpcre_find (lua_State *L) {
  return generic_find (L, 1);
}

static int Lpcre_match (lua_State *L) {
  return generic_find (L, 0);
}

static int Lpcre_gsub (lua_State *L) {
  TPcre *ud;
  TArgComp argC;
  TArgExec argE;
  int reps = 0, st = 0;
  TBuffer BufOut, BufRep;
  TFreeList freelist;
  /*--------------------------------------------------------------------------*/
  checkarg_gsub (L, &argC, &argE);
  compile_regex (L, &argC, &ud);
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
    res = pcre_exec (ud->pr, ud->extra, argE.text, (int)argE.textlen, st,
                     argE.eflags, ud->match, (NSUB(ud) + 1) * 3);
    if (res < 0)
      break;
    ++reps;
    from = SUB_BEG(ud,0);
    to = SUB_END(ud,0);
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
              buffer_addlstring (&BufOut, argE.text + SUB_BEG(ud,num),
                                 SUB_LEN(ud,num));
          }
        }
        else buffer_addlstring (&BufOut, str, num);
      }
    }
    /*------------------------------------------------------------------------*/
    else if (argE.reptype == LUA_TTABLE) {
      if (NSUB(ud) > 0)
        PUSH_SUB_OR_FALSE (L, ud, argE.text, 1);
      else
        lua_pushlstring (L, argE.text + from, to - from);
      lua_gettable (L, argE.funcpos);
    }
    /*------------------------------------------------------------------------*/
    else if (argE.reptype == LUA_TFUNCTION) {
      int narg;
      lua_pushvalue (L, argE.funcpos);
      if (NSUB(ud) > 0) {
        push_substrings (L, ud, argE.text);
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

static int Lpcre_gc (lua_State *L) {
  TPcre *ud = check_ud (L, 1);
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
  TPcre *ud = check_ud (L, 1);
  if (ud->freed == 0)
    lua_pushfstring (L, "%s (%p)", pcre_typename, (void*)ud);
  else
    lua_pushfstring (L, "%s (deleted)", pcre_typename);
  return 1;
}

static int Lpcre_version (lua_State *L) {
  lua_pushstring (L, pcre_version ());
  return 1;
}

static const luaL_reg pcremeta[] = {
  { "exec",        Lpcre_exec },
  { "tfind",       Lpcre_tfind },    /* old match */
#if PCRE_MAJOR >= 6
  { "dfa_exec",    Lpcre_dfa_exec },
#endif
  { "__gc",        Lpcre_gc },
  { "__tostring",  Lpcre_tostring },
  { NULL, NULL }
};

static const luaL_reg rexlib[] = {
  { "match",       Lpcre_match },
  { "find",        Lpcre_find },
  { "gmatch",      Lpcre_gmatch },
  { "gsub",        Lpcre_gsub },
  { "split",       Lpcre_split },
  { "new",         Lpcre_new },
  { "plainfind",   plainfind_func },
  { "flags",       Lpcre_get_flags },
  { "config",      Lpcre_config },
  { "version",     Lpcre_version },
  { NULL, NULL }
};

/* Open the library */
REX_API int REX_OPENLIB (lua_State *L) {
  if (PCRE_MAJOR > atoi (pcre_version ())) {
    return luaL_error (L, "%s requires at least version %d of PCRE library",
      REX_LIBNAME, (int)PCRE_MAJOR);
  }
  createmeta (L, pcre_handle);
  luaL_register (L, NULL, pcremeta);
  lua_pop (L, 1);
  luaL_register (L, REX_LIBNAME, rexlib);
  lua_pushliteral (L, "Lrexlib 2.0.1 (for PCRE)");
  lua_setfield (L, -2, "_VERSION");
  return 1;
}

