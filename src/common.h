/* common.h */
/* (c) Reuben Thomas 2000-2006 */
/* (c) Shmuel Zeigerman 2004-2006 */

#ifndef COMMON_H
#define COMMON_H

#include "lua.h"

/* REX_API can be overridden from the command line or Makefile */
#ifndef REX_API
#  define REX_API LUALIB_API
#endif

/* helper macro */
#define DIM(array) (sizeof (array) / sizeof (array[0]))

/* Common structs and functions */

typedef struct { const char* key; int val; } flag_pair;

void createmeta (lua_State *L, const char *name);
int get_flags (lua_State *L, const flag_pair *arr);
int get_startoffset (lua_State *L, int stackpos, size_t len);
void *Lmalloc (lua_State *L, size_t size);
int udata_tostring (lua_State *L, const char* type_handle, const char* type_name);
void CheckStack (lua_State *L, int extraslots);
int OptFunction (lua_State *L, int pos);
int CheckFunction (lua_State *L, int pos);


#endif
