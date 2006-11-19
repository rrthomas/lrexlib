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
#  define REX_LIBNAME "rex_pcre"
#endif
#ifndef REX_OPENLIB
#  define REX_OPENLIB luaopen_rex_pcre
#endif

const char pcre_handle[] = REX_LIBNAME"?regex_handle";
const char pcre_typename[] = REX_LIBNAME"_regex";

/*  Data Types
 ******************************************************************************
 */

typedef struct {
  pcre       * pr;
  pcre_extra * extra;
  int        * match;
  int          ncapt;
  const unsigned char * tables;
} TPcre;

typedef struct {             /* pcre_compile arguments */
  const char * pattern;
  int          cflags;
  const char * locale;
} TArgComp;

typedef struct {             /* pcre_exec arguments */
  TPcre      * ud;
  const char * text;
  size_t       textlen;
  int          startoffset;
  int          eflags;
  int          funcpos;
  int          maxmatch;
} TArgExec;

typedef struct {
  pcre_extra   own_extra;    /* own pcre_extra block */
  pcre_extra * ptr_extra;    /* pointer to used pcre_extra block */
  lua_State  * L;            /* lua state */
  int          funcpos;      /* function position on Lua stack */
} TExecData;

/*  Functions
 ******************************************************************************
 */

static TPcre* CheckUD (lua_State *L, int stackpos) {
  TPcre *ud = (TPcre *)luaL_checkudata (L, stackpos, pcre_handle);
  luaL_argcheck (L, ud != NULL, stackpos, "compiled regexp expected");
  return ud;
}

static void Check_arg_new (lua_State *L, TArgComp *argC) {
  argC->pattern = luaL_checkstring (L, 1);
  argC->cflags = luaL_optint (L, 2, 0);
  argC->locale = luaL_optstring (L, 3, NULL);
}

/* function find (s, p, [st], [cf], [lo], [ef], [co]) */
static void Check_arg_findmatch_func (lua_State *L, TArgComp *argC, TArgExec *argE) {
  argE->text = luaL_checklstring (L, 1, &argE->textlen);
  argC->pattern = luaL_checkstring (L, 2);
  argE->startoffset = get_startoffset (L, 3, argE->textlen);
  argC->cflags = luaL_optint (L, 4, 0);
  argC->locale = luaL_optstring (L, 5, NULL);
  argE->eflags = luaL_optint (L, 6, 0);
  argE->funcpos = OptFunction (L, 7);
}

/* method r:find (s, [st], [ef], [co]) */
static void Check_arg_findmatch_method (lua_State *L, TArgExec *argE) {
  argE->ud = CheckUD (L, 1);
  argE->text = luaL_checklstring (L, 2, &argE->textlen);
  argE->startoffset = get_startoffset (L, 3, argE->textlen);
  argE->eflags = luaL_optint (L, 4, 0);
  argE->funcpos = OptFunction (L, 5);
}

/* method r:gmatch (s, [ef]) */
static void Check_arg_gmatch_method (lua_State *L, TArgExec *argE) {
  argE->ud = CheckUD (L, 1);
  argE->text = luaL_checklstring (L, 2, &argE->textlen);
  argE->eflags = luaL_optint (L, 3, 0);
}

/* function gmatch (s, p, [cf], [lo], [ef]) */
static void Check_arg_gmatch_func (lua_State *L, TArgComp *argC, TArgExec *argE) {
  argE->text = luaL_checklstring (L, 1, &argE->textlen);
  argC->pattern = luaL_checkstring (L, 2);
  argC->cflags = luaL_optint (L, 3, 0);
  argC->locale = luaL_optstring (L, 4, NULL);
  argE->eflags = luaL_optint (L, 5, 0);
}

/* method r:oldgmatch (s, f, [n], [ef]) */
static void Check_arg_oldgmatch_method (lua_State *L, TArgExec *argE) {
  argE->ud = CheckUD (L, 1);
  argE->text = luaL_checklstring (L, 2, &argE->textlen);
  argE->funcpos = CheckFunction (L, 3);
  argE->maxmatch = luaL_optint (L, 4, 0);
  argE->eflags = luaL_optint (L, 5, 0);
}

static int Lpcre_maketables (const char* locale, const unsigned char ** tables) {
  char old_locale[256];
  strcpy (old_locale, setlocale (LC_CTYPE, NULL)); /* store the locale */
  if (NULL == setlocale (LC_CTYPE, locale))        /* set new locale */
    return 1;
  *tables = pcre_maketables ();             /* make tables with new locale */
  setlocale (LC_CTYPE, old_locale);         /* restore the old locale */
  return 0;
}

