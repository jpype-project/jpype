import _jpype
import jpype
from jpype.types import *
import sys
import logging
import time
import common


class JBooleanTestCase(common.JPypeTestCase):
    def setUp(self):
        common.JPypeTestCase.setUp(self)
        self.Test = jpype.JClass("jpype.common.Fixture")()

    @common.requireInstrumentation
    def testJPBoolean_str(self):
        jb = JBoolean(True)
        _jpype.fault("PyJPBoolean_str")
        with self.assertRaisesRegex(SystemError, "fault"):
            str(jb)
        _jpype.fault("PyJPModule_getContext")
        str(jb)

    @common.requireInstrumentation
    def testJPBooleanType(self):
        ja = JArray(JBoolean)(5)  # lgtm [py/similar-function]
        _jpype.fault("JPBooleanType::setArrayRange")
        with self.assertRaisesRegex(SystemError, "fault"):
            ja[1:3] = [0, 0]
        with self.assertRaises(TypeError):
            ja[1] = object()
        jf = JClass("jpype.common.Fixture")
        with self.assertRaises(TypeError):
            jf.static_bool_field = object()
        with self.assertRaises(TypeError):
            jf().bool_field = object()

    @common.requireInstrumentation
    def testJBooleanGetJavaConversion(self):
        _jpype.fault("JPBooleanType::getJavaConversion")
        with self.assertRaisesRegex(SystemError, "fault"):
            JBoolean._canConvertToJava(object())

    def testBooleanFromInt(self):
        self.assertEqual(self.Test.callBoolean(int(123)), True)
        self.assertEqual(self.Test.callBoolean(int(0)), False)

    @common.requireNumpy
    def testBooleanFromNPInt(self):
        import numpy as np
        self.assertEqual(self.Test.callBoolean(np.int(123)), True)

    @common.requireNumpy
    def testBooleanFromNPInt8(self):
        import numpy as np
        self.assertEqual(self.Test.callBoolean(np.int8(123)), True)
        self.assertEqual(self.Test.callBoolean(np.uint8(123)), True)

    @common.requireNumpy
    def testBooleanFromNPInt16(self):
        import numpy as np
        self.assertEqual(self.Test.callBoolean(np.int16(123)), True)
        self.assertEqual(self.Test.callBoolean(np.uint16(123)), True)

    @common.requireNumpy
    def testBooleanFromNPInt32(self):
        import numpy as np
        self.assertEqual(self.Test.callBoolean(np.int32(123)), True)
        self.assertEqual(self.Test.callBoolean(np.uint32(123)), True)

    @common.requireNumpy
    def testBooleanFromNPInt64(self):
        import numpy as np
        self.assertEqual(self.Test.callBoolean(np.int64(123)), True)
        self.assertEqual(self.Test.callBoolean(np.uint64(123)), True)

    def testBooleanFromFloat(self):
        with self.assertRaises(TypeError):
            self.Test.callBoolean(float(2))

    @common.requireNumpy
    def testBooleanFromNPFloat(self):
        import numpy as np
        with self.assertRaises(TypeError):
            self.Test.callBoolean(np.float(2))

    @common.requireNumpy
    def testBooleanFromNPFloat32(self):
        import numpy as np
        with self.assertRaises(TypeError):
            self.Test.callBoolean(np.float32(2))

    @common.requireNumpy
    def testBooleanFromNPFloat64(self):
        import numpy as np
        with self.assertRaises(TypeError):
            self.Test.callBoolean(np.float64(2))

    def testBooleanFromNone(self):
        with self.assertRaises(TypeError):
            self.Test.callBoolean(None)

    def testJArrayConversionBool(self):
        expected = [True, False, False, True]
        jarr = jpype.JArray(jpype.JBoolean)(expected)
        self.assertEqual(expected, list(jarr[:]))

    @common.requireNumpy
    def testSetFromNPBoolArray(self):
        import numpy as np
        n = 100
        a = np.random.randint(0, 1, size=n, dtype=np.bool)
        jarr = jpype.JArray(jpype.JBoolean)(n)
        jarr[:] = a
        self.assertCountEqual(a, jarr)
