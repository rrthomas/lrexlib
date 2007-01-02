-- [ Shmuel Zeigerman; Nov-Dec 2006 ]

local luatest = require "luatest"

-- returns: number of failures
local function test_library (libname, setfile, verbose)
  if verbose then
    print (("[lib: %s; file: %s]"):format (libname, setfile))
  end
  local lib = require (libname)
  local f = require (setfile)
  local sets = f (libname)
  local n = 0 -- number of failures
  for _, set in ipairs (sets) do
    if verbose then
      print (set.Name or "Unnamed set")
    end
    local err = luatest.test_set (set, lib)
    if verbose then
      for _,v in ipairs (err) do
        print ("  Test " .. v.i)
        luatest.print_results (v, "  ")
      end
    end
    n = n + #err
  end
  if verbose then
    print ""
  end
  return n
end

local tests = {
  { lib = "rex_posix", "common_sets", "posix_sets", },
  { lib = "rex_pcre",  "common_sets", "pcre_sets", "pcre_sets2", },
}

local function test_all (verbose)
  local n = 0
  for _, tlib in ipairs (tests) do
    for _, setfile in ipairs (tlib) do
      n = n + test_library (tlib.lib, setfile, verbose)
    end
  end
  return n
end

local arg1 = ...
local verbose = (arg1 == "-v")
local n = test_all (verbose)

print ("Total number of failures: " .. n)

