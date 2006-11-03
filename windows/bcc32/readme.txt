Introduction
------------
  This directory contains files for building Lrexlib libraries for Windows
  using Borland development tools.

  The makefile was tested with:
     bcc32.exe     -   version 5.5.1
     ilink32.exe   -   version 5.00
     make.exe      -   version 5.2

Build instructions
------------------
  1. Edit config.mak to adjust the directory names, library names
     and target names to your system configuration and personal preferences.

  2. Run "make.exe -fmake_bcc.mak"
         -- to build all the available targets;
     Run "make.exe -fmake_bcc.mak <target_name> [<target_name> ...]",
                 for example:    make.exe -fmake_bcc.mak posix1 pcre
         -- to build only specified targets.

