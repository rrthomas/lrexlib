/* common.c */
/* Reuben Thomas   nov00-18dec04 */
/* Shmuel Zeigerman   may04-18dec04 */

#include <stdlib.h>
#include <stdio.h>
#include <lua.h>
#include <lauxlib.h>
#include "common.h"

void L_lua_error(lua_State *L, const char *message)
{
  lua_pushstring(L, message);
  lua_error(L);
}

void *Lmalloc(lua_State *L, size_t size)
{
  void *p = malloc(size);
  if(p == NULL)
    L_lua_error(L, "malloc failed");
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

int udata_tostring(lua_State *L, const char* type_handle, const char* type_name)
{
  char buf[256];
  void *udata = luaL_checkudata(L, 1, type_handle);
  if(udata) {
    sprintf(buf, "%s (%p)", type_name, udata);
    lua_pushstring(L, buf);
  }
  else {
    sprintf(buf, "must be userdata of type '%s'", type_name);
    luaL_argerror(L, 1, buf);
  }
  return 1;
}

/* This function fills a table with string-number pairs.
   The table can be passed as the 1-st lua-function parameter,
   otherwise it is created. The return value is the filled table.
*/
int get_flags (lua_State *L, const flags_pair *arr)
{
  const flags_pair *p;
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
    lua_pushnumber(L, p->val);
    lua_rawset(L, -3);
  }
  return 1;
}

void createmeta(lua_State *L, const char *name)
{
  luaL_newmetatable(L, name);   /* create new metatable */
  lua_pushliteral(L, "__index");
  lua_pushvalue(L, -2);         /* push metatable */
  lua_rawset(L, -3);            /* metatable.__index = metatable */
}

