-- See Copyright Notice in the file LICENSE

-- This file should contain only test sets that behave identically
-- when being run with pcre or posix regex libraries.

local luatest = require "luatest"
local N = luatest.NT

local function norm(a) return a==nil and N or a end

local function get_gsub (lib)
  return lib.gsub or
    function (subj, pattern, repl, n)
      return lib.new (pattern) : gsub (subj, repl, n)
    end
end

local function set_f_gmatch (lib, flg)
  -- gmatch (s, p, [cf], [ef])
  local function test_gmatch (subj, patt)
    local out, guard = {}, 10
    for a, b in lib.gmatch (subj, patt) do
      table.insert (out, { norm(a), norm(b) })
      guard = guard - 1
      if guard == 0 then break end
    end
    return unpack (out)
  end
  return {
    Name = "Function gmatch",
    Func = test_gmatch,
  --{  subj             patt         results }
    { {("abcd"):rep(3), "\\(.\\)b.\\(d\\)"}, {{"a","d"},{"a","d"},{"a","d"}} },
  }
end

local function set_f_split (lib, flg)
  -- split (s, p, [cf], [ef])
  local function test_split (subj, patt)
    local out, guard = {}, 10
    for a, b, c in lib.split (subj, patt) do
      table.insert (out, { norm(a), norm(b), norm(c) })
      guard = guard - 1
      if guard == 0 then break end
    end
    return unpack (out)
  end
  return {
    Name = "Function split",
    Func = test_split,
  --{  subj             patt      results }
    { {"ab<78>c", "<\\(.\\)\\(.\\)>"},    {{"ab","7","8"}, {"c",N,N},            } },
  }
end

local function set_f_find (lib, flg)
  return {
    Name = "Function find",
    Func = lib.find,
  --  {subj, patt, st},         { results }
    { {"abcd", "\\(.\\)b.\\(d\\)"},     { 1,4,"a","d" }}, -- [captures]
  }
end

local function set_f_match (lib, flg)
  return {
    Name = "Function match",
    Func = lib.match,
  --  {subj, patt, st},         { results }
    { {"abcd", "\\(.\\)b.\\(d\\)"},     {"a","d"} }, -- [captures]
  }
end

local function set_m_exec (lib, flg)
  return {
    Name = "Method exec",
    Method = "exec",
  --{patt},                 {subj, st}           { results }
    { { "\\(.\\)b.\\(d\\)"},        {"abcd"},            {1,4,{1,1,4,4}}},--[captures]
    { {"\\(a+\\)6+\\(b+\\)"},       {"Taa66bbT",2},      {2,7,{2,3,6,7}}},--[st+captures]
  }
end

local function set_m_tfind (lib, flg)
  return {
    Name = "Method tfind",
    Method = "tfind",
  --{patt},                 {subj, st}           { results }
    { {"\\(.\\)b.\\(d\\)"},         {"abcd"},            {1,4,{"a","d"}}},--[captures]
  }
end

local function set_m_find (lib, flg)
  return {
    Name = "Method find",
    Method = "find",
  --{patt},                 {subj, st}           { results }
    { {"\\(.\\)b.\\(d\\)"},         {"abcd"},            {1,4,"a","d"}},--[captures]
  }
end

local function set_m_match (lib, flg)
  return {
    Name = "Method match",
    Method = "match",
  --{patt},                 {subj, st}           { results }
    {{ "\\(.\\)b.\\(d\\)"},         {"abcd"},            {"a","d"} }, --[captures]
  }
end

local function set_f_gsub3 (lib, flg)
  return {
    Name = "Function gsub, set3",
    Func = get_gsub (lib),
  --{ s,      p,      f,  n,   res1,res2,res3 },
    { {"abc", "\\(a\\)",  "%1" }, {"abc", 1, 1} },
    { {"abc", "\\(a\\)",  "%2" }, "invalid capture index" },
  }
end

local function set_f_gsub5 (lib, flg)
  local function frep1 () end                       -- returns nothing
  local function frep2 () return "#" end            -- ignores arguments
  local function frep3 (...) return table.concat({...}, ",") end -- "normal"
  local function frep4 () return {} end             -- invalid return type
  local function frep5 () return "7", "a" end       -- 2-nd return is "a"
  local function frep6 () return "7", "break" end   -- 2-nd return is "break"
  local subj = "a2c3"
  return {
    Name = "Function gsub, set5",
    Func = get_gsub (lib),
  --{ s,     p,          f,   n,   res1,     res2, res3 },
    { {subj, "a\\(.\\)c\\(.\\)", frep1 }, {subj,        1, 0} },
    { {subj, "a\\(.\\)c\\(.\\)", frep2 }, {"#",         1, 1} },
    { {subj, "a\\(.\\)c\\(.\\)", frep3 }, {"2,3",       1, 1} },
  }
end

local function set_f_gsub6 (lib, flg)
  local tab1, tab2, tab3 = {}, { ["2"] = 56 }, { ["2"] = {} }
  local subj = "a2c3"
  return {
    Name = "Function gsub, set6",
    Func = get_gsub (lib),
  --{ s,     p,          f, n,   res1,res2,res3 },
    { {subj, "a\\(.\\)c\\(.\\)", tab1 }, {subj,  1, 0} },
    { {subj, "a\\(.\\)c\\(.\\)", tab2 }, {"56",  1, 1} },
    { {subj, "a\\(.\\)c\\(.\\)", tab3 }, "invalid replacement type" },
  }
end

return function (libname)
  local lib = require (libname)
  _G[libname].setsyntax ("EMACS")
  return {
    set_f_gmatch    (lib),
    set_f_split     (lib),
    set_f_find      (lib),
    set_f_match     (lib),
    set_m_exec      (lib),
    set_m_tfind     (lib),
    set_m_find      (lib),
    set_m_match     (lib),
    set_f_gsub3     (lib),
    set_f_gsub5     (lib),
    set_f_gsub6     (lib),
  }
end
