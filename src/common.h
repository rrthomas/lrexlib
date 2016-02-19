/* common.h */
/* See Copyright Notice in the file LICENSE */

#ifndef COMMON_H
#define COMMON_H

#include "lua.h"

#if LUA_VERSION_NUM > 501
# define lua_objlen lua_rawlen
  int luaL_typerror (lua_State *L, int narg, const char *tname);
# undef luaL_checkint
# define luaL_checkint(L,i) ((int)luaL_checkinteger((L),(i)))
# undef luaL_optint
# define luaL_optint(L,i,d) ((int)luaL_optinteger((L),(i),(d)))
#endif

/* REX_API can be overridden from the command line or Makefile */
#ifndef REX_API
#  define REX_API LUALIB_API
#endif

/* Special values for maxmatch in gsub. They all must be negative. */
#define GSUB_UNLIMITED   -1
#define GSUB_CONDITIONAL -2

/* Common structs and functions */

typedef struct {
  const char* key;
  int val;
} flag_pair;

typedef struct {            /* compile arguments */
  const char * pattern;
  size_t       patlen;
  void       * ud;
  int          cflags;
  const char * locale;             /* PCRE, Oniguruma */
  const unsigned char * tables;    /* PCRE */
  int          tablespos;          /* PCRE */
  void       * syntax;             /* Oniguruma */
  const unsigned char * translate; /* GNU */
  int          gnusyn;             /* GNU */
} TArgComp;

typedef struct {            /* exec arguments */
  const char * text;
  size_t       textlen;
  int          startoffset;
  int          eflags;
  int          funcpos;
  int          maxmatch;
  int          funcpos2;          /* used with gsub */
  int          reptype;           /* used with gsub */
  size_t       ovecsize;          /* PCRE: dfa_exec */
  size_t       wscount;           /* PCRE: dfa_exec */
} TArgExec;

struct tagFreeList; /* forward declaration */

struct tagBuffer {
  size_t      size;
  size_t      top;
  char      * arr;
  lua_State * L;
  struct tagFreeList * freelist;
};

struct tagFreeList {
  struct tagBuffer * list[16];
  int top;
};

typedef struct tagBuffer TBuffer;
typedef struct tagFreeList TFreeList;

void freelist_init (TFreeList *fl);
void freelist_add (TFreeList *fl, TBuffer *buf);
void freelist_free (TFreeList *fl);

void buffer_init (TBuffer *buf, size_t sz, lua_State *L, TFreeList *fl);
void buffer_free (TBuffer *buf);
void buffer_clear (TBuffer *buf);
void buffer_addbuffer (TBuffer *trg, TBuffer *src);
void buffer_addlstring (TBuffer *buf, const void *src, size_t sz);
void buffer_addvalue (TBuffer *buf, int stackpos);
void buffer_pushresult (TBuffer *buf);

void bufferZ_putrepstring (TBuffer *buf, int reppos, int nsub);
int  bufferZ_next (TBuffer *buf, size_t *iter, size_t *len, const char **str);
void bufferZ_addlstring (TBuffer *buf, const void *src, size_t len);
void bufferZ_addnum (TBuffer *buf, size_t num);

int  get_int_field (lua_State *L, const char* field);
void set_int_field (lua_State *L, const char* field, int val);
int  get_flags (lua_State *L, const flag_pair **arr);
const char *get_flag_key (const flag_pair *fp, int val);
void *Lmalloc (lua_State *L, size_t size);
void *Lrealloc (lua_State *L, void *p, size_t osize, size_t nsize);
void Lfree (lua_State *L, void *p, size_t size);

#ifndef REX_NOEMBEDDEDTEST
int newmembuffer (lua_State *L);
#endif

#endif
