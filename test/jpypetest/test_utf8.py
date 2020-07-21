# -*- coding: utf-8 -*-
# *****************************************************************************
#   Copyright 2018 Rene Bakker
# *****************************************************************************
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.
#
#   See NOTICE file for details.
#
# *****************************************************************************

'''
    Test communication with jpype using 4-byte utf-8 characters (emoji)

    IMPORTANT:
    The routines have only be tested in Python3. Given the difference in string handling between
    Python2 and Python3, it is not likely this routines have any significance for Python2.

    The following tests are defined:
    General:
    1. Test if the java class (jpype.utf8.Utf8Test) can return the default ASCII string.
    2. Pass an ASCII string to the java test class and check if it remained unchanged when returned from java.

    Binary: python strings are injected into java.lang.String in binary format with str.encode()
    3. Pass a series of reference UTF-8 strings and compare them with the reference strings
       in the java class.
    4. Pass a series of reference UTF-8 strings and check if they remained unchanged when returned from Java.
       Allow for surrogate substitution in the utf-16 strings returned from Java.
    5. Pass a series of reference UTF-8 strings and check if they remained unchanged when returned from Java.
       Use the python default strict encoding rules for the returned string.

    Navive strings: python strings are passed as-is into a java method, wich accepts String as argument
    6. Pass a series of reference UTF-8 strings and compare them with the reference strings
       in the java class.
    7. Pass a series of reference UTF-8 strings and check if they remained unchanged when returned from Java.
       Allow for surrogate substitution in the utf-16 strings returned from Java.
    8. Pass a series of reference UTF-8 strings and check if they remained unchanged when returned from Java.
       Use the python default strict encoding rules for the returned string.

    At the time if writing:
    Passed tests: 1, 2, 3, and 4
    Failed tests:
     5. encoding error returned string
     6. uploaded string mutilated for emoji
     7. follow-up of 6: mutilated string returned to python (emoji only=
     8. idem 7.

    Note on encoding errors:
    UnicodeEncodeError: 'utf-8' codec can't encode characters in position xxx-xxx: surrogates not allowed
'''

import sys
from jpype.types import *
import common


class Utf8TestCase(common.JPypeTestCase):

    def setUp(self):
        common.JPypeTestCase.setUp(self)

        # Java IO test class
        self.Utf8Test = JClass('jpype.utf8.Utf8Test')

        # Test strings
        # IMPORTANT: they should be identical, and in the same order, as the test strings difned in the
        #            java class Utf8Test
        self.TDICT = []
        self.TDICT.append(['english',
                           ("I can eat glass and it doesn't hurt me.")])
        self.TDICT.append(['french',
                           ("Je peux manger du verre, ça ne me fait pas mal.")])
        self.TDICT.append(['rune',
                           ("ᛖᚴ ᚷᛖᛏ ᛖᛏᛁ ᚧ ᚷᛚᛖᚱ ᛘᚾ ᚦᛖᛋᛋ ᚨᚧ ᚡᛖ ᚱᚧᚨ ᛋᚨᚱ")])
        self.TDICT.append(['cn_simp',
                           ("人人生而自由,在尊严和权利上一律平等。他们赋有理性和良心,并应以兄弟关系的精神互相对待。")])
        self.TDICT.append(['cn_trad',
                           ("人人生而自由﹐在尊嚴和權利上一律平等。他們賦有理性和良心﹐並應以兄弟關係的精神互相對待。")])
        self.TDICT.append(['arab',
                           ("أنا قادر على أكل الزجاج و هذا لا يؤلمني.")])
        self.TDICT.append(['emoji',
                           ("😁😂😃😄😅😆😠😡😢😣😤😥😨😩😪🚉🚌🚏🚑🚒🚓🚕🚗🚙🚚🚢🚤🚥🚧🚨🚻🚼🚽🚾🛀🆕🆖🆗🆘🆙🆚🈁🈂🈚🈯🈹🈺🉐🉑8⃣9⃣7⃣6⃣1⃣0")])

    def test_get_ascii(self):
        """
        Test if the default string returns from the java test class.
        """
        utf8_test = self.Utf8Test()
        self.assertEqual("Utf8Test pure ASCII", utf8_test.get(),
                         "Utf8Test.java default string")

    def test_ascii_upload(self):
        """
        Test uploading and downloading of a simple ASCII string.
        """
        test_string = 'Python Utf8Test ascii test string'
        utf8_test = self.Utf8Test(test_string)
        self.assertEqual(test_string, utf8_test.get(),
                         "Utf8Test.java uploaded ASCII string")

    def test_binary_upload(self):
        """
        Test binary upload and check in Java if the strings are correct.
        Assumes synchronized test strings in the java class and in this test class.
        """
        String = JClass('java.lang.String')
        indx = 0
        for lbl, val in self.TDICT:
            utf8_test = self.Utf8Test(String(val.encode('utf-8'), 'UTF8'))
            self.assertTrue(utf8_test.equalsTo(indx), "Utf8Test.java binary upload %d (%s) = %s" %
                            (indx, lbl, val))
            indx += 1

    def test_binary_upload_with_surrogates(self):
        """
        Test binary upload and download of utf strings.
        Allow for surrogate unicode substitution of the return value.
        """
        String = JClass('java.lang.String')
        for lbl, val in self.TDICT:
            utf8_test = self.Utf8Test(String(val.encode('utf-8'), 'UTF8'))
            try:
                rval = str(utf8_test.get()).encode(
                    'utf-16').decode('utf-16')
            except UnicodeEncodeError as uue:
                rval = str(utf8_test.get()).encode(
                    'utf-16', errors='surrogatepass').decode('utf-16')
                lbl += (' ' + str(uue))
            self.assertEqual(
                val, rval, "Utf8Test.java binary upload with surrogate substitution for: " + lbl)

    def test_binary_upload_no_surrogates(self):
        """
        Test pure binary upload and download of utf strings.
        """
        String = JClass('java.lang.String')
        for lbl, val in self.TDICT:
            utf8_test = self.Utf8Test(String(val.encode('utf-8'), 'UTF8'))
            self.assertEqual(val, str(utf8_test.get()),
                             "Utf8Test.java binary upload for: " + lbl)

    def test_string_upload(self):
        """
        Test binary upload and check in Java if the strings are correct.
        Assumes synchronized test strings in the java class and in this test class.
        """
        indx = 0
        for lbl, val in self.TDICT:
            utf8_test = self.Utf8Test(val)
            self.assertTrue(utf8_test.equalsTo(
                indx), "Utf8Test.java binary upload: indx %d = %s" % (indx, lbl))
            indx += 1

    def test_string_upload_with_surrogates(self):
        """
        Test python string upload and download of utf strings.
        Allow for surrogate unicode substitution of the return value.
        """
        for lbl, val in self.TDICT:
            utf8_test = self.Utf8Test(val)
            try:
                rval = str(utf8_test.get()).encode(
                    'utf-16').decode('utf-16')
            except UnicodeEncodeError as uue:
                rval = str(utf8_test.get()).encode(
                    'utf-16', errors='surrogatepass').decode('utf-16')
                lbl += (' ' + str(uue))
            self.assertEqual(
                val, rval, "Utf8Test.java string upload with surrogate substitution for: " + lbl)

    def test_string_upload_no_surrogates(self):
        """
        Test pure python string upload and download of utf strings.
        """
        for lbl, val in self.TDICT:
            utf8_test = self.Utf8Test(val)
            res = str(utf8_test.get())
            self.assertEqual(
                val, res, "Utf8Test.java string upload for: " + lbl)
