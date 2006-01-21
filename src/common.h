/* common.h */
/* Reuben Thomas   nov00-18dec04 */
/* Shmuel Zeigerman   may04-18dec04 */

#ifndef COMMON_H
#define COMMON_H

#include <lua.h>

/* REX_LIB_API can be overridden from the command line or Makefile */
#ifndef REX_LIB_API
  #define REX_LIB_API LUALIB_API
#endif

/* Common structs and functions */

typedef struct { const char* key; int val; } flags_pair;

void createmeta (lua_State *L, const char *name);
int  get_flags (lua_State *L, const flags_pair *arr);
int  get_startoffset (lua_State *L, int stackpos, size_t len);
void L_lua_error (lua_State *L, const char *message);
void *Lmalloc (lua_State *L, size_t size);
int  udata_tostring (lua_State *L, const char* type_handle, const char* type_name);

#endif

