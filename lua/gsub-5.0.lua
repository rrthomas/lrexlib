--[[
  This module  defines the  `generic_gsub'  function which
  provides string.gsub-compatible interface.  In addition,
  it allows to plug in various search/match  engines (such
  as PCRE or POSIX regex library).

  Shmuel Zeigerman; June 2004 - November 2005.
--]]

local insert = table.insert
local sub = string.sub
-----------------------------------------------------------------
-- Split the "replace" string and store parts into a table.
--   a) Replace %x with tonumber(x) (if x matches [1-9])
--      and store those numbers ("captures") as separate items
--      in the table.
--      (The %x'es described in this paragraph are the text
--      splitting points).
--   b) Replace %x with x (if x does not match [1-9])
--   c) Return the table and the number of "captures".
-----------------------------------------------------------------
local function split_rep_string(str)
    local tb = {}
    local num_capt = 0
    local chunk_start = 1
    for s,char in string.gfind(str, "()%%(.)") do
        if string.find(char, "[1-9]") then
            local chunk = sub(str, chunk_start, s-1)
            if chunk ~= "" then
                insert(tb, (string.gsub(chunk, "%%(.)", "%1")))
            end
            insert(tb, tonumber(char))
            chunk_start = s+2
            num_capt = num_capt + 1
        end
    end
    local chunk = sub(str, chunk_start)  -- store the last chunk
    if chunk ~= "" then
        insert(tb, (string.gsub(chunk, "%%(.)", "%1")))
    end
    return tb, num_capt
end

-- generic_gsub()
--
--   This function can be "wrapped", for example:
--       function gsubPCRE(str, pat, repl, n)
--          return generic_gsub(pcre.newPCRE, str, pat, repl, n)
--       end
--   Wrapper like this can be direct replacement for string.gsub().
--
--   This function's API is meant to be 100% string.gsub (Lua 5.02)
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
    assert(type(repl) == "string" or type(repl) == "function",
                "generic_gsub: 4th argument must be string or function")
    assert(n == nil or type(n) == "number",
                "generic_gsub: 5th argument must be number or nil")

    local expr = func(pat, cflags) -- compile the pattern
    local num_rep = 0              -- current number of replacements made
    local curr_start = 1           -- current offset in str for matching operation
    local tb_out = {}              -- array of the output string's parts
    local tb_replace               -- array of the replace string's parts
    local n_capt_repl              -- number of captures in the replace string

    if type(repl) == "string" then
        tb_replace, n_capt_repl = split_rep_string(repl)
    end

    while not n or n > num_rep do

        local s,e,t = expr:match(str, curr_start, eflags)
        if s then
            insert( tb_out, sub(str, curr_start, s-1) )

            if type(repl) == "string" then
                if n_capt_repl == 0 then
                    insert(tb_out, tb_replace[1])
                else
                    for _,v in ipairs(tb_replace) do
                        if type(v) == "number" then
                            if t[v] then insert(tb_out, t[v]) end
                        else
                            insert(tb_out, v)
                        end
                    end
                end

            else -- if type(repl) == "function" then
                local val
                if table.getn(t) > 0 then
                    val = repl(unpack(t))
                else
                    val = repl(sub(str,s,e))
                end
                if type(val) == "string" then
                    insert(tb_out, val)
                end
            end

            num_rep = num_rep + 1
            curr_start = e+1
        else
            break
        end
    end

    if curr_start <= string.len(str) then
        insert(tb_out, sub(str, curr_start))
    end

    return table.concat(tb_out), num_rep
end

