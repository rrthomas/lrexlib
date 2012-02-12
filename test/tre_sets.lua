-- See Copyright Notice in the file LICENSE

-- This file should contain only test sets that behave identically
-- when being run with pcre or posix regex libraries.

local luatest = require "luatest"
local N = luatest.NT

local L = function(s) return (string.gsub(s, ".", "%0\0")) end

local function norm(a) return a==nil and N or a end

local function get_wgsub (lib)
  return lib.wgsub or
    function (subj, pattern, repl, n)
      return lib.wnew (pattern) : wgsub (subj, repl, n)
    end
end

local function set_f_wgmatch (lib, flg)
  -- gmatch (s, p, [cf], [ef])
  local function test_wgmatch (subj, patt)
    local out, guard = {}, 10
    for a, b in lib.wgmatch (subj, patt) do
      table.insert (out, { norm(a), norm(b) })
      guard = guard - 1
      if guard == 0 then break end
    end
    return unpack (out)
  end
  return {
    Name = "Function wgmatch",
    Func = test_wgmatch,
  --{  subj              patt             results }
    { {L"ab",            lib.wnew(L".")}, {{L"a",N}, {L"b",N} } },
    { {(L"abcd"):rep(3), L"(.)b.(d)"},    {{L"a",L"d"},{L"a",L"d"},{L"a",L"d"}} },
    { {L"abcd",          L".*" },         {{L"abcd",N},{L"",N}  } },--zero-length match
    { {L"abc",           L"^." },         {{L"a",N}} },--anchored pattern
  }
end

local function set_f_wsplit (lib, flg)
  -- split (s, p, [cf], [ef])
  local function test_wsplit (subj, patt)
    local out, guard = {}, 10
    for a, b, c in lib.wsplit (subj, patt) do
      table.insert (out, { norm(a), norm(b), norm(c) })
      guard = guard - 1
      if guard == 0 then break end
    end
    return unpack (out)
  end
  return {
    Name = "Function wsplit",
    Func = test_wsplit,
  --{  subj              patt       results }
    { {L"ab",     lib.wnew(L",")},  {{L"ab",N,N},                           } },
    { {L"ab",            L","},     {{L"ab",N,N},                           } },
    { {L",",             L","},     {{L"",L",",N},     {L"", N, N},           } },
    { {L",,",            L","},     {{L"",L",",N},     {L"",L",",N},  {L"",N,N} } },
    { {L"a,b",           L","},     {{L"a",L",",N},    {L"b",N,N},            } },
    { {L",a,b",          L","},     {{L"",L",",N},     {L"a",L",",N}, {L"b",N,N}} },
    { {L"a,b,",          L","},     {{L"a",L",",N},    {L"b",L",",N}, {L"",N,N} } },
    { {L"a,,b",          L","},     {{L"a",L",",N},    {L"",L",",N},  {L"b",N,N}} },
    { {L"ab<78>c", L"<(.)(.)>"},    {{L"ab",L"7",L"8"}, {L"c",N,N},            } },
    { {L"abc",           L"^."},    {{L"", L"a",N},    {L"bc",N,N},           } },--anchored pattern
    { {L"abc",           L"^"},     {{L"", L"", N},    {L"abc",N,N},          } },
--  { {"abc",           "$"},     {{"abc","",N},   {"",N,N},             } },
--  { {"abc",         "^|$"},     {{"", "", N},    {"abc","",N},{"",N,N},} },
  }
end

local function set_f_wfind (lib, flg)
  return {
    Name = "Function wfind",
    Func = lib.wfind,
  --  {subj, patt, st},         { results }
    { {L"abcd", lib.wnew(L".+")}, { 1,4 }   },      -- [none]
    { {L"abcd", L".+"},           { 1,4 }   },      -- [none]
    { {L"abcd", L".+", 2},        { 2,4 }   },      -- positive st
    { {L"abcd", L".+", -2},       { 3,4 }   },      -- negative st
    { {L"abcd", L".*"},           { 1,4 }   },      -- [none]
    { {L"abc",  L"bc"},           { 2,3 }   },      -- [none]
    { {L"abcd", L"(.)b.(d)"},     { 1,4,L"a",L"d" }}, -- [captures]
  }
end

local function set_f_wmatch (lib, flg)
  return {
    Name = "Function wmatch",
    Func = lib.wmatch,
  --  {subj, patt, st},           { results }
    { {L"abcd", lib.wnew(L".+")}, {L"abcd"}  }, -- [none]
    { {L"abcd", L".+"},           {L"abcd"}  }, -- [none]
    { {L"abcd", L".+", 2},        {L"bcd"}   }, -- positive st
    { {L"abcd", L".+", -2},       {L"cd"}    }, -- negative st
    { {L"abcd", L".*"},           {L"abcd"}  }, -- [none]
    { {L"abc",  L"bc"},           {L"bc"}    }, -- [none]
    { {L"abcd", L"(.)b.(d)"},     {L"a",L"d"} }, -- [captures]
  }