static int Lpcre_comp (lua_State *L, const TArgComp *argC, TPcre **pud) {
  const char *error;
  int erroffset;
  TPcre *ud;
  const unsigned char *tables = NULL;

  if (argC->locale) {
    if (Lpcre_maketables (argC->locale, &tables) != 0)
      return luaL_error (L, "cannot set locale");
  }

  ud = (TPcre*)lua_newuserdata (L, sizeof (TPcre));
  luaL_getmetatable (L, pcre_handle);
  lua_setmetatable (L, -2);
  ud->match = NULL;
  ud->extra = NULL;
  ud->tables = tables; /* keep this for eventual freeing */

  ud->pr = pcre_compile (argC->pattern, argC->cflags, &error, &erroffset, tables);
  if (!ud->pr) {
    return luaL_error (L, "%s (pattern offset: %d)", error, erroffset+1);
                         /* show offset 1-based as it's common in Lua */
  }

  ud->extra = pcre_study (ud->pr, 0, &error);
  if (error) return luaL_error (L, "%s", error);

  pcre_fullinfo (ud->pr, ud->extra, PCRE_INFO_CAPTURECOUNT, &ud->ncapt);
  /* need (2 ints per capture, plus one for substring match) * 3/2 */
  ud->match = (int *) Lmalloc (L, (ud->ncapt + 1) * 3 * sizeof (int));

  if (pud) *pud = ud;
  return 1;
}

static int Lpcre_new (lua_State *L) {
  TArgComp argC;
  Check_arg_new (L, &argC);
  return Lpcre_comp (L, &argC, NULL);
}

static void Lpcre_push_substrings (lua_State *L, const TArgExec *argE) {
  TPcre *ud = argE->ud;
  int i, namecount, name_entry_size;
  unsigned char *name_table;
  unsigned char *tabptr;
  const int *match = ud->match;

  lua_newtable (L);
  for (i = 1; i <= ud->ncapt; i++) {
    int j = i * 2;
    if (match[j] >= 0)
      lua_pushlstring (L, argE->text + match[j], match[j + 1] - match[j]);
    else
      lua_pushboolean (L, 0);
    lua_rawseti (L, -2, i);
  }

  /* now do named subpatterns - NJG */
  pcre_fullinfo (ud->pr, ud->extra, PCRE_INFO_NAMECOUNT, &namecount);
  if (namecount <= 0)
    return;
  pcre_fullinfo (ud->pr, ud->extra, PCRE_INFO_NAMETABLE, &name_table);
  pcre_fullinfo (ud->pr, ud->extra, PCRE_INFO_NAMEENTRYSIZE, &name_entry_size);
  tabptr = name_table;
  for (i = 0; i < namecount; i++) {
    int n = (tabptr[0] << 8) | tabptr[1]; /* number of the capturing parenthesis */
    if (n > 0 && n <= ud->ncapt) {   /* check range */
      int j = n * 2;
      lua_pushstring (L, tabptr + 2); /* name of the capture, zero terminated */
      if (match[j] >= 0)
        lua_pushlstring (L, argE->text + match[j], match[j + 1] - match[j]);
      else
        lua_pushboolean (L, 0);
      lua_rawset (L, -3);
    }
    tabptr += name_entry_size;
  }
}

