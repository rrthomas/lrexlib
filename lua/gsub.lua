--[[
  This module  defines the  `generic_gsub'  function which
  provides string.gsub-compatible interface.  In addition,
  it allows to plug in various search/match  engines (such
  as PCRE or POSIX regex library).

  Shmuel Zeigerman; June 2004 - February 2006.
--]]

local insert = table.insert
local sub = string.sub
-----------------------------------------------------------------
-- Split the "replace" string and store parts into a table.
--   a) Replace %x with tonumber(x) (if x matches [0-9])
--      and store those numbers ("captures") as separate items
--      in the table.
--      (The %x'es described in this paragraph are the text
--      splitting points).
--   b) Replace %x with x (if x does not match [0-9])
--   c) Return the table.
-----------------------------------------------------------------
local function split_rep_string(str)
    local tb = {}
    local chunk_start = 1
    for s,char in string.gmatch(str, "()%%(.)") do
        if string.find(char, "[0-9]") then
            local chunk = sub(str, chunk_start, s-1)
            if chunk ~= "" then
                insert(tb, (string.gsub(chunk, "%%(.)", "%1")))
            end
            insert(tb, tonumber(char))
            chunk_start = s+2
        end
    end
    local chunk = sub(str, chunk_start)  -- store the last chunk
    if chunk ~= "" then
        insert(tb, (string.gsub(chunk, "%%(.)", "%1")))
    end
    return tb
end

-- generic_gsub()
--
--   This function can be "wrapped", for example:
--       function gsubPCRE(str, pat, repl, n)
--          return generic_gsub(pcre.newPCRE, str, pat, repl, n)
--       end
--   Wrapper like this can be direct replacement for string.gsub().
--
--   This function's API is meant to be 100% string.gsub (Lua 5.1)
--   compatible,
--   except for syntax of regular expression in the argument 'pat'
--   which must correspond to the library determined by argument 'func'.
--   (Argument 'repl' has the same syntax as one in string.gsub, i.e.,
--   the character % is treated specially).
--
--   Arguments n, cflags, eflags are all optional. If there's a need to
--   use the library defaults, any of them must be either omitted (if it
--   is a trailing one) or passed as nil.
--
function generic_gsub (func, str, pat, repl, n, cflags, eflags)
    local reptype = type(repl)
    assert(reptype == "string" or reptype == "function" or reptype == "table",
                "generic_gsub: 4th argument must be string, function or table")
    assert(n == nil or type(n) == "number",
                "generic_gsub: 5th argument must be number or nil")

    local expr = func(pat, cflags) -- compile the pattern
    local num_rep = 0              -- current number of replacements made
    local curr_start = 1           -- current offset in str for matching operation
    local tb_out = {}              -- array of the output string's parts
    local tb_replace               -- array of the replace string's parts

    if reptype == "string" then
        tb_replace = split_rep_string(repl)
    end
    if reptype == "table" then
        setmetatable(repl, { __call = function(self, k) return self[k] end })
    end

    while not n or n > num_rep do

        local from,to,cap = expr:match(str, curr_start, eflags)
        if not from then break; end
        insert( tb_out, sub(str, curr_start, from-1) )

        if reptype == "string" then
            for _,v in ipairs(tb_replace) do
                if type(v) == "number" then
                    if v==0 or (v==1 and #cap==0) then insert(tb_out, sub(str,from,to))
                    else
                        assert(cap[v] ~= nil, "invalid capture index")
                        if cap[v] then insert(tb_out, cap[v]) end
                    end
                else
                    insert(tb_out, v)
                end
            end

        elseif reptype == "function" or reptype == "table" then
            local val
            if reptype == "function" and #cap > 0 then
                val = repl(unpack(cap))
            elseif reptype == "table" and cap[1] then
                val = repl[cap[1]]
            else
                val = repl(sub(str,from,to)) -- metatable __call used if repl is a table
            end
            if val then
                local valtype = type(val)
                if valtype == "string" or valtype == "number" then
                    insert(tb_out, val)
                else
                    error("invalid replacement value (a " .. valtype .. ")")
                end
            else
                insert(tb_out, sub(str,from,to))
                num_rep = num_rep - 1
            end
        end

        num_rep = num_rep + 1
        if curr_start <= to then
            curr_start = to+1
        elseif curr_start <= #str then -- advance by 1 char (not replaced)
            table.insert(tb_out, string.sub(str, curr_start, curr_start))
            curr_start = curr_start + 1
        else
            break
        end
    end

    if curr_start <= #str then
        insert(tb_out, sub(str, curr_start))
    end

    return table.concat(tb_out), num_rep
end

