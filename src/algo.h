/* algo.h */
/* See Copyright Notice in the file LICENSE */


/* Forward declarations */
static void gmatch_pushsubject (lua_State *L, TArgExec *argE);
static int gmatch_exec     (TUserdata *ud, TArgExec *argE);
static int findmatch_exec  (TUserdata *ud, TArgExec *argE);
static int tfind_exec      (TUserdata *ud, TArgExec *argE);
static int gsub_exec       (TUserdata *ud, TArgExec *argE, int offset);
static int split_exec      (TUserdata *ud, TArgExec *argE, int offset);
static int compile_regex   (lua_State *L, const TArgComp *argC, TUserdata **pud);
static int generate_error  (lua_State *L, const TUserdata *ud, int errcode);
static void do_named_subpatterns (lua_State *L, TUserdata *ud, const char *text);


static TUserdata* check_ud (lua_State *L, int stackpos) {
  TUserdata *ud = (TUserdata *)luaL_checkudata(L, stackpos, UD_HANDLE);
  luaL_argcheck (L, !ud->freed, stackpos, "attempt to access the deleted regex");
  return ud;
}


/* allow access to the userdata, even if it's in the deleted state */
static TUserdata* check_ud_gc (lua_State *L, int stackpos) {
  return (TUserdata *)luaL_checkudata (L, stackpos, UD_HANDLE);
}


static void checkarg_new (lua_State *L, TArgComp *argC) {
  argC->pattern = luaL_checklstring (L, 1, &argC->patlen);
  argC->cflags = luaL_optint (L, 2, CFLAGS_DEFAULT);
  OPTLOCALE (argC->locale, L, 3);
}


/* function gsub (s, patt, f, [n], [cf], [ef], [lo]) */
static void checkarg_gsub (lua_State *L, TArgComp *argC, TArgExec *argE) {
  argE->text = luaL_checklstring (L, 1, &argE->textlen);
  argC->pattern = luaL_checklstring (L, 2, &argC->patlen);
  lua_tostring (L, 3);    /* converts number (if any) to string */
  argE->reptype = lua_type (L, 3);
  if (argE->reptype != LUA_TSTRING && argE->reptype != LUA_TTABLE &&
      argE->reptype != LUA_TFUNCTION) {
    luaL_argerror (L, 3, "must be string, table or function");
  }
  argE->funcpos = 3;
  argE->funcpos2 = 4;
  argE->maxmatch = OptLimit (L, 4);
  argC->cflags = luaL_optint (L, 5, CFLAGS_DEFAULT);
  argE->eflags = luaL_optint (L, 6, EFLAGS_DEFAULT);
  OPTLOCALE (argC->locale, L, 7);
}


/* function find  (s, patt, [st], [cf], [ef], [lo]) */
/* function match (s, patt, [st], [cf], [ef], [lo]) */
static void checkarg_find_f (lua_State *L, TArgComp *argC, TArgExec *argE) {
  argE->text = luaL_checklstring (L, 1, &argE->textlen);
  argC->pattern = luaL_checklstring (L, 2, &argC->patlen);
  argE->startoffset = get_startoffset (L, 3, argE->textlen);
  argC->cflags = luaL_optint (L, 4, CFLAGS_DEFAULT);
  argE->eflags = luaL_optint (L, 5, EFLAGS_DEFAULT);
  OPTLOCALE (argC->locale, L, 6);
}


/* function gmatch (s, patt, [cf], [ef], [lo]) */
/* function split  (s, patt, [cf], [ef], [lo]) */
static void checkarg_gmatch_split (lua_State *L, TArgComp *argC, TArgExec *argE) {
  argE->text = luaL_checklstring (L, 1, &argE->textlen);
  argC->pattern = luaL_checklstring (L, 2, &argC->patlen);
  argC->cflags = luaL_optint (L, 3, CFLAGS_DEFAULT);
  argE->eflags = luaL_optint (L, 4, EFLAGS_DEFAULT);
  OPTLOCALE (argC->locale, L, 5);
}


/* method r:tfind (s, [st], [ef]) */
/* method r:exec  (s, [st], [ef]) */
static void checkarg_tfind (lua_State *L, TArgExec *argE, TUserdata **ud) {
  *ud = check_ud (L, 1);
  argE->text = luaL_checklstring (L, 2, &argE->textlen);
  argE->startoffset = get_startoffset (L, 3, argE->textlen);
  argE->eflags = luaL_optint (L, 4, EFLAGS_DEFAULT);
}


