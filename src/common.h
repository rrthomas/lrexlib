/* common.h */
/* Copyright (C) Reuben Thomas 2000-2007 */
/* Copyright (C) Shmuel Zeigerman 2004-2007 */

#ifndef COMMON_H
#define COMMON_H

#include "lua.h"

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

typedef struct {
  void      * list[32];
  int         top;
} TFreeList;

typedef struct {
  size_t      size;
  size_t      top;
  char      * arr;
  lua_State * L;
  TFreeList * freelist;
} TBuffer;

void freelist_init (TFreeList *fl);
void freelist_add (TFreeList *fl, void *p);
void freelist_free (TFreeList *fl);

void buffer_init (TBuffer *buf, size_t sz, lua_State *L, TFreeList *fl);
void buffer_addlstring (TBuffer *buf, const void *src, size_t sz);
void buffer_addvalue (TBuffer *buf, int stackpos);
void buffer_pushresult (TBuffer *buf);

void bufferZ_addlstring (TBuffer *buf, const void *src, size_t sz);
void bufferZ_addnum (TBuffer *buf, size_t num);
int  bufferZ_next (TBuffer *buf, size_t *iter, size_t *len, const char **str);

#endif
