import _jpype
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
        return Tracer.ctor-Tracer.dtor

    @staticmethod
    def attach(obj):
        object.__setattr__(obj, "_trace", Tracer())


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