static int ud_new (lua_State *L) {
  TArgComp argC;
  checkarg_new (L, &argC);
  return compile_regex (L, &argC, NULL);
}


static void push_substrings (lua_State *L, TUserdata *ud, const char *text) {
  int i;
  CheckStack (L, NSUB(ud));
  for (i = 1; i <= NSUB(ud); i++) {
    PUSH_SUB_OR_FALSE (L, ud, text, i);
  }
}


static int gsub (lua_State *L) {
  TUserdata *ud;
  TArgComp argC;
  TArgExec argE;
  int n_mat = 0, n_subst = 0, st = 0;
  TBuffer BufOut, BufRep, BufTemp, *pBuf = &BufOut;
  TFreeList freelist;
  /*------------------------------------------------------------------*/
  checkarg_gsub (L, &argC, &argE);
  compile_regex (L, &argC, &ud);
  freelist_init (&freelist);
  /*------------------------------------------------------------------*/
  if (argE.reptype == LUA_TSTRING) {
    buffer_init (&BufRep, 256, L, &freelist);
    bufferZ_putrepstring (&BufRep, argE.funcpos, NSUB(ud));
  }
  /*------------------------------------------------------------------*/
  if (argE.maxmatch == GSUB_CONDITIONAL) {
    buffer_init (&BufTemp, 1024, L, &freelist);
    pBuf = &BufTemp;
  }
  /*------------------------------------------------------------------*/
  buffer_init (&BufOut, 1024, L, &freelist);
  while ((argE.maxmatch < 0 || n_mat < argE.maxmatch) && st <= (int)argE.textlen) {
    int from, to, res;
    int curr_subst = 0;
    res = gsub_exec (ud, &argE, st);
    if (res == CODE_NOMATCH)
      break;
    else if (!IS_MATCH (res)) {
      freelist_free (&freelist);
      return generate_error (L, ud, res);
    }
    ++n_mat;
    from = BASE(st) + SUB_BEG(ud,0);
    to = BASE(st) + SUB_END(ud,0);
    if (st < from) {
      buffer_addlstring (&BufOut, argE.text + st, from - st);
      PULL(st, from);
    }
    /*----------------------------------------------------------------*/
    if (argE.reptype == LUA_TSTRING) {
      size_t iter = 0, num;
      const char *str;
      while (bufferZ_next (&BufRep, &iter, &num, &str)) {
        if (str)
          buffer_addlstring (pBuf, str, num);
        else if (num == 0 || SUB_VALID (ud,num))
          buffer_addlstring (pBuf, argE.text + BASE(st) + SUB_BEG(ud,num), SUB_LEN(ud,num));
      }
      curr_subst = 1;
    }
    /*----------------------------------------------------------------*/
    else if (argE.reptype == LUA_TTABLE) {
      if (NSUB(ud) > 0)
        PUSH_SUB_OR_FALSE (L, ud, argE.text + BASE(st), 1);
      else
        lua_pushlstring (L, argE.text + from, to - from);
      lua_gettable (L, argE.funcpos);
    }
    /*----------------------------------------------------------------*/
    else if (argE.reptype == LUA_TFUNCTION) {
      int narg;
      lua_pushvalue (L, argE.funcpos);
      if (NSUB(ud) > 0) {
        push_substrings (L, ud, argE.text + BASE(st));
        narg = NSUB(ud);
      }
      else {
        lua_pushlstring (L, argE.text + from, to - from);
        narg = 1;
      }
      if (0 != lua_pcall (L, narg, 1, 0)) {
        freelist_free (&freelist);
        return lua_error (L);  /* re-raise the error */
      }
    }
    /*----------------------------------------------------------------*/
    if (argE.reptype != LUA_TSTRING) {
      if (lua_tostring (L, -1)) {
        buffer_addvalue (pBuf, -1);
        curr_subst = 1;
      }
      else if (!lua_toboolean (L, -1))
        buffer_addlstring (pBuf, argE.text + from, to - from);
      else {
        freelist_free (&freelist);
        luaL_error (L, "invalid replacement value (a %s)", luaL_typename (L, -1));
      }
      if (argE.maxmatch != GSUB_CONDITIONAL)
        lua_pop (L, 1);
    }
    /*----------------------------------------------------------------*/
    if (argE.maxmatch == GSUB_CONDITIONAL) {
      /* Call the function */
      lua_pushvalue (L, argE.funcpos2);
      lua_pushinteger (L, from + 1);
      lua_pushinteger (L, to);
      if (argE.reptype == LUA_TSTRING)
        buffer_pushresult (&BufTemp);
      else {
        lua_pushvalue (L, -4);
        lua_remove (L, -5);
      }
      if (0 != lua_pcall (L, 3, 2, 0)) {
        freelist_free (&freelist);
        lua_error (L);  /* re-raise the error */
      }
      /* Handle the 1-st return value */
      if (lua_isstring (L, -2)) {               /* coercion is allowed here */
        buffer_addvalue (&BufOut, -2);          /* rep2 */
        curr_subst = 1;
      }
      else if (lua_toboolean (L, -2))
        buffer_addbuffer (&BufOut, &BufTemp);   /* rep1 */
      else {
        buffer_addlstring (&BufOut, argE.text + from, to - from); /* "no" */
        curr_subst = 0;
      }
      /* Handle the 2-nd return value */
      if (lua_type (L, -1) == LUA_TNUMBER) {    /* no coercion is allowed here */
        int n = lua_tointeger (L, -1);
        if (n < 0)                              /* n */
          n = 0;
        argE.maxmatch = n_mat + n;
      }
      else if (lua_toboolean (L, -1))           /* "yes to all" */
        argE.maxmatch = GSUB_UNLIMITED;
      else
        buffer_clear (&BufTemp);

      lua_pop (L, 2);
      if (argE.maxmatch != GSUB_CONDITIONAL)
        pBuf = &BufOut;
    }
    /*----------------------------------------------------------------*/
    n_subst += curr_subst;
    if (st < to)
      st = to;
    else if (st < (int)argE.textlen) {  /* advance by 1 char (not replaced) */
      buffer_addlstring (&BufOut, argE.text + st, 1);
      ++st;
    }
    else break;
  }
  /*------------------------------------------------------------------*/
  buffer_addlstring (&BufOut, argE.text + st, argE.textlen - st);
  buffer_pushresult (&BufOut);
  lua_pushinteger (L, n_mat);
  lua_pushinteger (L, n_subst);
  freelist_free (&freelist);
  return 3;
}


