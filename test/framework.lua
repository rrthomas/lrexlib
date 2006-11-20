-- [ Shmuel Zeigerman; Nov-Nov 2006 ]

module ("framework", package.seeall)

-- deep table comparison
function eq (t1, t2)
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

function new (lib, par)
  local ptype = type (par)
  return ptype == nil and lib["new"] ()
    or ptype == "table" and lib["new"] (unpackNT (par))
    or lib["new"] (par)
end

function run_func (func, par)
  local ptype = type (par)
  return ptype == nil and packNT (func ())
    or ptype == "table" and packNT (func (unpackNT (par)))
    or packNT (func (par))
end

function run_method (r, name, par)
  local ptype = type (par)
  return ptype == "nil" and packNT (r[name] (r))
    or ptype == "table" and packNT (r[name] (r, unpackNT (par)))
    or packNT(r[name] (r, par))
end

