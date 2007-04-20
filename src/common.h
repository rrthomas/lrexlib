/* common.h */
/* See Copyright Notice in the file LICENSE */

#ifndef COMMON_H
#define COMMON_H

#include "lua.h"

/* Common structs and functions */

typedef struct { const char* key; int val; } flag_pair;

int get_int_field (lua_State *L, const char* field);
void set_int_field (lua_State *L, const char* field, int val);
int get_flags (lua_State *L, const flag_pair **arr);
const char *get_flag_key (const flag_pair *fp, int val);
void *Lmalloc (lua_State *L, size_t size);

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
void buffer_clear (TBuffer *buf);
void buffer_addbuffer (TBuffer *trg, TBuffer *src);
void buffer_addlstring (TBuffer *buf, const void *src, size_t sz);
void buffer_addvalue (TBuffer *buf, int stackpos);
void buffer_pushresult (TBuffer *buf);

void bufferZ_putrepstring (TBuffer *buf, int reppos, int nsub);
int  bufferZ_next (TBuffer *buf, size_t *iter, size_t *len, const char **str);

#endif