static int generic_find (lua_State *L, int find) {
  int res;
  TUserdata *ud;
  TArgComp argC;
  TArgExec argE;

  checkarg_find_f (L, &argC, &argE);
  if (argE.startoffset > (int)argE.textlen)
    return lua_pushnil(L), 1;

  compile_regex (L, &argC, &ud);
  res = findmatch_exec (ud, &argE);
  if (IS_MATCH (res)) {
    if (find)
      PUSH_OFFSETS (L, ud, BASE(argE.startoffset), 0);
    if (NSUB(ud))    /* push captures */
      push_substrings (L, ud, argE.text);
    else if (!find) {
      PUSH_SUB (L, ud, argE.text, 0);
      return 1;
    }
    return find ? NSUB(ud) + 2 : NSUB(ud);
  }
  else if (res == CODE_NOMATCH)
    return lua_pushnil (L), 1;
  else
    return generate_error (L, ud, res);
}


static int find (lua_State *L) {
  return generic_find (L, 1);
}


static int match (lua_State *L) {
  return generic_find (L, 0);
}


static int gmatch_iter (lua_State *L) {
  int res;
  TArgExec argE;
  TUserdata *ud    = (TUserdata*) lua_touserdata (L, lua_upvalueindex (1));
  argE.text        = lua_tolstring (L, lua_upvalueindex (2), &argE.textlen);
  argE.eflags      = lua_tointeger (L, lua_upvalueindex (3));
  argE.startoffset = lua_tointeger (L, lua_upvalueindex (4));

  if (argE.startoffset > (int)argE.textlen)
    return 0;

  res = gmatch_exec (ud, &argE);
  if (IS_MATCH (res)) {
    int incr = (SUB_LEN(ud,0) == 0) ? 1 : 0;          /* prevent endless loop */
    PUSH_END (L, ud, BASE(argE.startoffset)+incr, 0); /* update start offset */
    lua_replace (L, lua_upvalueindex (4));
    /* push either captures or entire match */
    if (NSUB(ud)) {
      push_substrings (L, ud, argE.text);
      return NSUB(ud);
    }
    else {
      PUSH_SUB (L, ud, argE.text, 0);
      return 1;
    }
  }
  else if (res == CODE_NOMATCH)
    return 0;
  else
    return generate_error (L, ud, res);
}


