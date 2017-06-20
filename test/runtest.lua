-- See Copyright Notice in the file LICENSE

do
  local path = "./?.lua;"
  if package.path:sub(1, #path) ~= path then
    package.path = path .. package.path
  end
end
local luatest = require "luatest"

local function newalienbuffer (str)
  local alien = require "alien"
  local buf = alien.buffer (#str)
  if #str > 0 then
    alien.memmove (buf:topointer (), str, #str)
  end
  return buf
end

-- returns: number of failures
local function test_library (libname, setfile, verbose, use_alien)
  if verbose then
    print (("[lib: %s; file: %s]"):format (libname, setfile))
  end
  local lib = require (libname)
  local f = require (setfile)
  local sets = f (libname)

  local newmembuffer = use_alien and newalienbuffer or lib._newmembuffer
  if newmembuffer then
    if libname == "rex_posix" and not lib.flags ().STARTEND then
      newmembuffer = nil
      io.stderr:write ("Cannot run posix tests with buffer subjects without REG_STARTEND\n")
    end
  else
    io.stderr:write ("Warning: cannot run tests with buffer subjects\n")
  end

  local n = 0 -- number of failures
  for _, set in ipairs (sets) do
    if verbose then
      print (set.Name or "Unnamed set")
    end
    local err = luatest.test_set (set, lib, newmembuffer)
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
  posix     = { lib = "rex_posix",   "common_sets", "posix_sets" },
  gnu       = { lib = "rex_gnu",     "common_sets", "emacs_sets", "gnu_sets" },
  oniguruma = { lib = "rex_onig",    "common_sets", "oniguruma_sets", },
  pcre      = { lib = "rex_pcre",    "common_sets", "pcre_sets", "pcre_sets2", },
  pcre2     = { lib = "rex_pcre2",   "common_sets", "pcre_sets", "pcre_sets2", },
  spencer   = { lib = "rex_spencer", "common_sets", "posix_sets", "spencer_sets" },
  tre       = { lib = "rex_tre",     "common_sets", "posix_sets", "spencer_sets", --[["tre_sets"]] },
}

do
  local verbose, tests, nerr = false, {}, 0
  local dir
  local use_alien
  -- check arguments
  for i = 1, select ("#", ...) do
    local arg = select (i, ...)
    if arg:sub(1,1) == "-" then
      if arg == "-v" then
        verbose = true
      elseif arg == "-a" then
        use_alien = true
      elseif arg:sub(1,2) == "-d" then
        dir = arg:sub(3)
      else
        error ("invalid argument: [" .. arg .. "]")
      end
    else
      if avail_tests[arg] then
        tests[#tests+1] = avail_tests[arg]
      else
        error ("invalid argument: [" .. arg .. "]")
      end
    end
  end
  assert (#tests > 0, "no library specified")
  -- give priority to libraries located in the specified directory
  if dir then
    dir = dir:gsub("[/\\]+$", "")
    for _, ext in ipairs {"dll", "so", "dylib"} do
      if package.cpath:match ("%?%." .. ext) then
        local cpath = dir .. "/?." .. ext .. ";"
        if package.cpath:sub(1, #cpath) ~= cpath then
          package.cpath = cpath .. package.cpath
        end
        break
      end
    end
  end
  -- do tests
  for _, test in ipairs (tests) do
    package.loaded[test.lib] = nil -- to force-reload the tested library
    for _, setfile in ipairs (test) do
      nerr = nerr + test_library (test.lib, setfile, verbose, use_alien)
    end
  end
  print ("Total number of failures: " .. nerr)
end
