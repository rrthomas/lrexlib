-- [ Shmuel Zeigerman; Nov-Nov 2006 ]

local fw = require "framework"

-- gmatch (s, p, [cf], [ef])
local function test_func_gmatch (lib)
  print ("Function gmatch")
  local rep = 10
  local subj = string.rep ("abcd", rep)
  for a, b in lib.gmatch (subj, "(.)b.(d)") do
    if a == "a" and b == "d" then rep = rep - 1 end
  end
  if rep ~= 0 then print ("  FAIL") end
end

-- r:gmatch (s, [ef])
local function test_method_gmatch (lib)
  print ("Method gmatch")
  local rep = 10
  local subj = string.rep ("abcd", rep)
  local r = lib.new ("(.)b.(d)")
  for a, b in r:gmatch (subj) do
    if a == "a" and b == "d" then rep = rep - 1 end
  end
  if rep ~= 0 then print ("  FAIL") end
end

-- r:oldgmatch (s, f, [n], [ef])
local function test_method_oldgmatch (lib)
  print ("Method oldgmatch")
  local r = lib.new ("(.)b.(d)")
  local subj = string.rep ("abcd", 10)
  local rep, n
  -------- 1:  simple case
  rep = 10
  local decr = function (m, t)
    if m == "abcd" and fw.eq (t, {"a","d"}) then rep = rep - 1 end
  end
  r:oldgmatch (subj, decr)
  if rep ~= 0 then print ("  FAIL") return end
  -------- 2:  limiting number of matches in advance
  rep, n = 10, 4
  r:oldgmatch (subj, decr, n)
  if rep + n ~= 10 then print ("  FAIL") return end
  -------- 3:  break iterations from the callback
  rep = 10
  local f2 = function (m, t)
    decr (m, t)
    if rep == 3 then return true end
  end
  r:oldgmatch (subj, f2)
  if rep ~= 3 then print ("  FAIL") return end
end

