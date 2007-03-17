-- See Copyright Notice in the file LICENSE

local luatest = require "luatest"
local N = luatest.NT

local function set_f_find (lib, flg)
return {
  Name = "Function find",
  Func = lib.find,
  --{subj,   patt,      st,cf,ef},           { results }
  { {"a\0c", ".+"},                          { 1,3 }   }, -- subj contains nul
  { {"a\0c", "a\0c",    N,flg.PEND},         { 1,3 }   }, -- subj and patt contain nul
}
end

local function set_f_match (lib, flg)
return {
  Name = "Function match",
  Func = lib.match,
  --{subj,   patt,      st,cf,ef},           { results }
  { {"a\0c", ".+"},                          {"a\0c"} }, -- subj contains nul
  { {"a\0c", "a\0c",    N,flg.PEND},         {"a\0c"} }, -- subj and patt contain nul
}
end

local function set_m_exec (lib, flg)
return {
  Name = "Method exec",
  Method = "exec",
--  {patt,cf},         {subj,st,ef}           { results }
  { {".+"},            {"a\0c"},              {1,3,{}} }, -- subj contains nul
  { {"a\0c",flg.PEND}, {"a\0c"},              {1,3,{}} }, -- subj and patt contain nul
}
end

local function set_m_tfind (lib, flg)
return {
  Name = "Method tfind",
  Method = "tfind",
--  {patt,cf},         {subj,st,ef}           { results }
  { {".+"},            {"a\0c"},              {1,3,{}} }, -- subj contains nul
  { {"a\0c",flg.PEND}, {"a\0c"},              {1,3,{}} }, -- subj and patt contain nul
}
end

return function (libname)
  local lib = require (libname)
  local flags = lib.flags ()
  return {
    set_f_match  (lib, flags),
    set_f_find   (lib, flags),
    set_m_exec   (lib, flags),
    set_m_tfind  (lib, flags),
  }
end

