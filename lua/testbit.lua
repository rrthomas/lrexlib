require "bit"

local function log (testnum, op)
  print ("test "..testnum.."; operation "..op)
end

local tests = {
--  operand1    operand2    operand3    bor         band        bxor
  { 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000 },
  { 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff },
  { 0x00000000, 0x00000000, 0xffffffff, 0xffffffff, 0x00000000, 0xffffffff },
  { 0x00000000, 0xffffffff, 0xffffffff, 0xffffffff, 0x00000000, 0x00000000 },
  { 0x5a5a5a5a, 0x5a5a5a5a, 0x5a5a5a5a, 0x5a5a5a5a, 0x5a5a5a5a, 0x5a5a5a5a },
  { 0x5a5a5a5a, 0xa5a5a5a5, 0x5a5a5a5a, 0xffffffff, 0x00000000, 0xa5a5a5a5 },
  { 0x1,        0x2,        0x4,        0x7,        0x0,        0x7,       },
  { 0x1,        0x4,        0x4,        0x5,        0x0,        0x1,       },
  { 0x4,        0x4,        0x4,        0x4,        0x4,        0x4,       },

  { 2^40,          0,          0,       2^40,          0,       2^40,      },
  { 2^40,       2^40,          0,       2^40,          0,          0,      },
  { 2^40,       2^40,       2^40,       2^40,       2^40,       2^40,      },
  { 2^53,          0,          0,       2^53,          0,       2^53,      },
  { 2^53,       2^53,          0,       2^53,          0,          0,      },
  { 2^53,       2^53,       2^53,       2^53,       2^53,       2^53,      },

  { 2^40-1,        0,          0,       2^40-1,        0,       2^40-1,    },
  { 2^40-1,     2^40,          0,       2^41-1,        0,       2^41-1,    },
  { 2^40-1,     2^40,       2^40,       2^41-1,        0,       2^40-1,    },
  { 2^53-1,        0,          0,       2^53-1,        0,       2^53-1,    },
  { 2^53-1,     2^53,          0,       2^54-1,        0,       2^54-1,    },
  { 2^53-1,     2^53,       2^53,       2^54-1,        0,       2^53-1,    },
}

local bor, band, bxor, bnot = bit.bor, bit.band, bit.bxor, bit.bnot
local BOR, BAND, BXOR = 4,5,6
for i,v in ipairs(tests) do
  if bor  (v[1],v[2],v[3]) ~= v[BOR ] then log(i, "BOR") end
  if band (v[1],v[2],v[3]) ~= v[BAND] then log(i, "BAND") end
  if bxor (v[1],v[2],v[3]) ~= v[BXOR] then log(i, "BXOR") end
end

local tests = { -- bnot tests
-- operand1       bnot
  { 0x00000000,   -1               },
  { 0xFFFFFFFF,   -1 - 0xFFFFFFFF  },
  { 0x5A5A5A5A,   -1 - 0x5A5A5A5A  },
}

for i,v in ipairs(tests) do
  if bnot (v[1]) ~= v[2] then log(i, "BNOT") end
end
