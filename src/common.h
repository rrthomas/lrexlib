/* common.h */
/* See Copyright Notice in the file LICENSE */

#ifndef COMMON_H
#define COMMON_H

#include "lua.h"

#define REX_VERSION "Lrexlib 2.0.2"

/* REX_API can be overridden from the command line or Makefile */
#ifndef REX_API
#  define REX_API LUALIB_API
#endif

/* Common structs and functions */

typedef struct { const char* key; int val; } flag_pair;

void createmeta (lua_State *L, const char *name);
int get_flags (lua_State *L, const flag_pair *arr);
int get_startoffset (lua_State *L, int stackpos, size_t len);
void *Lmalloc (lua_State *L, size_t size);
void CheckStack (lua_State *L, int extraslots);
int OptLimit (lua_State *L, int pos);
int plainfind_func (lua_State *L);

/* Classes */

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
void buffer_addlstring (TBuffer *buf, const void *src, size_t sz);
void buffer_addvalue (TBuffer *buf, int stackpos);
void buffer_pushresult (TBuffer *buf);

void bufferZ_putrepstring (TBuffer *buf, int reppos, int nsub);
int  bufferZ_next (TBuffer *buf, size_t *iter, size_t *len, const char **str);

#endif
