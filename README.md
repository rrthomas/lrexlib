# Lrexlib

by Reuben Thomas (<rrt@sc3d.org>)  
and Shmuel Zeigerman (<shmuz@013net.net>)  

**Lrexlib** provides bindings of five regular expression library APIs
([POSIX](https://www.opengroup.org/onlinepubs/009695399/basedefs/xbd_chap09.html),
[PCRE](https://www.pcre.org/current/doc/html/),
[GNU](https://ftp.gnu.org/old-gnu/regex/),
[TRE](https://laurikari.net/tre/documentation/) and
[Oniguruma](https://github.com/kkos/oniguruma)) to
[Lua](https://www.lua.org) \>= 5.1. The bindings for TRE and Oniguruma
are not currently complete.

**Lrexlib** is copyright Reuben Thomas 2000-2026 and copyright Shmuel
Zeigerman 2004-2024, and is released under the same license as Lua, the
[MIT](https://www.opensource.org/licenses/mit-license.php) license
(otherwise known as the revised BSD license). There is no warranty.

Please report bugs and make suggestions on
[GitHub](https://github.com/rrthomas/lrexlib/issues).

Thanks to Thatcher Ulrich for bug and warning fixes, and to Nick Gammon
for adding support for PCRE named subpatterns.

------------------------------------------------------------------------

## Installation

Lrexlib is installed with [LuaRocks](https://luarocks.org), using the
command:

    luarocks install lrexlib-FLAVOUR

where **FLAVOUR** is one of PCRE2, POSIX, oniguruma, TRE, GNU

Note that the obsolete PCRE version 1 is also supported, as flavour
PCRE.

## Quickstart

You can use Lrexlib as follows:

```lua
local rex = require "rex_pcre"
local subj = "We go to school"
local patt = "(\\w+)\\s+(\\w+)"
local repl = "%2 %1"

local from, to, cap1, cap2 = rex.find(subj, patt)
print(from, to, cap1, cap2)

local result = rex.gsub(subj, patt, repl)
print(result)
```

## Links

- [GitHub project page](https://github.com/rrthomas/lrexlib)
- [License](https://rrthomas.github.io/lrexlib/license.html)
- [Reference Manual](https://rrthomas.github.io/lrexlib/manual.html)
- [Downloads](https://github.com/rrthomas/lrexlib/downloads)
