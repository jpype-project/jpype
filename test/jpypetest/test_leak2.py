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
# *****************************************************************************import _jpype
import jpype
from jpype.types import *
import sys
import logging
import time
import common


class Tracer(object):
    ctor = 0
    dtor = 0

    def __init__(self):
        Tracer.ctor += 1

    def __del__(self):
        Tracer.dtor += 1

    @staticmethod
    def reset():
        Tracer.ctor = 0
        Tracer.dtor = 0

    @staticmethod
    def leaked():
        return Tracer.ctor - Tracer.dtor

    @staticmethod
    def attach(obj):
        setattr(obj, "_trace", Tracer())


# This test finds reference counting leak by attaching an
# object that lives as long as the wrapper is alive.
# It can't detect Java reference counter leaks.
class Leak2TestCase(common.JPypeTestCase):

    def setUp(self):
        common.JPypeTestCase.setUp(self)
        self.fixture = JClass('jpype.common.Fixture')()
        Tracer.reset()

    def testArrayCall(self):
        JA = JArray(JInt)

        def call():
            inst = JA(100)
            Tracer.attach(inst)
            self.fixture.callObject(inst)
        for i in range(100):
            call()
        self.assertEqual(Tracer.leaked(), 0)

    def testArrayMemoryView(self):
        JA = JArray(JInt)

        def call():
            inst = JA(100)
            Tracer.attach(inst)
            memoryview(inst)
        for i in range(100):
            call()
        self.assertEqual(Tracer.leaked(), 0)

    def testBufferCall(self):
        byte_buffer = jpype.JClass('java.nio.ByteBuffer')

        def call():
            inst = byte_buffer.allocateDirect(10)
            Tracer.attach(inst)
            self.fixture.callObject(inst)
        for i in range(100):
            call()
        self.assertEqual(Tracer.leaked(), 0)

    def testBufferMemoryView(self):
        byte_buffer = jpype.JClass('java.nio.ByteBuffer')

        def call():
            inst = byte_buffer.allocateDirect(10)
            Tracer.attach(inst)
            memoryview(inst)
        for i in range(100):
            call()
        self.assertEqual(Tracer.leaked(), 0)
