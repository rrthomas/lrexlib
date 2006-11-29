-- [ Shmuel Zeigerman; Nov-Nov 2006 ]

require "posix_test"
require "pcre_test"

do
  local arg1 = ...
  local verbose = (arg1 == "-v")
  local oldprint = print
  if not verbose then
    print = function() end
  end

  local n = 0
  n = n + pcre_test.testlib ("rex_pcre")
  n = n + pcre_test.testlib ("rex_pcre_nr")
  n = n + pcre_test.testlib ("rex_pcre45")

  n = n + posix_test.testlib ("rex_posix1")
  n = n + posix_test.testlib ("rex_posix2")
  n = n + posix_test.testlib ("rex_pcreposix")

  print = oldprint
  print ("Total number of failures: " .. n)
end
