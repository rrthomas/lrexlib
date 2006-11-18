-- [ Shmuel Zeigerman; Nov-Nov 2006 ]

local pcre = require "rex_pcre"
local posix = require "rex_posix1"
pcre:flags()
posix:flags()

-- deep table comparison
local function eq (t1, t2)
  assert (type(t1) == "table")
  assert (type(t2) == "table")
  if t1 == t2 then return true end
  if #t1 ~= #t2 then return false end
  for k,v in ipairs (t1) do
    if type(t2[k])=="table" and type(v)=="table" then
      if not eq (t2[k], v) then return false end -- recursion
    else
      if t2[k] ~= v then return false end
    end
  end
  return true
end

local NT = {} -- a unique "nil table", to be used instead of nils in datasets

-- pack vararg in table, replacing nils with "NT" table
local function packNT (...)
  local t = {}
  for i=1, select ("#", ...) do
    local v = select (i, ...)
    if v == nil then v = NT end
    t[i] = v
  end
  return t
end

-- unpack table into vararg, replacing "NT" items with nils
local function unpackNT (t)
  local len = #t
  local function unpack_from (i)
    local v = t[i]
    if v == NT then v = nil end
    if i == len then return v end
    return v, unpack_from (i+1)
  end
  if len > 0 then return unpack_from (1) end
end

local function testfunc (func, par)
  local ptype = type (par)
  return ptype == nil and packNT (func ())
    or ptype == "table" and packNT (func (unpackNT (par)))
    or packNT (func (par))
end

local function testmethod (r, name, par)
  local ptype = type (par)
  return ptype == "nil" and packNT (r[name] (r))
    or ptype == "table" and packNT (r[name] (r, unpackNT (par)))
    or packNT(r[name] (r, par))
end

local set1 = {
  name = "Set1",
  { pcre.find,     {"abcd", ".+"},  { 1, 4 }   },
  { pcre.match,    {"abcd", ".+"},  { "abcd" } },
  { pcre.find,     {"abcd", ".*"},  { 1, 4 }   },
  { pcre.match,    {"abcd", ".*"},  { "abcd" } },
  { pcre.find,     {"abcd", ".*?"}, { 1, 0 }   },
  { pcre.match,    {"abcd", ".*?"}, { "" }     },
  { posix.find,     {"abcd", ".+"},  { 1, 4 }   },
  { posix.match,    {"abcd", ".+"},  { "abcd" } },
  { posix.find,     {"abcd", ".*"},  { 1, 4 }   },
  { posix.match,    {"abcd", ".*"},  { "abcd" } },
  --{ posix.find,     {"abcd", ".*?"}, { 1, 0 }   },
  --{ posix.match,    {"abcd", ".*?"}, { "" }     },
}

local function test (set)
  print (set.name or "Unnamed set")
  for i,v in ipairs (set) do
    assert (type(v) == "table")
    assert (type(v[1]) == "function")
    if not eq (testfunc (v[1], v[2]), v[3]) then
      print ("  Test " .. i)
    end
  end
end

test (set1)


