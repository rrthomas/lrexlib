-- [ Shmuel Zeigerman; Nov-Nov 2006 ]

local fw = require "framework"
local rex = require "rex_posix1"
rex:flags()

local N = fw.NT

local set_f_find = {
  SetName = "Function find",
  FMName = "find",
--  { subj,  patt,  st, cf, ef }              { results }
  { {"abcd", ".+"},                           { 1,4 }   }, -- [none]
  { {"abcd", ".+",  2},                       { 2,4 }   }, -- positive st
  { {"abcd", ".+", -2},                       { 3,4 }   }, -- negative st
  { {"abcd", ".+",  5},                       { N,rex.NOMATCH }}, -- failing st
  { {"abcd", ".*"},                           { 1,4 }   }, -- [none]
  { {"abc", "aBC",  N, rex.ICASE},            { 1,3 }   }, -- cf
  { {"abc", "bc"},                            { 2,3 }   }, -- [none]
--{ {"abc", "bc",      N, rex.ANCHORED},      { N,rex.NOMATCH }}, -- cf
--{ {"abc", "bc",      N, N,N, rex.ANCHORED}, { N,rex.NOMATCH }}, -- ef
  { {"abcd", "(.)b.(d)"},                     { 1,4,"a","d" }},--[captures]
}

local set_m_find = {
  SetName = "Method find",
  FMName = "find",
--  {patt,cf},            {subj,st,ef}             { results }
  { {".+"},               {"abcd"},                { 1,4 }   }, -- [none]
  { {".+"},               {"abcd",2},              { 2,4 }   }, -- positive st
  { {".+"},               {"abcd",-2},             { 3,4 }   }, -- negative st
  { {".+"},               {"abcd",5},              { N,rex.NOMATCH }}, -- failing st
  { {".*"},               {"abcd"},                { 1,4 }   }, -- [none]
  { {"aBC",rex.ICASE},    {"abc"},                 { 1,3 }   }, -- cf
  { {"bc"},               {"abc"},                 { 2,3 }   }, -- [none]
--{ {"bc",rex.ANCHORED},  {"abc"},                 { N,rex.NOMATCH }}, -- cf
--{ {"bc"},               {"abc",N, rex.ANCHORED}, { N,rex.NOMATCH }}, -- ef
  { { "(.)b.(d)"},        {"abcd"},                { 1,4,"a","d" }},--[captures]
}

local set_f_match = {
  SetName = "Function match",
  FMName = "match",
--  { subj,  patt,  st, cf, ef }              { results }
  { {"abcd", ".+"},                           {"abcd"}  }, -- [none]
  { {"abcd", ".+",  2},                       { "bcd"}  }, -- positive st
  { {"abcd", ".+", -2},                       { "cd" }  }, -- negative st
  { {"abcd", ".+",  5},                       { N,rex.NOMATCH }}, -- failing st
  { {"abcd", ".*"},                           {"abcd"}  }, -- [none]
  { {"abc", "aBC",  N, rex.ICASE},            {"abc" }  }, -- cf
  { {"abc", "bc",                  },         { "bc" }  }, -- [none]
--{ {"abc", "bc",   N, rex.ANCHORED},         { N,rex.NOMATCH }}, -- cf
--{ {"abc", "bc",   N, N,N, rex.ANCHORED},    { N,rex.NOMATCH }}, -- ef
  { {"abcd", "(.)b.(d)"},                     { "a","d" }},--[captures]
}

local set_m_match = {
  SetName = "Method match",
  FMName = "match",
--  {patt,cf},            {subj,st,ef}             { results }
  { {".+"},               {"abcd"},                 {"abcd"}  }, -- [none]
  { {".+"},               {"abcd",2},               { "bcd"}  }, -- positive st
  { {".+"},               {"abcd",-2},              { "cd" }  }, -- negative st
  { {".+"},               {"abcd",5},               { N,rex.NOMATCH }}, -- failing st
  { {".*"},               {"abcd"},                 {"abcd"}  }, -- [none]
  { {"aBC",rex.ICASE},    {"abc"},                  {"abc" }  }, -- cf
  { {"bc"},               {"abc"},                  { "bc" }  }, -- [none]
--{ {"bc",rex.ANCHORED},  {"abc"},                  { N,rex.NOMATCH }}, -- cf
--{ {"bc"},               {"abc",N, rex.ANCHORED},  { N,rex.NOMATCH }}, -- ef
  { { "(.)b.(d)"},        {"abcd"},                 { "a","d" }},--[captures]
}

