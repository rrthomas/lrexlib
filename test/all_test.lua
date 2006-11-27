-- [ Shmuel Zeigerman; Nov-Nov 2006 ]

require "posix_test"
require "pcre_test"

do
  local oldprint = print
  print = function() end -- make it silent
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
