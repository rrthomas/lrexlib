--[[------------------------------------------------------------------------
   * A script for testing Lua binding of Henry Spencer's regex library.

   * runs tests from the file "tests" taken from the regex distribution.

   * tested with Lua 5.1, Lrexlib 2.0.

   * author: Shmuel Zeigerman

   * history:
     * 14-18 June 2004:  initial writing
     * 29 Oct 2005:      updated due to changes in Lrexlib
     * 10 Nov 2005:      updated due to changes in Lrexlib
     * 07 Sep 2006:      when it is run, does nothing but returns a function;
                         that function accepts 3 parameters
     * 22 Dec 2006:      got rid of getn; Lua 5.1 is required now.
--]]------------------------------------------------------------------------

local Rex, Print, Opts, Errcount
local find, sub, gsub = string.find, string.sub, string.gsub
local insert = table.insert

local function DoOneTest(line, lineno)
  local Prop = {} -- line properties
  local s, e
  s, e, Prop.re, Prop.fl, Prop.f3, Prop.f4, Prop.f5 = find(line,
    "^([^\t]+)\t+([^\t]+)\t+([^\t]+)\t*([^\t]*)\t*([^\t]*)\t*$");
  if not s then
    error ("bad syntax in testfile: line " .. lineno)
  end

  if Prop.re == '""' then Prop.re = "" end
  if Prop.fl == '""' then Prop.fl = "" end
  if Prop.f3 == '""' then Prop.f3 = "" end
  if Prop.f4 == '""' then Prop.f4 = "" end
  if Prop.f5 == '""' then Prop.f5 = "" end

  local Mustfail = find(Prop.fl, "C")
  local Extended = not find(Prop.fl, "b") -- use as boolean
  local Nosub = false

  local Cflags = 0 -- compile flags
  if find(Prop.fl, "i")     then Cflags = Cflags + Opts.ICASE; end
  if find(Prop.fl, "n")     then Cflags = Cflags + Opts.NEWLINE; end
  if find(Prop.fl, "p")     then Cflags = Cflags + Opts.PEND; end
  if find(Prop.fl, "m")     then
    Cflags = Cflags + Opts.NOSPEC
    Extended = false
  end
  if find(Prop.fl, "s")     then
    Cflags = Cflags + Opts.NOSUB
    Nosub = true
  end

  local Trytwice = find(Prop.fl, "%&") -- use as boolean

  local function rep2(s)
    s = gsub(s, "N", "\n")
    s = gsub(s, "S", " ")
    s = gsub(s, "T", "\t")
    s = gsub(s, "Z", "\000")
    return s
  end

  Prop.re = rep2(Prop.re)
  if not Mustfail then
    Prop.f3 = rep2(Prop.f3)
    if Prop.f4 then Prop.f4 = rep2(Prop.f4) end
    if Prop.f5 then Prop.f5 = rep2(Prop.f5) end
  end

  local Errorflag
  local function Error(s)
    Errorflag = true
    Errcount = Errcount + 1
    Print("line " .. lineno)
    Print("", line)
    Print("", "ERROR: " .. s)
    return nil
  end

  local Success
  local function Compile(flags)
    local expr
    Success, expr = pcall(Rex.new, Prop.re, flags)
    if Mustfail and Success or not Mustfail and not Success then
      return Error("compile result is not as expected")
    end
    return expr
  end

  local ExprB, ExprE
  if not Extended or Trytwice then
    ExprB = Compile(Cflags)
    if Errorflag then return end
  end
  if Extended or Trytwice then
    ExprE = Compile(Cflags + Opts.EXTENDED)
    if Errorflag then return end
  end

  if not Success then return end
  -- END OF 'newPOSIX' TEST --

  Mustfail = ( Prop.f4 == "" )
  local Start, Eflags = 0, 0
  if find(Prop.fl, "%^")    then Eflags = Eflags + Opts.NOTBOL; end
  if find(Prop.fl, "%$")    then Eflags = Eflags + Opts.NOTEOL; end
  if find(Prop.fl, "%#")    then
    Eflags = Eflags + Opts.STARTEND
    Start = find(Prop.f3, "%(") + 1
    local fin = find(Prop.f3, "%)")
    Prop.f3 = sub(Prop.f3, 1, fin-1) -- truncating: not fully compatible
                                     --   with REG_STARTEND option
  end

  local function CheckSubstr(sRef, sSubj, iStart, iEnd)
    if sub(sRef,1,1) ~= "@" then -- not null string match
      if sub(sSubj, iStart, iEnd) ~= sRef then
        return Error("found substring differs from the expected")
      end
    else -- null string match
      if iStart ~= iEnd + 1 then
        return Error("null string not matched")
      end
      if sRef ~= "@" then -- substring follows a null string
        local substr = sub(sRef, 2)
        local Start = find(sSubj, substr, 1, true)
        if Start ~= iStart then
          Print("substr="..substr.."; sSubj="..sSubj);
          return Error("substring after a null string not found")
        end
      end
    end
  end

  local function Match(expr)
    local s,e,t = expr:tfind(Prop.f3, Start, Eflags)
    if Mustfail and s or not Mustfail and not s then
      return Error("match result is not as expected")
    end
    if not s or Nosub then return end
    CheckSubstr(Prop.f4, Prop.f3, s, e)
    if Errorflag then return end

    -- subexpressions
    if Prop.f5 ~= "" and Prop.f5 ~= "-" then
      local init, t5 = 1, {}
      while true do
        local a,b,c = find(Prop.f5, "([^,]+)%,?", init)
        if not a then break end
        if c == "-" then c = "" end
        insert(t5, c)
        init = b+1
      end
      if #t5 ~= #t then
        return Error("found number of subexpressions is ".. #t ..
                     "; needed ".. #t5)
      end
      for i,v in ipairs(t) do
        if v then
          if sub(t5[i],1,1) == "@" and v ~= ""
          or sub(t5[i],1,1) ~= "@" and v ~= t5[i] then
            Error("captured substring differs from the expected")
            Print("", "captured: "..v.."; expected: "..t5[i])
          end
          if Errorflag then return true end
        end
      end
    end
  end

  if not Extended or Trytwice then
    Match(ExprB)
    if Errorflag then return end
  end

  if Extended or Trytwice then
    Match(ExprE)
    if Errorflag then return end
  end
end

--[[
   Optional parameters:
       libname  -- posix regex library name (defaults to "rex_posix")
       testfile -- test file name (defaults to "./tests")
       silent   -- no messages are put on the stdout (defaults to false)

   Returns:  number of errors occured.
]]
local function run (libname, testfile, silent)
  -- process parameters
  Rex = require (libname)
  testfile = testfile or "./tests"
  Print = (not silent and print) or (function() end)

  -- initialize upvalues
  Opts = Rex.flags()
  Errcount = 0

  -- run tests
  local lineno = 0
  for line in io.lines(testfile) do
    lineno = lineno + 1
    if line ~= "" and sub(line,1,1) ~= "#" then
      DoOneTest(line, lineno)
    end
  end
  Print("Total errors: " .. Errcount)
  return Errcount
end

return run