local set_m_exec = {
  SetName = "Method exec",
  FMName = "exec",
--  {patt,cf},            {subj,st,ef}              { results }
  { {".+"},               {"abcd"},                 {1,4,{},0}  }, -- [none]
  { {".+"},               {"abcd",2},               {2,4,{},0}  }, -- positive st
  { {".+"},               {"abcd",-2},              {3,4,{},0}  }, -- negative st
  { {".+"},               {"abcd",5},               { N,rex.NOMATCH }}, -- failing st
  { {".*"},               {"abcd"},                 {1,4,{},0}  }, -- [none]
  { {"aBC",rex.ICASE},    {"abc"},                  {1,3,{},0}  }, -- cf
  { {"bc"},               {"abc"},                  {2,3,{},0}  }, -- [none]
--{ {"bc",rex.ANCHORED},  {"abc"},                  { N,rex.NOMATCH }}, -- cf
--{ {"bc"},               {"abc",N, rex.ANCHORED},  { N,rex.NOMATCH }}, -- ef
  { { "(.)b.(d)"},        {"abcd"},                 {1,4,{1,1,4,4},0}},--[captures]
}

local set_m_oldmatch = {
  SetName = "Method oldmatch",
  FMName = "oldmatch",
--  {patt,cf},            {subj,st,ef}              { results }
  { {".+"},               {"abcd"},                 {1,4,{},0}  }, -- [none]
  { {".+"},               {"abcd",2},               {2,4,{},0}  }, -- positive st
  { {".+"},               {"abcd",-2},              {3,4,{},0}  }, -- negative st
  { {".+"},               {"abcd",5},               { N,rex.NOMATCH }}, -- failing st
  { {".*"},               {"abcd"},                 {1,4,{},0}  }, -- [none]
  { {"aBC",rex.ICASE},    {"abc"},                  {1,3,{},0}  }, -- cf
  { {"bc"},               {"abc"},                  {2,3,{},0}  }, -- [none]
--{ {"bc",rex.ANCHORED},  {"abc"},                  { N,rex.NOMATCH }}, -- cf
--{ {"bc"},               {"abc",N, rex.ANCHORED},  { N,rex.NOMATCH }}, -- ef
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

-- gmatch (s, p, [cf], [ef])
local function test_func_gmatch ()
  print ("Function gmatch")
  local rep = 10
  local subj = string.rep ("abcd", rep)
  for a, b in rex.gmatch (subj, "(.)b.(d)") do
    if a == "a" and b == "d" then rep = rep - 1 end
  end
  if rep ~= 0 then print ("  FAIL") end
end

-- r:gmatch (s, [ef])
local function test_method_gmatch ()
  print ("Method gmatch")
  local rep = 10
  local subj = string.rep ("abcd", rep)
  local r = rex.new ("(.)b.(d)")
  for a, b in r:gmatch (subj) do
    if a == "a" and b == "d" then rep = rep - 1 end
  end
  if rep ~= 0 then print ("  FAIL") end
end

-- r:oldgmatch (s, f, [n], [ef])
local function test_method_oldgmatch ()
  print ("Method oldgmatch")
  local r = rex.new ("(.)b.(d)")
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

local function test_func (set)
  print (set.SetName or "Unnamed set")
  assert (type (set.FMName) == "string")
  local func = rex[set.FMName]
  assert (type(func) == "function")
  for i,v in ipairs (set) do
    assert (type(v) == "table")
    if not fw.eq (fw.run_func (func, v[1]), v[2]) then
      print ("  Test " .. i)
    end
  end
end

local function test_method (set)
  print (set.SetName or "Unnamed set")
  assert (type (set.FMName) == "string")
  for i,v in ipairs (set) do
    assert (type(v) == "table")
    local r = fw.new (rex, v[1])
    if not fw.eq (fw.run_method (r, set.FMName, v[2]), v[3]) then
      print ("  Test " .. i)
    end
  end
end

do
  test_func_gmatch ()
  test_func (set_f_match)
  test_func (set_f_find)
  test_func (set_f_plainfind)

  test_method_gmatch ()
  test_method (set_m_match)
  test_method (set_m_find)
  test_method (set_m_exec)
  test_method (set_m_oldmatch)
  test_method_oldgmatch ()
end