end

local function set_m_wexec (lib, flg)
  return {
    Name = "Method wexec",
    Method = "wexec",
  --{patt},                 {subj, st}           { results }
    { {L".+"},              {L"abcd"},            {1,4,{}}  }, -- [none]
    { {L".+"},              {L"abcd",2},          {2,4,{}}  }, -- positive st
    { {L".+"},              {L"abcd",-2},         {3,4,{}}  }, -- negative st
    { {L".*"},              {L"abcd"},            {1,4,{}}  }, -- [none]
    { {L"bc"},              {L"abc"},             {2,3,{}}  }, -- [none]
    { {L "(.)b.(d)"},       {L"abcd"},            {1,4,{1,1,4,4}}},--[captures]
    { {L"(a+)6+(b+)"},      {L"Taa66bbT",2},      {2,7,{2,3,6,7}}},--[st+captures]
  }
end

local function set_m_wtfind (lib, flg)
  return {
    Name = "Method wtfind",
    Method = "wtfind",
  --{patt},                 {subj, st}           { results }
    { {L".+"},              {L"abcd"},           {1,4,{}}  }, -- [none]
    { {L".+"},              {L"abcd",2},         {2,4,{}}  }, -- positive st
    { {L".+"},              {L"abcd",-2},        {3,4,{}}  }, -- negative st
    { {L".*"},              {L"abcd"},           {1,4,{}}  }, -- [none]
    { {L"bc"},              {L"abc"},            {2,3,{}}  }, -- [none]
    { {L"(.)b.(d)"},        {L"abcd"},           {1,4,{L"a",L"d"}}},--[captures]
  }
end

local function set_m_wfind (lib, flg)
  return {
    Name = "Method wfind",
    Method = "wfind",
  --{patt},                 {subj, st}           { results }
    { {L".+"},              {L"abcd"},           {1,4}  }, -- [none]
    { {L".+"},              {L"abcd",2},         {2,4}  }, -- positive st
    { {L".+"},              {L"abcd",-2},        {3,4}  }, -- negative st
    { {L".*"},              {L"abcd"},           {1,4}  }, -- [none]
    { {L"bc"},              {L"abc"},            {2,3}  }, -- [none]
    { {L"(.)b.(d)"},        {L"abcd"},           {1,4,L"a",L"d"}},--[captures]
  }
end

local function set_m_wmatch (lib, flg)
  return {
    Name = "Method wmatch",
    Method = "wmatch",
  --{patt},                 {subj, st}           { results }
    { {L".+"},              {L"abcd"},           {L"abcd"}  }, -- [none]
    { {L".+"},              {L"abcd",2},         {L"bcd" }  }, -- positive st
    { {L".+"},              {L"abcd",-2},        {L"cd"  }  }, -- negative st
    { {L".*"},              {L"abcd"},           {L"abcd"}  }, -- [none]
    { {L"bc"},              {L"abc"},            {L"bc"  }  }, -- [none]
    {{ L"(.)b.(d)"},        {L"abcd"},           {L"a",L"d"} }, --[captures]
  }
end

local function set_f_plainfind (lib, flg)
  return {
    Name = "Function plainfind",
    Func = lib.plainfind,
  --{ subj,  patt,  st, ci }      { results }
    { {"abcd", "bc"},             {2,3} }, -- [none]
    { {"abcd", "dc"},             {N}   }, -- [none]
    { {"abcd", "cd"},             {3,4} }, -- positive st
    { {"abcd", "cd", 3},          {3,4} }, -- positive st
    { {"abcd", "cd", 4},          {N}   }, -- failing st
    { {"abcd", "bc", 2},          {2,3} }, -- positive st
    { {"abcd", "bc", -4},         {2,3} }, -- negative st
    { {"abcd", "bc", 3},          {N}   }, -- failing st
    { {"abcd", "BC"},             {N}   }, -- case sensitive
    { {"abcd", "BC", N, true},    {2,3} }, -- case insensitive
    { {"ab\0cd", "b\0c"},         {2,4} }, -- contains nul
    { {"abcd", "", 1},            {1,0} }, -- empty pattern
    { {"abcd", "", 5},            {5,4} }, -- empty pattern
    { {"abcd", "", 6},            {N}   }, -- empty pattern
  }
end