local function test_library (libname)

  local lib = require (libname)
  lib:flags()

  local N = fw.NT

  local set_f_find = {
    SetName = "Function find",
    FMName = "find",
  --  { subj,  patt,  st, cf, ef }              { results }
    { {"abcd", ".+"},                           { 1,4 }   }, -- [none]
    { {"abcd", ".+",  2},                       { 2,4 }   }, -- positive st
    { {"abcd", ".+", -2},                       { 3,4 }   }, -- negative st
    { {"abcd", ".+",  5},                       { N,lib.NOMATCH }}, -- failing st
    { {"abcd", ".*"},                           { 1,4 }   }, -- [none]
    { {"abc", "aBC",  N, lib.ICASE},            { 1,3 }   }, -- cf
    { {"abc", "bc"},                            { 2,3 }   }, -- [none]
  --{ {"abc", "bc",      N, lib.ANCHORED},      { N,lib.NOMATCH }}, -- cf
  --{ {"abc", "bc",      N, N,N, lib.ANCHORED}, { N,lib.NOMATCH }}, -- ef
    { {"abcd", "(.)b.(d)"},                     { 1,4,"a","d" }},--[captures]
  }

  local set_m_find = {
    SetName = "Method find",
    FMName = "find",
  --  {patt,cf},            {subj,st,ef}             { results }
    { {".+"},               {"abcd"},                { 1,4 }   }, -- [none]
    { {".+"},               {"abcd",2},              { 2,4 }   }, -- positive st
    { {".+"},               {"abcd",-2},             { 3,4 }   }, -- negative st
    { {".+"},               {"abcd",5},              { N,lib.NOMATCH }}, -- failing st
    { {".*"},               {"abcd"},                { 1,4 }   }, -- [none]
    { {"aBC",lib.ICASE},    {"abc"},                 { 1,3 }   }, -- cf
    { {"bc"},               {"abc"},                 { 2,3 }   }, -- [none]
  --{ {"bc",lib.ANCHORED},  {"abc"},                 { N,lib.NOMATCH }}, -- cf
  --{ {"bc"},               {"abc",N, lib.ANCHORED}, { N,lib.NOMATCH }}, -- ef
    { { "(.)b.(d)"},        {"abcd"},                { 1,4,"a","d" }},--[captures]
  }

  local set_f_match = {
    SetName = "Function match",
    FMName = "match",
  --  { subj,  patt,  st, cf, ef }              { results }
    { {"abcd", ".+"},                           {"abcd"}  }, -- [none]
    { {"abcd", ".+",  2},                       { "bcd"}  }, -- positive st
    { {"abcd", ".+", -2},                       { "cd" }  }, -- negative st
    { {"abcd", ".+",  5},                       { N,lib.NOMATCH }}, -- failing st
    { {"abcd", ".*"},                           {"abcd"}  }, -- [none]
    { {"abc", "aBC",  N, lib.ICASE},            {"abc" }  }, -- cf
    { {"abc", "bc",                  },         { "bc" }  }, -- [none]
  --{ {"abc", "bc",   N, lib.ANCHORED},         { N,lib.NOMATCH }}, -- cf
  --{ {"abc", "bc",   N, N,N, lib.ANCHORED},    { N,lib.NOMATCH }}, -- ef
    { {"abcd", "(.)b.(d)"},                     { "a","d" }},--[captures]
  }

  local set_m_match = {
    SetName = "Method match",
    FMName = "match",
  --  {patt,cf},            {subj,st,ef}             { results }
    { {".+"},               {"abcd"},                 {"abcd"}  }, -- [none]
    { {".+"},               {"abcd",2},               { "bcd"}  }, -- positive st
    { {".+"},               {"abcd",-2},              { "cd" }  }, -- negative st
    { {".+"},               {"abcd",5},               { N,lib.NOMATCH }}, -- failing st
    { {".*"},               {"abcd"},                 {"abcd"}  }, -- [none]
    { {"aBC",lib.ICASE},    {"abc"},                  {"abc" }  }, -- cf
    { {"bc"},               {"abc"},                  { "bc" }  }, -- [none]
  --{ {"bc",lib.ANCHORED},  {"abc"},                  { N,lib.NOMATCH }}, -- cf
  --{ {"bc"},               {"abc",N, lib.ANCHORED},  { N,lib.NOMATCH }}, -- ef
    { { "(.)b.(d)"},        {"abcd"},                 { "a","d" }},--[captures]
  }

  local set_m_exec = {
    SetName = "Method exec",
    FMName = "exec",
  --  {patt,cf},            {subj,st,ef}              { results }
    { {".+"},               {"abcd"},                 {1,4,{},0}  }, -- [none]
    { {".+"},               {"abcd",2},               {2,4,{},0}  }, -- positive st
    { {".+"},               {"abcd",-2},              {3,4,{},0}  }, -- negative st
    { {".+"},               {"abcd",5},               { N,lib.NOMATCH }}, -- failing st
    { {".*"},               {"abcd"},                 {1,4,{},0}  }, -- [none]
    { {"aBC",lib.ICASE},    {"abc"},                  {1,3,{},0}  }, -- cf
    { {"bc"},               {"abc"},                  {2,3,{},0}  }, -- [none]
  --{ {"bc",lib.ANCHORED},  {"abc"},                  { N,lib.NOMATCH }}, -- cf
  --{ {"bc"},               {"abc",N, lib.ANCHORED},  { N,lib.NOMATCH }}, -- ef
    { { "(.)b.(d)"},        {"abcd"},                 {1,4,{1,1,4,4},0}},--[captures]
  }

  local set_m_oldmatch = {
    SetName = "Method oldmatch",
    FMName = "oldmatch",
  --  {patt,cf},            {subj,st,ef}              { results }
    { {".+"},               {"abcd"},                 {1,4,{},0}  }, -- [none]
    { {".+"},               {"abcd",2},               {2,4,{},0}  }, -- positive st
    { {".+"},               {"abcd",-2},              {3,4,{},0}  }, -- negative st
    { {".+"},               {"abcd",5},               { N,lib.NOMATCH }}, -- failing st
    { {".*"},               {"abcd"},                 {1,4,{},0}  }, -- [none]
    { {"aBC",lib.ICASE},    {"abc"},                  {1,3,{},0}  }, -- cf
    { {"bc"},               {"abc"},                  {2,3,{},0}  }, -- [none]
  --{ {"bc",lib.ANCHORED},  {"abc"},                  { N,lib.NOMATCH }}, -- cf
  --{ {"bc"},               {"abc",N, lib.ANCHORED},  { N,lib.NOMATCH }}, -- ef
    { { "(.)b.(d)"},        {"abcd"},                 {1,4,{"a","d"},0}}, --[captures]
  }

  local set_f_plainfind = {
    SetName = "Function plainfind",
    FMName = "plainfind",
  --  { subj,  patt,  st, ci }    { results }
    { {"abcd", "bc"},             {2,3}  }, -- [none]
    { {"abcd", "cd"},             {3,4}  }, -- positive st
    { {"abcd", "cd", 3},          {3,4}  }, -- positive st
    { {"abcd", "cd", 4},          {N}  },   -- failing st
    { {"abcd", "bc", 2},          {2,3}  }, -- positive st
    { {"abcd", "bc", -4},         {2,3}  }, -- negative st
    { {"abcd", "bc", 3},          {N}  },   -- failing st
    { {"abcd", "BC", N, true},    {2,3}  }, -- case insensitive
    { {"ab\000cd", "b\000c"},     {2,4}  }, -- contains nul
  }

  do
    fw.test_func (lib, set_f_match)
    fw.test_func (lib, set_f_find)
    fw.test_func (lib, set_f_plainfind)
    test_func_gmatch (lib)

    fw.test_method (lib, set_m_match)
    fw.test_method (lib, set_m_find)
    fw.test_method (lib, set_m_exec)
    fw.test_method (lib, set_m_oldmatch)
    test_method_gmatch (lib)
    test_method_oldgmatch (lib)
    print ""
  end
end

test_library ("rex_posix1")
test_library ("rex_posix2")
test_library ("rex_pcreposix")
