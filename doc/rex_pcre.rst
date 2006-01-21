=======================
Lua rexlib release 1.20
=======================
    | by Shmuel Zeigerman (shmuz at actcom co il) [maintainer]
    | and Reuben Thomas (rrt at sc3d org)

.. contents:: Table of Contents

.. role:: funcdef(literal)

Synopsis
--------

The initialisation function (for use with `loadlib`) is `luaopen_rex_pcre`.
The library provides PCRE regular expression matching:

--------

rex_pcre.new
~~~~~~~~~~~~
:funcdef:`rex_pcre.new(p[, cf[, lo]])`
    Returns a PCRE regular expression object for the pattern `p`,
    subject to compilation flags `cf` (a number) and locale `lo` (a
    string).

--------

rex_pcre.flags
~~~~~~~~~~~~~~
:funcdef:`rex_pcre.flags([t])`
    Fills the table *t* with all available PCRE flags (numbers)
    keyed by their names (strings).

    - if the parameter *t* is missing, then a new table *t* is first
      created.
    - what flags are available depends on the version of the used
      PCRE library.

    Returns: *t*.

--------

rex_pcre.version
~~~~~~~~~~~~~~~~
:funcdef:`rex_pcre.version()`
    Returns a string containing version and release date of the
    used PCRE library.

--------

r:match
~~~~~~~~
.. _r:match:

:funcdef:`r:match(s[, st[, ef[, f1]]])`
    Returns (when there is a match):

    - 1-st and 2-nd results: the start and end point of the first
      match of the compiled regexp `r` in the string `s`, starting
      from offset `st` (a number), subject to execution flags `ef`
      (a number);
    - 3-rd result: a table of substring matches ("captures" in Lua
      terminology). This table contains `false` in the positions
      where the corresponding sub-pattern did not match. If named
      subpatterns are used then the table also contains substring
      matches keyed by their correspondent subpattern names
      (strings).
    - 4-th result: return value of the underlying *pcre_exec()*
      call (a number).

    OR (no match or failure):

    - 1-st result: nil
    - 2-nd result: return value of the underlying *pcre_exec()* call
      (a number)

    *f1* parameter is a callout_ function. It receives a table as its
    only parameter.

--------

r:exec
~~~~~~~
:funcdef:`r:exec(s[, st[, ef[, f1]]])`
    This function is like `r:match`_ except that a table returned
    as a 3-rd result contains offsets of substring matches rather
    than substring matches themselves. (That table will not contain
    string keys, even if named subpatterns are used).

    For example, if the whole match is at offsets 10,20 and
    substring matches are at offsets 12,14 and 16,19 then the
    function returns the following: 10, 20, { 12,14,16,19 }, 3

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

    If "named subpatterns" are used then the table also
    contains substring matches keyed by their correspondent
    subpattern names (strings).

    If `f` returns a true value, then `gmatch` immediately returns;
    `gmatch` returns the number of matches made.

--------

r:dfa_exec -- PCRE 6 and above
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
:funcdef:`r:dfa_exec(s[, st[, ef[, f1]]])`
    | This function uses DFA matching algorithm.
    | See `r:match`_ for description of parameters.
    | It returns (when there are matches):

    - start offset of the found matches (a number)
    - an array containing end offsets of the found matches (a table)
    - return value of the underlying *pcre_dfa_exec()* call (a number)

    OR (no matches or failure):

    - nil
    - return value of the underlying *pcre_dfa_exec()* call (a number)

--------

.. _callout:

Callouts (copied from PCRE docs)
--------------------------------
Within a regular expression, (?C) indicates the points at which
the external function is to be called. Different callout points
can be identified by putting a number less than 256 after the
letter C. The default value is zero.

The external callout function returns an integer to PCRE.
If the value is zero, matching proceeds as normal. If the value
is greater than zero, matching fails at the current point, but
the testing of other matching possibilities goes ahead, just as
if a lookahead assertion had failed. If the value is less than
zero, the match is abandoned, and pcre_exec() (or pcre_dfa_exec())
returns the negative value.

Negative values should normally be chosen from the set of
PCRE_ERROR_xxx values. In particular, PCRE_ERROR_NOMATCH forces
a standard "no match" failure. The error number PCRE_ERROR_CALLOUT
is reserved for use by callout functions; it will never be used by
PCRE itself.

Notes
------
Optional arguments supplied as `nil` are treated as if they had been
omitted. Regular expression objects are automatically garbage
collected.

Strings to be matched may contain NULs but patterns may not.

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

