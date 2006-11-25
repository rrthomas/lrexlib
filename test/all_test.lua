-- [ Shmuel Zeigerman; Nov-Nov 2006 ]

require "posix_test"
require "pcre_test"

pcre_test.testlib ("rex_pcre")
pcre_test.testlib ("rex_pcre_nr")
pcre_test.testlib ("rex_pcre45")

posix_test.testlib ("rex_posix1")
posix_test.testlib ("rex_posix2")
posix_test.testlib ("rex_pcreposix")
