-- [ Shmuel Zeigerman; Nov-Nov 2006 ]

module (..., package.seeall)

-- arrays: deep comparison
function eq (t1, t2, lut)
  if t1 == t2 then return true end
  if #t1 ~= #t2 then return false end

  lut = lut or {} -- look-up table: are these 2 arrays already compared?
  local s1, s2 = tostring (t1), tostring (t2)
  local key = s1 < s2 and s1.."\0"..s2 or s2.."\0"..s1
  if lut[key] then return true end
  lut[key] = true

  for k,v in ipairs (t1) do
    if type(t2[k])=="table" and type(v)=="table" then
      if not eq (t2[k], v, lut) then return false end -- recursion
    else
      if t2[k] ~= v then return false end
    end
  end
  return true
end

NT = {} -- a unique "nil table", to be used instead of nils in datasets

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

local function new (lib, par)
  local ptype = type (par)
  return ptype == nil and lib.new ()
    or ptype == "table" and lib.new (unpackNT (par))
    or lib.new (par)
end

local function run_func (func, par)
  local ptype = type (par)
  return ptype == nil and packNT (func ())
    or ptype == "table" and packNT (func (unpackNT (par)))
    or packNT (func (par))
end

local function run_method (r, name, par)
  local ptype = type (par)
  return ptype == "nil" and packNT (r[name] (r))
    or ptype == "table" and packNT (r[name] (r, unpackNT (par)))
    or packNT(r[name] (r, par))
end

function test_func (lib, set)
  print (set.SetName or "Unnamed set")
  assert (type (set.FMName) == "string")
  local func = lib[set.FMName]
  assert (type(func) == "function")
  for i,v in ipairs (set) do
    assert (type(v) == "table")
    if not eq (run_func (func, v[1]), v[2]) then
      print ("  Test " .. i)
    end
  end
end

function test_method (lib, set)
  print (set.SetName or "Unnamed set")
  assert (type (set.FMName) == "string")
  for i,v in ipairs (set) do
    assert (type(v) == "table")
    local r = new (lib, v[1])
    if not eq (run_method (r, set.FMName, v[2]), v[3]) then
      print ("  Test " .. i)
    end
  end
end

