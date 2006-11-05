local ipairs = ipairs
module ("bit")

-- Bitwise OR emulation
--   TODO: should be done from the C-part of the library.
function bor (...)
  local out, res = {}, 0
  for _, v in ipairs {...} do
    local k = 1
    while v ~= 0 do
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
  local k = 1
  for i,v in ipairs (out) do
    res = res + v * k
    k = k * 2
  end
  return res
end

