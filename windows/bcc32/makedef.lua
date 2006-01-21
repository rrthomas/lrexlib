-- This script creates definition (.def) files.
-- arg[1] is target name w/o extension.

local target = assert(arg[1])
local f = assert(io.open(target .. ".def", "w"))
f:write("EXPORTS\n")
f:write("   luaopen_" .. target .. " = _luaopen_" .. target .. "\n")
f:close()