local function set_f_wgsub1 (lib, flg)
  local subj, pat = L"abcdef", L"[abef]+"
  local cpat = lib.wnew(pat)
  return {
    Name = "Function wgsub, set1",
    Func = get_wgsub (lib),
  --{ s,       p,    f,   n,    res1,  res2, res3 },
    { {subj,  cpat,  L"",  0},   {subj,     0, 0} }, -- test "n" + empty_replace
    { {subj,   pat,  L"",  0},   {subj,     0, 0} }, -- test "n" + empty_replace
    { {subj,   pat,  L"", -1},   {subj,     0, 0} }, -- test "n" + empty_replace
    { {subj,   pat,  L"",  1},   {L"cdef",  1, 1} },
    { {subj,   pat,  L"",  2},   {L"cd",    2, 2} },
    { {subj,   pat,  L"",  3},   {L"cd",    2, 2} },
    { {subj,   pat,  L""    },   {L"cd",    2, 2} },
    { {subj,   pat,  L"#", 0},   {subj,     0, 0} }, -- test "n" + non-empty_replace
    { {subj,   pat,  L"#", 1},   {L"#cdef", 1, 1} },
    { {subj,   pat,  L"#", 2},   {L"#cd#",  2, 2} },
    { {subj,   pat,  L"#", 3},   {L"#cd#",  2, 2} },
    { {subj,   pat,  L"#"   },   {L"#cd#",  2, 2} },
    { {L"abc", L"^.",L"#"   },   {L"#bc",   1, 1} }, -- anchored pattern
  }
end

local function set_f_wgsub2 (lib, flg)
  local subj, pat = L"abc", L"([ac])"
  return {
    Name = "Function wgsub, set2",
    Func = get_wgsub (lib),
  --{ s,     p,   f,   n,     res1,    res2, res3 },
    { {subj, pat, L"<%1>" },   {L"<a>b<c>", 2, 2} }, -- test non-escaped chars in f
    { {subj, pat, L"%<%1%>" }, {L"<a>b<c>", 2, 2} }, -- test escaped chars in f
    { {subj, pat, L"" },       {L"b",       2, 2} }, -- test empty replace
    { {subj, pat, L"1" },      {L"1b1",     2, 2} }, -- test odd and even %'s in f
    { {subj, pat, L"%1" },     {L"abc",     2, 2} },
    { {subj, pat, L"%%1" },    {L"%1b%1",   2, 2} },
    { {subj, pat, L"%%%1" },   {L"%ab%c",   2, 2} },
    { {subj, pat, L"%%%%1" },  {L"%%1b%%1", 2, 2} },
    { {subj, pat, L"%%%%%1" }, {L"%%ab%%c", 2, 2} },
  }
end

local function set_f_wgsub3 (lib, flg)
  return {
    Name = "Function wgsub, set3",
    Func = get_wgsub (lib),
  --{ s,      p,      f,  n,   res1,res2,res3 },
    { {L"abc", L"a",    "%0" }, {"abc", 1, 1} }, -- test (in)valid capture index
    { {L"abc", L"a",    "%1" }, {"abc", 1, 1} },
    { {L"abc", L"[ac]", "%1" }, {"abc", 2, 2} },
    { {L"abc", L"(a)",  "%1" }, {"abc", 1, 1} },
    { {L"abc", L"(a)",  "%2" }, "invalid capture index" },
  }
end

local function set_f_wgsub4 (lib, flg)
  return {
    Name = "Function wgsub, set4",
    Func = get_wgsub (lib),
  --{ s,           p,              f, n,  res1,      res2, res3 },
    { {"a2c3",     ".",            "#" }, {"####",      4, 4} }, -- test .
    { {"a2c3",     ".+",           "#" }, {"#",         1, 1} }, -- test .+
    { {"a2c3",     ".*",           "#" }, {"##",        2, 2} }, -- test .*
    { {"/* */ */", "\\/\\*(.*)\\*\\/", "#" }, {"#",     1, 1} },
    { {"a2c3",     "[0-9]",        "#" }, {"a#c#",      2, 2} }, -- test %d
    { {"a2c3",     "[^0-9]",       "#" }, {"#2#3",      2, 2} }, -- test %D
    { {"a \t\nb",  "[ \t\n]",      "#" }, {"a###b",     3, 3} }, -- test %s
    { {"a \t\nb",  "[^ \t\n]",     "#" }, {"# \t\n#",   2, 2} }, -- test %S
  }
end

