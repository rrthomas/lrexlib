rex = require "rex_pcre"

-- Default constructor (use PCREs)
setmetatable (rex, {__call =
                function (self, p, cf, lo)
                  return self.newPCRE (p, cf, lo)
                end})

function rex.find (s, p, st, cf, lo, ef)
  local from, to, cap = rex (p, cf, lo):match (s, st, ef)
  if from and (cap[1] ~= nil) then
    return from, to, unpack (cap)
  end
  return from, to
end

