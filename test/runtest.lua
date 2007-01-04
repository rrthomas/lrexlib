-- See Copyright Notice in the file LICENSE

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

local avail_tests = {
  posix = { lib = "rex_posix", "common_sets", "posix_sets", },
  pcre  = { lib = "rex_pcre",  "common_sets", "pcre_sets", "pcre_sets2", },
}

do
  local verbose, tests, nerr = false, {}, 0
  -- check arguments
  for i = 1, select ("#", ...) do
    local arg = select (i, ...)
    if arg == "-v" then
      verbose = true
    else
      if avail_tests[arg] then
        tests[#tests+1] = avail_tests[arg]
      else
        error ("invalid argument: [" .. arg .. "]")
      end
    end
  end
  assert (#tests > 0, "no library specified")
  -- do tests
  for _, test in ipairs (tests) do
    for _, setfile in ipairs (test) do
      nerr = nerr + test_library (test.lib, setfile, verbose)
    end
  end
  print ("Total number of failures: " .. nerr)
end
