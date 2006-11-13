-- @module rex
-- Regular expressions library

-- The rex module provides an interface to the lrexlib POSIX and PCRE
-- regular expression support that mimics the standard string library.
-- It provides find and gsub functions, which as far as possible are
-- compatible with their string library equivalents.

-- TODO: Allow a default regex library to be installed (Lua, POSIX or PCRE)
rex = require "rex_pcre" -- global!
module ("rex", package.seeall)
-- Use bitlib, from http://luaforge.net/projects/bitlib.
local bit = require "bit"

_M:flags () -- add flags to rex namespace

-- @func find: string.find with regexs
--   @param s: string to search
--   @param p: pattern to find
--   @param [st]: start position for match
--   @param [cf]: compile-time flags for the regex
--   @param [lo]: locale for the regex
--   @param [ef]: execution flags for the regex
-- @returns
--   @param from, to: start and end points of match, or nil
--   @param [c1, ...]: captures
function find (s, p, st, cf, lo, ef)
  local from, to, cap = _M.new (p, cf, lo):match (s, st, ef)
  if from and cap[1] ~= nil then
    return from, to, unpack (cap)
  end
  return from, to
end

-- @func gsub: string.gsub for rex
--   @param s: string to search
--   @param p: pattern to find
--   @param f: replacement function or string
--   @param [n]: maximum number of replacements [all]
--   @param [cf]: compile-time flags for the regex
--   @param [lo]: locale for the regex
--   @param [ef]: execution flags for the regex
-- @returns
--   @param r: string with replacements
--   @param reps: number of replacements made
function gsub (s, p, f, n, cf, lo, ef)
  local rep = f
  local reptype = type (rep)
  if reptype == "string" then
    f = function (cap)
          local function repfun (d)
            local n = tonumber (d)
            if n then
              d = cap [n == 0 and 1 or n]
              assert (d ~= nil, "invalid capture index")
              return d or "" -- capture can be false
            end
            return d
          end
          return (string.gsub (rep, "%%(.)", repfun))
        end
  elseif reptype == "table" then
    f = function (cap)
          return rep[cap[1]]
        end
  elseif reptype == "function" then
    f = function (cap)
          return rep (unpack (cap))
        end
  else
    error ("argument #3 must be string, table or function")
  end
  local reg = _M.new (p, cf, lo)
  local st = 1
  local r, reps = {}, 0
  local efr = bit.bor (ef or 0, NOTEMPTY, ANCHORED)
  local retry
  while (not n) or reps < n do
    local from, to, cap = reg:match (s, st, retry and efr or ef)
    if from then
      table.insert (r, string.sub (s, st, from - 1))
      if #cap == 0 then
        cap[1] = string.sub (s, from, to)
      end
      local rep = f (cap)
      if rep then
        local reptype = type (rep)
        if reptype == "string" or reptype == "number" then
          table.insert (r, rep)
        else
          error ("invalid replacement value (a " .. reptype .. ")")
        end
      else
        table.insert (r, string.sub (s, from, to))
      end
      reps = reps + 1
      if from <= to then
        retry = false
        st = to + 1
      elseif st <= #s then -- retry from the matching point
        retry = true
        st = from
      else
        break
      end
    else
      if retry and st <= #s then -- advance by 1 char (not replaced)
        table.insert (r, string.sub (s, st, st))
        st = st + 1
        retry = false
      else
        break
      end
    end
  end
  table.insert (r, string.sub (s, st))
  return table.concat (r), reps
end


-- Tests
if type (_DEBUG) == "table" and _DEBUG.std then
  dofile "gsub_test.lua"
end


-- TODO: @func check{Posix,PCRE}Regex: check POSIX regex is valid
--   @param p: POSIX regex pattern
-- @returns
--   @param f: true if regex is valid, or nil otherwise
