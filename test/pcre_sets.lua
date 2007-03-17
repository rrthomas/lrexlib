-- See Copyright Notice in the file LICENSE

local luatest = require "luatest"
local N = luatest.NT

local function norm(a) return a==nil and N or a end

local function fill (n, m)
  local t = {}
  for i = n, m, -1 do table.insert (t, i) end
  return t
end

local function set_named_subpatterns (lib, flg)
  return {
    Name = "Named Subpatterns",
    Func = function (methodname, subj, patt, name1, name2)
      local r = lib.new (patt)
      local _,_,caps = r[methodname] (r, subj)
      return norm(caps[name1]), norm(caps[name2])
    end,
    --{}
    { {"tfind", "abcd", "(?P<dog>.)b.(?P<cat>d)", "dog", "cat"},  {"a","d"} },
    { {"exec",  "abcd", "(?P<dog>.)b.(?P<cat>d)", "dog", "cat"},  {"a","d"} },
  }
end

local function set_f_find (lib, flg)
  return {
  Name = "Function find",
  Func = lib.find,
  --{subj,   patt,      st,cf,ef,lo},        { results }
  { {"abcd", ".+",      5},                  { N   } }, -- failing st
  { {"abcd", ".*?"},                         { 1,0 } }, -- non-greedy
  { {"abc",  "aBC",     N,flg.CASELESS},     { 1,3 } }, -- cf
  { {"abc",  "bc",      N,N,flg.ANCHORED},   { N   } }, -- cf
  { {"abc",  "bc",      N,N,flg.ANCHORED},   { N   } }, -- ef
}
end

local function set_f_match (lib, flg)
  return {
  Name = "Function match",
  Func = lib.match,
  --{subj,   patt,      st,cf,ef,lo},        { results }
  { {"abcd", ".+",      5},                  { N    }}, -- failing st
  { {"abcd", ".*?"},                         { ""   }}, -- non-greedy
  { {"abc",  "aBC",     N,flg.CASELESS},     {"abc" }}, -- cf
  { {"abc",  "bc",      N,N,flg.ANCHORED},   { N    }}, -- cf
  { {"abc",  "bc",      N,N,flg.ANCHORED},   { N    }}, -- ef
}
end

local function set_m_exec (lib, flg)
  return {
  Name = "Method exec",
  Method = "exec",
--{patt,cf,lo},           {subj,st,ef}              { results }
  { {".+"},               {"abcd",5},               { N }    }, -- failing st
  { {".*?"},              {"abcd"},                 {1,0,{}} }, -- non-greedy
  { {"aBC",flg.CASELESS}, {"abc"},                  {1,3,{}} }, -- cf
  { {"bc",flg.ANCHORED},  {"abc"},                  { N }    }, -- cf
  { {"bc"},               {"abc",N, flg.ANCHORED},  { N }    }, -- ef
}
end

local function set_m_tfind (lib, flg)
  return {
  Name = "Method tfind",
  Method = "tfind",
--{patt,cf,lo},           {subj,st,ef}              { results }
  { {".+"},               {"abcd",5},               { N }    }, -- failing st
  { {".*?"},              {"abcd"},                 {1,0,{}} }, -- non-greedy
  { {"aBC",flg.CASELESS}, {"abc"},                  {1,3,{}} }, -- cf
  { {"bc",flg.ANCHORED},  {"abc"},                  { N }    }, -- cf
  { {"bc"},               {"abc",N, flg.ANCHORED},  { N }    }, -- ef
}
end

local function set_m_dfa_exec (lib, flg)
  return {
  Name = "Method dfa_exec",
  Method = "dfa_exec",
--{patt,cf,lo},           {subj,st,ef,os,ws}        { results }
  { {".+"},               {"abcd"},                 {1,{4,3,2,1},4} }, -- [none]
  { {".+"},               {"abcd",2},               {2,{4,3,2},  3} }, -- positive st
  { {".+"},               {"abcd",-2},              {3,{4,3},    2} }, -- negative st
  { {".+"},               {"abcd",5},               {N }            }, -- failing st
  { {".*"},               {"abcd"},                 {1,{4,3,2,1,0},5}}, -- [none]
  { {".*?"},              {"abcd"},                 {1,{4,3,2,1,0},5}}, -- non-greedy
  { {"aBC",flg.CASELESS}, {"abc"},                  {1,{3},1}  }, -- cf
  { {"bc"},               {"abc"},                  {2,{3},1}  }, -- [none]
  { {"bc",flg.ANCHORED},  {"abc"},                  {N }       }, -- cf
  { {"bc"},               {"abc",N, flg.ANCHORED},  {N }       }, -- ef
  { { "(.)b.(d)"},        {"abcd"},                 {1,{4},1}  }, --[captures]
  { {"abc"},              {"ab"},                   {N }       },
  { {"abc"},              {"ab",N,flg.PARTIAL},     {1,{2},flg.ERROR_PARTIAL} },
  { {".+"},     {string.rep("a",50),N,N,50,50},     {1, fill(50,26), 0}},-- small ovecsize
}
end

return function (libname)
  local lib = require (libname)
  local flags = lib.flags ()
  local sets = {
    set_f_match  (lib, flags),
    set_f_find   (lib, flags),
    set_m_exec   (lib, flags),
    set_m_tfind  (lib, flags),
  }
  if flags.MAJOR >= 4 then
    table.insert (sets, set_named_subpatterns (lib, flags))
  end
  if flags.MAJOR >= 6 then
    table.insert (sets, set_m_dfa_exec (lib, flags))
  end
  return sets
end
