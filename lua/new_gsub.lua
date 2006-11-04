rex = require "rex_pcre" -- global

rex:flags() -- add all flags to rex namespace; works with Lrexlib v1.20 or higher

-- Bitwise OR emulation
-- (TODO: should be done from the C-part of the library)
local function bor (...)
  local res, tb = 0, {...}
  for _, v in ipairs(tb) do
    res = (math.floor (res / v) % 2 == 1) and res or (res + v)
  end
  return res
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
function rex.gsub (s, p, f, n, cf, lo, ef)
  if type (f) == "string" then
    local rep = f
    f = function (...)
          local arg = {...}
          local ret = rep
          local function repfun (percent, d)
            if #percent % 2 == 1 then
              d = tonumber(d)
              d = arg [d == 0 and 1 or d]
              assert (d ~= nil, "invalid capture index")
              d = d or "" -- capture can be false
              percent = string.sub (percent, 2)
            end
            return percent .. d
          end
          ret = string.gsub (ret, "(%%+)([0-9])", repfun)
          ret = string.gsub (ret, "%%(.)", "%1")
          return ret
        end
  elseif type (f) == "table" then
    local rep = f
    f = function (s)
          return rep[s]
        end
  end
  local reg = rex.new (p, cf, lo)
  local st = 1
  local r, reps = {}, 0
  local efr = bor (ef or 0, rex.NOTEMPTY, rex.ANCHORED)
  local retry
  while (not n) or reps < n do
    local from, to, cap = reg:match (s, st, retry and efr or ef)
    if from then
      table.insert (r, string.sub (s, st, from - 1))
      if #cap == 0 then
        cap[1] = string.sub (s, from, to)
      end
      local rep = f (unpack (cap))
      if rep then
        local reptype = type (rep)
        if reptype == "string" or reptype == "number" then
          table.insert (r, rep)
          reps = reps + 1
        else
          error ("invalid replacement value (a " .. reptype .. ")")
        end
      else
        table.insert (r, string.sub (s, from, to))
      end
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

