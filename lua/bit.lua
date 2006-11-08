-- bit.lua
--
local ipairs, select = ipairs, select
local floor = math.floor

module ("bit")

local MAXBITS = 53  --> 53-bit mantissa of a standard 64-bit "double"


-- Bitwise OR emulation
function bor (...)
  local out = {}
  for _, v in ipairs {...} do
    local k = 1
    while v >= 1 do
      if v % 2 == 0 then
        out[k] = out[k] or 0
      else
        out[k] = 1
        v = v - 1
      end
      v = v / 2
      k = k + 1
    end
  end
  local res, k = 0, 1
  for _, v in ipairs (out) do
    res = res + v * k
    k = k * 2
  end
  return res
end


-- Bitwise AND emulation
function band (...)
  local out = {}
  for _, v in ipairs {...} do
    local k = 1
    while v >= 1 do
      if v % 2 == 0 then
        out[k] = 0
      else
        out[k] = out[k]==nil and 1 or out[k]==0 and 0 or out[k]+1
        v = v - 1
      end
      v = v / 2
      k = k + 1
    end
  end
  local res, k = 0, 1
  local narg = select ("#", ...)
  for _, v in ipairs (out) do
    if v == narg then
      res = res + k
    end
    k = k * 2
  end
  return res
end


-- Bitwise XOR emulation
function bxor (...)
  local out = {}
  for _, v in ipairs {...} do
    local k = 1
    while v >= 1 do
      if v % 2 == 0 then
        out[k] = out[k] or 0
      else
        out[k] = out[k] ~= 1 and 1 or 0
        v = v - 1
      end
      v = v / 2
      k = k + 1
    end
  end
  local res, k = 0, 1
  for _, v in ipairs (out) do
    res = res + v * k
    k = k * 2
  end
  return res
end


-- Bitwise NOT emulation
function bnot (v)
  return -v - 1
end


function lshift (v, n)
  return (floor (v * 2^n)) % (2^MAXBITS)
end


function rshift (v, n)
  return (floor (v / 2^n)) % (2^MAXBITS)
end