static int split_iter (lua_State *L) {
  int res, newoffset;
  TArgExec argE;
  TUserdata *ud    = (TUserdata*) lua_touserdata (L, lua_upvalueindex (1));
  argE.text        = lua_tolstring (L, lua_upvalueindex (2), &argE.textlen);
  argE.eflags      = lua_tointeger (L, lua_upvalueindex (3));
  argE.startoffset = lua_tointeger (L, lua_upvalueindex (4));

  if (argE.startoffset >= (int)argE.textlen)
    return 0;

  for (newoffset = argE.startoffset; newoffset < (int)argE.textlen; ++newoffset) {
    res = split_exec (ud, &argE, newoffset);
    if (IS_MATCH (res)) {
      if (SUB_LEN(ud,0)) {
        PUSH_END (L, ud, BASE(newoffset), 0);          /* update start offset */
        lua_replace (L, lua_upvalueindex (4));
        /* push text preceding the match */
        lua_pushlstring (L, argE.text + argE.startoffset,
                         SUB_BEG(ud,0) + BASE(newoffset) - argE.startoffset);
        /* push either captures or entire match */
        if (NSUB(ud)) {
          push_substrings (L, ud, argE.text + BASE(newoffset));
          return 1 + NSUB(ud);
        }
        else {
          PUSH_SUB (L, ud, argE.text + BASE(newoffset), 0);
          return 2;
        }
      }
    }
    else if (res == CODE_NOMATCH)
      break;
    else
      return generate_error (L, ud, res);
  }
  lua_pushinteger (L, argE.textlen);        /* mark as last iteration */
  lua_replace (L, lua_upvalueindex (4));    /* update start offset */
  lua_pushlstring (L, argE.text+argE.startoffset, argE.textlen-argE.startoffset);
  return 1;
}


static int generic_gmatch (lua_State *L, lua_CFunction iter)
{
  TArgComp argC;
  TArgExec argE;
  TUserdata *ud;
  checkarg_gmatch_split (L, &argC, &argE);
  compile_regex (L, &argC, &ud);              /* 1-st upvalue: ud */
  gmatch_pushsubject (L, &argE);              /* 2-nd upvalue: s  */
  lua_pushinteger (L, argE.eflags);           /* 3-rd upvalue: ef */
  lua_pushinteger (L, 0);                     /* 4-th upvalue: startoffset */
  lua_pushcclosure (L, iter, 4);
  return 1;
}


static int gmatch (lua_State *L) {
  return generic_gmatch (L, gmatch_iter);
}


static int split (lua_State *L) {
  return generic_gmatch (L, split_iter);
}


static void push_substring_table (lua_State *L, TUserdata *ud, const char *text) {
  int i;
  lua_newtable (L);
  for (i = 1; i <= NSUB(ud); i++) {
    PUSH_SUB_OR_FALSE (L, ud, text, i);
    lua_rawseti (L, -2, i);
  }
}


static void push_offset_table (lua_State *L, TUserdata *ud, int startoffset) {
  int i, j;
  lua_newtable (L);
  for (i=1, j=1; i <= NSUB(ud); i++) {
    if (SUB_VALID (ud,i)) {
      PUSH_START (L, ud, startoffset, i);
      lua_rawseti (L, -2, j++);
      PUSH_END (L, ud, startoffset, i);
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


static int generic_tfind (lua_State *L, int tfind) {
  TUserdata *ud;
  TArgExec argE;
  int res;

  checkarg_tfind (L, &argE, &ud);
  if (argE.startoffset > (int)argE.textlen)
    return lua_pushnil(L), 1;

  res = tfind_exec (ud, &argE);
  if (IS_MATCH (res)) {
    PUSH_OFFSETS (L, ud, BASE(argE.startoffset), 0);
    if (tfind)
      push_substring_table (L, ud, argE.text);
    else
      push_offset_table (L, ud, argE.startoffset);
    DO_NAMED_SUBPATTERNS (L, ud, argE.text);
    return 3;
  }
  else if (res == CODE_NOMATCH)
    return lua_pushnil (L), 1;
  else
    return generate_error(L, ud, res);
}


static int ud_tfind (lua_State *L) {
  return generic_tfind (L, 1);
}


static int ud_exec (lua_State *L) {
  return generic_tfind (L, 0);
}


/* function plainfind (s, p, [st], [ci]) */
static int plainfind_func (lua_State *L) {
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

