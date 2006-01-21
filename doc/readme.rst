=======================
Lua rexlib release 1.20
=======================
    | by Shmuel Zeigerman (shmuz at actcom co il) [maintainer]
    | and Reuben Thomas (rrt at sc3d org)

.. contents:: Table of Contents

Introduction
------------

`This archive`_ contains the **lrexlib** regular expression library for Lua_
5.0 and 5.1. The Makefile provided builds it into shared libraries called
rex_posix.so and rex_pcre.so, which can be used with `require`.

**Lrexlib** is copyright Reuben Thomas 2000-2006
and copyright Shmuel Zeigerman 2004-2006, and is released under
the `MIT license`_.

.. _This archive: http://luaforge.net/frs/?group_id=63
.. _Lua: http://www.lua.org
.. _MIT license: license.html

Please report bugs and make suggestions to Shmuel, who is the current
maintainer.

| Thanks to Thatcher Ulrich for bug and warning fixes.
| Thanks to Nick Gammon for adding support for PCRE named subpatterns.

| `POSIX regexp binding`_
| `PCRE binding`_

.. _`POSIX regexp binding`: rex_posix.html
.. _`PCRE binding`: rex_pcre.html

Notes
------
Optional arguments supplied as `nil` are treated as if they had been
omitted. Regular expression objects are automatically garbage
collected.

If the library is compiled against Henry Spencer's regex library, then
regular expressions and the strings to match them against may contain
NULs. (For PCRE, strings to be matched may contain NULs but patterns
may not.)

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

