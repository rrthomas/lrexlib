=======================
Lua rexlib release 1.20
=======================
    | by Shmuel Zeigerman (shmuz at actcom co il) [maintainer]
    | and Reuben Thomas (rrt at sc3d org)

.. contents:: Table of Contents

.. role:: funcdef(literal)

Synopsis
---------

The initialisation function (for use with `loadlib`) is `luaopen_rex_posix`.
The library provides POSIX regular expression matching:

--------

rex_posix.new
~~~~~~~~~~~~~
:funcdef:`rex_posix.new(p[, cf])`
    returns a POSIX regular expression object for the pattern `p`,
    subject to compilation flags `cf` (a number).

--------

rex_posix.flags
~~~~~~~~~~~~~~~
:funcdef:`rex_posix.flags([t])`
    fills the table *t* with all available POSIX flags (numbers)
    keyed by their names (strings). If the parameter *t* is missing,
    then a new table *t* is first created. Returns *t*.

--------

r:match
~~~~~~~~
.. _r:match:

:funcdef:`r:match(s[, st[, ef]])`
    Returns (when there is a match):

    - 1-st and 2-nd results: the start and end point of the first
      match of the compiled regexp `r` in the string `s`, starting
      from offset `st` (a number), subject to execution flags `ef`
      (a number);
    - 3-rd result: a table of substring matches ("captures" in Lua
      terminology). This table contains `false` in the positions
      where the corresponding sub-pattern did not match.
    - 4-th result: return value of the underlying *regexec()*
      call (a number).

    OR (no match or failure):

    - 1-st result: nil
    - 2-nd result: return value of the underlying *regexec()* call
      (a number)

--------

r:exec
~~~~~~~
:funcdef:`r:exec(s[, st[, ef]])`
    This function is like `r:match`_ except that a table returned
    as a third result contains offsets of substring matches
    rather than substring matches themselves.

    For example, if the whole match is at offsets 10,20 and
    substring matches are at offsets 12,14 and 16,19 then the
    function returns the following: 10, 20, { 12,14,16,19 }, 0.

--------

r:gmatch
~~~~~~~~~
:funcdef:`r:gmatch(s, f[, n[, ef]])`
    Tries to match the regex `r` against `s` up to `n` times (or as
    many as possible if `n` is either not given or is not a
    positive number), subject to execution flags `ef`;

    Each time there is a match, `f` is called as `f(m, t)`, where `m`
    is the matched string and `t` is a table of substring matches
    (this table contains `false` in the positions where the
    corresponding sub-pattern did not match.);

    If `f` returns a true value, then `gmatch` immediately returns;
    `gmatch` returns the number of matches made.

--------

Notes
------
Optional arguments supplied as `nil` are treated as if they had been
omitted. Regular expression objects are automatically garbage
collected.

If the library is compiled against Henry Spencer's regex library, then
regular expressions and the strings to match them against may contain
NULs.

Hint
------
It may be useful to define::

    -- Default constructor
    setmetatable(rex, {__call =
                   function (self, p, cf)
                     return self.new(p, cf)
                   end})

    -- partial string.find equivalent
    function rex.find(s, p, st)
      return rex(p):match(s, st)
    end

    -- partial string.gsub equivalent
    function rex.gsub(s, p, f, n)
      return rex(p):gmatch(s, f, n)
    end

