/* common.c */
/* See Copyright Notice in the file LICENSE */

#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "lua.h"
#include "lauxlib.h"
#include "common.h"

void *Lmalloc(lua_State *L, size_t size)
{
  void *p = malloc(size);
  if(p == NULL)
    luaL_error(L, "malloc failed");
  return p;
}

int get_startoffset(lua_State *L, int stackpos, size_t len)
{
  int startoffset = luaL_optint(L, stackpos, 1);
  if(startoffset > 0)
    startoffset--;
  else if(startoffset < 0) {
    startoffset += len;
    if(startoffset < 0)
      startoffset = 0;
  }
  return startoffset;
}

/* This function fills a table with string-number pairs.
   The table can be passed as the 1-st lua-function parameter,
   otherwise it is created. The return value is the filled table.
*/
int get_flags (lua_State *L, const flag_pair *arr)
{
  const flag_pair *p;
  int nparams = lua_gettop(L);

  if(nparams == 0)
    lua_newtable(L);
  else {
    if(!lua_istable(L, 1))
      luaL_argerror(L, 1, "not a table");
    if(nparams > 1)
      lua_pushvalue(L, 1);
  }

  for(p=arr; p->key != NULL; p++) {
    lua_pushstring(L, p->key);
    lua_pushinteger(L, p->val);
    lua_rawset(L, -3);
  }
  return 1;
}

void createmeta(lua_State *L, const char *name)
{
  luaL_newmetatable(L, name);   /* create new metatable */
  lua_pushliteral(L, "__index");
  lua_pushvalue(L, -2);         /* push metatable */
  lua_rawset(L, -3);            /* metatable.__index = metatable, for OO-style use */
}

void CheckStack (lua_State *L, int extraslots)
{
  if (lua_checkstack (L, extraslots) == 0)
    luaL_error (L, "cannot add %d stack slots", extraslots);
}

int OptLimit (lua_State *L, int pos) {
  int a;
  if (lua_isnoneornil (L, pos))
    return -1;
  a = luaL_checkint (L, pos);
  return a < 0 ? 0 : a;
}

/* function plainfind (s, p, [st], [ci]) */
int plainfind_func (lua_State *L) {
  size_t textlen, patlen;
  const char *text = luaL_checklstring (L, 1, &textlen);
  const char *pattern = luaL_checklstring (L, 2, &patlen);
  const char *from = text + get_startoffset (L, 3, textlen);
  int ci = lua_toboolean (L, 4);
  const char *end = text + textlen;

  for (; from + patlen <= end; ++from) {
    const char *f = from, *p = pattern;
    size_t len = patlen + 1;
    if (ci) {
      while (--len) {
        if (toupper (*f++) != toupper (*p++))
          break;
      }
    }
    else {
      while (--len) {
        if (*f++ != *p++)
          break;
      }
    }
    if (len == 0) {
      lua_pushinteger (L, from - text + 1);
      lua_pushinteger (L, from - text + patlen);
      return 2;
    }
  }
  lua_pushnil (L);
  return 1;
}

/* Classes */

/*
 *  class TFreeList
 *  ***************
 *  Simple array of pointers to malloc'ed memory blocks.
 *  The array has fixed capacity (not expanded automatically).
 */

void freelist_init (TFreeList *fl) {
  fl->top = 0;
}

void freelist_add (TFreeList *fl, void *p) {
  fl->list[fl->top++] = p;
}

void freelist_free (TFreeList *fl) {
  while (fl->top > 0)
    free (fl->list[--fl->top]);
}

/*
 *  class TBuffer
 *  *************
 *  Auto-extensible array of characters for building long strings incrementally.
 *    * Differs from luaL_Buffer in that:
 *       *  it does not use Lua facilities (except luaL_error when malloc fails)
 *       *  its operations do not change Lua stack top position
 *       *  buffer_addvalue does not extract the value from Lua stack
 *       *  buffer_pushresult does not have to be the last operation
 *    * Uses TFreeList class:
 *       *  for inserting itself into a TFreeList instance for future clean-up
 *       *  calls freelist_free prior to calling luaL_error.
 *    * Has specialized "Z-operations" for maintaining mixed string/integer
 *      array:  bufferZ_addlstring, bufferZ_addnum and bufferZ_next.
 *       *  if the array is intended to be "mixed", then the methods
 *          buffer_addlstring and buffer_addvalue must not be used
 *          (the application will crash on bufferZ_next).
 *       *  conversely, if the array is not intended to be "mixed",
 *          then the method bufferZ_next must not be used.
 */

enum { ID_NUMBER, ID_STRING };

void buffer_init (TBuffer *buf, size_t sz, lua_State *L, TFreeList *fl) {
  buf->arr = (char*) malloc (sz);
  if (!buf->arr) {
    freelist_free (fl);
    luaL_error (L, "malloc failed");
  }
  buf->size = sz;
  buf->top = 0;
  buf->L = L;
  buf->freelist = fl;
  freelist_add (fl, buf->arr);
}

void buffer_pushresult (TBuffer *buf) {
  lua_pushlstring (buf->L, buf->arr, buf->top);
}

void buffer_addlstring (TBuffer *buf, const void *src, size_t sz) {
  size_t newtop = buf->top + sz;
  if (newtop > buf->size) {
    char *p = (char*) realloc (buf->arr, 2 * newtop);   /* 2x expansion */
    if (!p) {
      freelist_free (buf->freelist);
      luaL_error (buf->L, "realloc failed");
    }
    buf->arr = p;
    buf->size = 2 * newtop;
  }
  memcpy (buf->arr + buf->top, src, sz);
  buf->top = newtop;
}

void buffer_addvalue (TBuffer *buf, int stackpos) {
  const char *p;
  size_t len;
  p = lua_tolstring (buf->L, stackpos, &len);
  buffer_addlstring (buf, p, len);
}

void bufferZ_addlstring (TBuffer *buf, const void *src, size_t len) {
  size_t header[2] = { ID_STRING };
  header[1] = len;
  buffer_addlstring (buf, header, sizeof (header));
  buffer_addlstring (buf, src, len);
}

void bufferZ_addnum (TBuffer *buf, size_t num) {
  size_t header[2] = { ID_NUMBER };
  header[1] = num;
  buffer_addlstring (buf, header, sizeof (header));
}

/******************************************************************************
  The intended use of this function is as follows:
        size_t iter = 0;
        while (bufferZ_next (buf, &iter, &num, &str)) {
          if (str) do_something_with_string (str, num);
          else     do_something_with_number (num);
        }
*******************************************************************************
*/
int bufferZ_next (TBuffer *buf, size_t *iter, size_t *num, const char **str) {
  if (*iter < buf->top) {
    size_t *ptr_header = (size_t*)(buf->arr + *iter);
    *num = ptr_header[1];
    *iter += 2 * sizeof (size_t);
    *str = NULL;
    if (*ptr_header == ID_STRING) {
      *str = buf->arr + *iter;
      *iter += *num;
    }
    return 1;
  }
  return 0;
}