static void Lpcre_push_offsets (lua_State *L, const TArgExec *argE) {
  TPcre *ud = argE->ud;
  int i, j, k;

  lua_newtable (L);
  for (i=1, j=1; i <= ud->ncapt; i++) {
    k = i * 2;
    if (ud->match[k] >= 0) {
      lua_pushinteger (L, ud->match[k] + 1);
      lua_rawseti (L, -2, j++);
      lua_pushinteger (L, ud->match[k+1]);
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

static void put_integer (lua_State *L, const char *key, int value) {
  lua_pushstring (L, key);
  lua_pushinteger (L, value);
  lua_rawset (L, -3);
}

static int Lpcre_callout (pcre_callout_block *block) {
  TExecData *data;
  lua_State *L;
  int result;

  if (!block || !block->callout_data)
    return 0;

  data = (TExecData*) block->callout_data;
  L = data->L;
  lua_pushvalue (L, data->funcpos);
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

  result = lua_pcall (L, 1, 1, 0);
  if ((result == 0) && lua_isnumber (L, -1))
    result = lua_tointeger (L, -1);
  else
    result = PCRE_ERROR_CALLOUT;
  lua_pop (L, 1);
  return result;
}

static void LpcreSetExecData (lua_State *L, const TArgExec *argE, TExecData *trg) {
  /* check callout and initialize if required */
  trg->funcpos = 0;
  trg->ptr_extra = argE->ud->extra;
  if (argE->funcpos) {
    if (trg->ptr_extra == NULL) {
      /* set up our own pcre_extra block */
      memset (&trg->own_extra, 0, sizeof (pcre_extra));
      trg->ptr_extra = &trg->own_extra;
    }
    trg->ptr_extra->flags |= PCRE_EXTRA_CALLOUT_DATA;
    trg->ptr_extra->callout_data = trg;
    trg->funcpos = argE->funcpos;
    trg->L = L;
  }
  else if (trg->ptr_extra) {
    trg->ptr_extra->callout_data = NULL;
  }
}

typedef void (*fptrPushMatches) (lua_State *L, const TArgExec *argE);

static int Lpcre_oldmatch_generic (lua_State *L, fptrPushMatches push_matches) {
  TArgExec argE;
  TExecData ed;
  int res;

  Check_arg_findmatch_method (L, &argE);
  LpcreSetExecData (L, &argE, &ed);
  res = pcre_exec (argE.ud->pr, ed.ptr_extra, argE.text, (int)argE.textlen,
                   argE.startoffset, argE.eflags, argE.ud->match,
                   (argE.ud->ncapt + 1) * 3);

  if (res >= 0) {
    lua_pushinteger (L, argE.ud->match[0] + 1);
    lua_pushinteger (L, argE.ud->match[1]);
    (*push_matches)(L, &argE);
    lua_pushinteger (L, res);
    return 4;
  }
  lua_pushnil (L);
  lua_pushinteger (L, res);
  return 2;
}

#if PCRE_MAJOR >= 6
static int Lpcre_dfa_exec (lua_State *L)
{
  TArgExec argE;
  TExecData ed;
  int res;
  int ovector[30];
  int wspace[40];

  Check_arg_findmatch_method (L, &argE);
  LpcreSetExecData (L, &argE, &ed);
  res = pcre_dfa_exec (argE.ud->pr, ed.ptr_extra, argE.text, (int)argE.textlen,
    argE.startoffset, argE.eflags, ovector, DIM (ovector), wspace, DIM (wspace));

  if (res >= 0 || res == PCRE_ERROR_PARTIAL) {
    int i;
    int max = (res>0) ? res : (res==0) ? (int)DIM (ovector)/2 : 1;
    lua_pushinteger (L, ovector[0] + 1);         /* 1-st return value */
    lua_newtable (L);                            /* 2-nd return value */
    for (i=0; i<max; i++) {
      lua_pushinteger (L, ovector[i+i+1]);
      lua_rawseti (L, -2, i+1);
    }
    lua_pushinteger (L, res);                    /* 3-rd return value */
    return 3;
  }
  lua_pushnil (L);
  lua_pushinteger (L, res);
  return 2;
}
#endif /* #if PCRE_MAJOR >= 6 */

static int Lpcre_oldmatch_method (lua_State *L) {
  return Lpcre_oldmatch_generic (L, Lpcre_push_substrings);
}

static int Lpcre_exec_method (lua_State *L) {
  return Lpcre_oldmatch_generic (L, Lpcre_push_offsets);
}

static int Lpcre_gmatch_iter (lua_State *L) {
  int res, i;
  size_t len;
  TPcre *ud = (TPcre*) lua_touserdata (L, lua_upvalueindex (1));
  const char *text = lua_tolstring (L, lua_upvalueindex (2), &len);
  int eflags = lua_tointeger (L, lua_upvalueindex (3));
  int startoffset = lua_tointeger (L, lua_upvalueindex (4));

  res = pcre_exec (ud->pr, ud->extra, text, (int)len, startoffset, eflags,
                  ud->match, (ud->ncapt + 1) * 3);
  if (res >= 0) {
    /* update start offset */
    len = (ud->match[1] == ud->match[0]) ? 1 : 0; /* prevent endless loop */
    lua_pushinteger (L, ud->match[1] + len);
    lua_replace (L, lua_upvalueindex (4));
    /* push either captures or entire match */
    if (ud->ncapt) {
      CheckStack (L, ud->ncapt);
      for (i = 1; i <= ud->ncapt; i++) {
        int j = i * 2;
        if (ud->match[j] >= 0)
          lua_pushlstring (L, text + ud->match[j], ud->match[j + 1] - ud->match[j]);
        else
          lua_pushboolean (L, 0);
      }
      return ud->ncapt;
    }
    else {
      lua_pushlstring (L, text + ud->match[0], ud->match[1] - ud->match[0]);
      return 1;
    }
  }
  return 0;
}

/* method r:gmatch (s, [ef]) */
static int Lpcre_gmatch_method (lua_State *L) {
  TArgExec argE;
  Check_arg_gmatch_method (L, &argE);
  lua_pushvalue (L, 1);                          /* ud */
  lua_pushlstring (L, argE.text, argE.textlen);  /* s  */
  lua_pushinteger (L, argE.eflags);              /* ef */
  lua_pushinteger (L, 0);                        /* start offset */
  lua_pushcclosure (L, Lpcre_gmatch_iter, 4);
  return 1;
}

/* function gmatch (s, p, [cf], [lo], [ef]) */
static int Lpcre_gmatch_func (lua_State *L) {
  TArgComp argC;
  TArgExec argE;
  Check_arg_gmatch_func (L, &argC, &argE);
  Lpcre_comp (L, &argC, NULL);
  lua_pushlstring (L, argE.text, argE.textlen);
  lua_pushinteger (L, argE.eflags);
  lua_pushinteger (L, 0);                  /* start offset */
  lua_pushcclosure (L, Lpcre_gmatch_iter, 4);
  return 1;
}

static int Lpcre_find_generic (lua_State *L, const TArgExec *argE,
                               TExecData *ed, int find) {
  int i, res;
  TPcre *ud = argE->ud;

  res = pcre_exec (ud->pr, ed->ptr_extra, argE->text, (int)argE->textlen,
    argE->startoffset, argE->eflags, ud->match, (ud->ncapt + 1) * 3);

  if (res >= 0) {
    if (find) {
      lua_pushinteger (L, ud->match[0] + 1);
      lua_pushinteger (L, ud->match[1]);
    }
    if (ud->ncapt) {  /* push captures */
      CheckStack (L, ud->ncapt);
      for (i = 1; i <= ud->ncapt; i++) {
        int j = i * 2;
        if (ud->match[j] >= 0)
          lua_pushlstring (L, argE->text + ud->match[j],
            ud->match[j + 1] - ud->match[j]);
        else
          lua_pushboolean (L, 0);
      }
    }
    else if (!find) {
      lua_pushlstring (L, argE->text + ud->match[0], ud->match[1] - ud->match[0]);
      return 1;
    }
    return find ? ud->ncapt + 2 : ud->ncapt;
  }
  lua_pushnil (L);
  lua_pushinteger (L, res);
  return 2;
}

static int Lpcre_findmatch_method (lua_State *L, int find) {
  TArgExec argE;
  TExecData ed;
  Check_arg_findmatch_method (L, &argE);
  LpcreSetExecData (L, &argE, &ed);
  return Lpcre_find_generic (L, &argE, &ed, find);
}

static int Lpcre_find_method (lua_State *L) {
  return Lpcre_findmatch_method (L, 1);
}

static int Lpcre_match_method (lua_State *L) {
  return Lpcre_findmatch_method (L, 0);
}

static int Lpcre_findmatch_func (lua_State *L, int find) {
  TArgComp argC;
  TArgExec argE;
  TExecData ed;
  Check_arg_findmatch_func (L, &argC, &argE);
  Lpcre_comp (L, &argC, &argE.ud);
  LpcreSetExecData (L, &argE, &ed);
  return Lpcre_find_generic (L, &argE, &ed, find);
}

static int Lpcre_find_func (lua_State *L) {
  return Lpcre_findmatch_func (L, 1);
}

static int Lpcre_match_func (lua_State *L) {
  return Lpcre_findmatch_func (L, 0);
}

static int Lpcre_oldgmatch_method (lua_State *L)
{
  int res, nmatch=0, startoffset=0;
  size_t len;
  TPcre *ud;

  TArgExec argE;
  Check_arg_oldgmatch_method (L, &argE);
  ud = argE.ud;

  while (argE.maxmatch <= 0 || nmatch < argE.maxmatch) {
    res = pcre_exec (ud->pr, ud->extra, argE.text, (int)argE.textlen, startoffset,
      argE.eflags, ud->match, (ud->ncapt + 1) * 3);
    if (res >= 0) {
      nmatch++;
      lua_pushvalue (L, argE.funcpos);
      lua_pushlstring (L, argE.text + ud->match[0], ud->match[1] - ud->match[0]);
      Lpcre_push_substrings (L, &argE);
      lua_call (L, 2, 1);
      if(lua_toboolean (L, -1))
        break;
      lua_pop(L, 1);
      if (startoffset < ud->match[1])
        startoffset = ud->match[1];
      else
        ++startoffset; /* prevent endless loop */
    } else
      break;
  }
  lua_pushinteger(L, nmatch);
  return 1;
}

static int Lpcre_gc (lua_State *L) {
  TPcre *ud = (TPcre *)luaL_checkudata (L, 1, pcre_handle);
  if (ud) {
    if (ud->pr)      pcre_free (ud->pr);
    if (ud->extra)   pcre_free (ud->extra);
    if (ud->tables)  pcre_free ((void *)ud->tables);
    if (ud->match)   free (ud->match);
  }
  return 0;
}

static int Lpcre_tostring (lua_State *L) {
  return udata_tostring (L, pcre_handle, pcre_typename);
}

static int Lpcre_version (lua_State *L) {
  lua_pushstring (L, pcre_version ());
  return 1;
}

static void CheckConfig (lua_State *L, int what, const char *key, int *pval) {
  if (0 == pcre_config (what, pval)) {
    lua_pushinteger (L, *pval);
    lua_setfield (L, -2, key);
  }
}

static int Lpcre_config (lua_State *L) {
  int val;
  lua_newtable (L);

# if PCRE_MAJOR >= 4
  CheckConfig (L, PCRE_CONFIG_UTF8, "CONFIG_UTF8", &val);
  CheckConfig (L, PCRE_CONFIG_NEWLINE, "CONFIG_NEWLINE", &val);
  CheckConfig (L, PCRE_CONFIG_LINK_SIZE, "CONFIG_LINK_SIZE", &val);
  CheckConfig (L, PCRE_CONFIG_POSIX_MALLOC_THRESHOLD, "CONFIG_POSIX_MALLOC_THRESHOLD", &val);
  CheckConfig (L, PCRE_CONFIG_MATCH_LIMIT, "CONFIG_MATCH_LIMIT", &val);
  CheckConfig (L, PCRE_CONFIG_STACKRECURSE, "CONFIG_STACKRECURSE", &val);
# endif

# if PCRE_MAJOR >= 5
  CheckConfig (L, PCRE_CONFIG_UNICODE_PROPERTIES, "CONFIG_UNICODE_PROPERTIES", &val);
# endif

# ifdef PCRE_CONFIG_MATCH_LIMIT_RECURSION
  CheckConfig (L, PCRE_CONFIG_MATCH_LIMIT_RECURSION, "CONFIG_MATCH_LIMIT_RECURSION", &val);
# endif

  return 1;
}

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
/*---------------------------------------------------------------------------*/
  { NULL, 0 }
};

static int Lpcre_get_flags (lua_State *L) {
  return get_flags (L, pcre_flags);
}

static const luaL_reg pcremeta[] = {
  { "exec",       Lpcre_exec_method },
  { "oldmatch",   Lpcre_oldmatch_method },
  { "oldgmatch",  Lpcre_oldgmatch_method },
  { "gmatch",     Lpcre_gmatch_method },
  { "find",       Lpcre_find_method },
  { "match",      Lpcre_match_method },
  { "__gc",       Lpcre_gc },
  { "__tostring", Lpcre_tostring },
#if PCRE_MAJOR >= 6
  { "dfa_exec",   Lpcre_dfa_exec },
#endif
  { NULL, NULL }
};

static const luaL_reg rexlib[] = {
  { "new",         Lpcre_new },
  { "flags",       Lpcre_get_flags },
  { "gmatch",      Lpcre_gmatch_func },
  { "find",        Lpcre_find_func },
  { "match",       Lpcre_match_func },
  { "versionPCRE", Lpcre_version },
  { "config",      Lpcre_config },
  { NULL, NULL }
};

/* Open the library */
REX_API int REX_OPENLIB (lua_State *L) {
  if (PCRE_MAJOR != atoi (pcre_version ())) {
    return luaL_error (L, "%s requires version %d of PCRE library",
      REX_LIBNAME, (int)PCRE_MAJOR);
  }
  createmeta (L, pcre_handle);
  luaL_register (L, NULL, pcremeta);
  lua_pop (L, 1);
  pcre_callout = Lpcre_callout;
  luaL_register (L, REX_LIBNAME, rexlib);
  lua_pushliteral (L, "Lrexlib 2.0 alpha (for PCRE)");
  lua_setfield (L, -2, "_VERSION");
  return 1;
}
