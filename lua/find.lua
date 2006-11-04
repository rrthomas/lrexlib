rex = require "rex_pcre"

function rex.find (s, p, st, cf, lo, ef)
  local from, to, cap = rex.new (p, cf, lo):match (s, st, ef)
  if from and (cap[1] ~= nil) then
    return from, to, unpack (cap)
  end
  return from, to
end