local function set_f_wgsub5 (lib, flg)
  local function frep1 () end                       -- returns nothing
  local function frep2 () return "#" end            -- ignores arguments
  local function frep3 (...) return table.concat({...}, ",") end -- "normal"
  local function frep4 () return {} end             -- invalid return type
  local function frep5 () return "7", "a" end       -- 2-nd return is "a"
  local function frep6 () return "7", "break" end   -- 2-nd return is "break"
  local subj = "a2c3"
  return {
    Name = "Function wgsub, set5",
    Func = get_wgsub (lib),
  --{ s,     p,          f,   n,   res1,     res2, res3 },
    { {subj, "a(.)c(.)", frep1 }, {subj,        1, 0} },
    { {subj, "a(.)c(.)", frep2 }, {"#",         1, 1} },
    { {subj, "a(.)c(.)", frep3 }, {"2,3",       1, 1} },
    { {subj, "a.c.",     frep3 }, {subj,        1, 1} },
    { {subj, "z*",       frep1 }, {subj,        5, 0} },
    { {subj, "z*",       frep2 }, {"#a#2#c#3#", 5, 5} },
    { {subj, "z*",       frep3 }, {subj,        5, 5} },
    { {subj, subj,       frep4 }, "invalid return type" },
    { {"abc",".",        frep5 }, {"777",       3, 3} },
    { {"abc",".",        frep6 }, {"777",       3, 3} },
  }
end

local function set_f_wgsub6 (lib, flg)
  local tab1, tab2, tab3 = {}, { ["2"] = 56 }, { ["2"] = {} }
  local subj = "a2c3"
  return {
    Name = "Function wgsub, set6",
    Func = get_wgsub (lib),
  --{ s,     p,          f, n,   res1,res2,res3 },
    { {subj, "a(.)c(.)", tab1 }, {subj,  1, 0} },
    { {subj, "a(.)c(.)", tab2 }, {"56",  1, 1} },
    { {subj, "a(.)c(.)", tab3 }, "invalid replacement type" },
    { {subj, "a.c.",     tab1 }, {subj,  1, 0} },
    { {subj, "a.c.",     tab2 }, {subj,  1, 0} },
    { {subj, "a.c.",     tab3 }, {subj,  1, 0} },
  }
end

local function set_f_wgsub8 (lib, flg)
  local subj, patt, repl = "abcdef", "..", "*"
  return {
    Name = "Function wgsub, set8",
    Func = get_wgsub (lib),
  --{ s,     p,       f, n,                                    res1,  res2, res3 },
    { {subj, patt, repl, function() end },                    {"abcdef", 3, 0} },
    { {subj, patt, repl, function() return nil end },         {"abcdef", 3, 0} },
    { {subj, patt, repl, function() return false end },       {"abcdef", 3, 0} },
    { {subj, patt, repl, function() return true end },        {"***",    3, 3} },
    { {subj, patt, repl, function() return {} end },          {"***",    3, 3} },
    { {subj, patt, repl, function() return "#" end },         {"###",    3, 3} },
    { {subj, patt, repl, function() return 57 end },          {"575757", 3, 3} },
    { {subj, patt, repl, function (from) return from end },   {"135",    3, 3} },
    { {subj, patt, repl, function (from, to) return to end }, {"246",    3, 3} },
    { {subj, patt, repl, function (from,to,rep) return rep end },
                                                              {"***",    3, 3} },
    { {subj, patt, repl, function (from, to, rep) return rep..to..from end },
                                                           {"*21*43*65", 3, 3} },
    { {subj, patt, repl, function() return nil end },         {"abcdef", 3, 0} },
    { {subj, patt, repl, function() return nil, nil end },    {"abcdef", 3, 0} },
    { {subj, patt, repl, function() return nil, false end },  {"abcdef", 3, 0} },
    { {subj, patt, repl, function() return nil, true end },   {"ab**",   3, 2} },
    { {subj, patt, repl, function() return true, true end },  {"***",    3, 3} },
    { {subj, patt, repl, function() return nil, 0 end },      {"abcdef", 1, 0} },
    { {subj, patt, repl, function() return true, 0 end },     {"*cdef",  1, 1} },
    { {subj, patt, repl, function() return nil, 1 end },      {"ab*ef",  2, 1} },
    { {subj, patt, repl, function() return true, 1 end },     {"**ef",   2, 2} },
  }
end

return function (libname)
  local lib = require (libname)
  lib.new = lib.wnew
  return {
    set_f_wgmatch    (lib),
    set_f_wsplit     (lib),
    set_f_wfind      (lib),
    set_f_wmatch     (lib),
    set_m_wexec      (lib),
    set_m_wtfind     (lib),
    set_m_wfind      (lib),
    set_m_wmatch     (lib),
    set_f_wgsub1     (lib),
    --set_f_wgsub2     (lib),
    --set_f_wgsub3     (lib),
    --set_f_wgsub4     (lib),
    --set_f_wgsub5     (lib),
    --set_f_wgsub6     (lib),
    --set_f_wgsub8     (lib),
    --set_f_plainfind (lib),
  }
end
