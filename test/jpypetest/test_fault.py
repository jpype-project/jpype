import _jpype
import jpype
from jpype import *
import common


class FaultTestCase(common.JPypeTestCase):
    """ Test for fault paths in JPype

    This test is only executed if fault instrumentation is compiled in.
    Fault instrumentation is trigger as part of the coverage compilation.

    This test suite brutally tries to force an exception to be thrown
    at each entry point and function call.  The exception is controlled
    based on the name of the function in the JP_TRACE_IN and JP_PY_TRY
    block.  Specific fault points are also triggered to produce
    abnormal objects which can then be passed to trigger error handling
    behaviors for off normal conditions.

    Most of the time the correct response to a fault is to pass it back
    to the user.  But there are two exception to this rule.  Function
    in which a Java resource is released using a Release* method must not
    fault because these calls can occur during exception handling routines
    and throwing may trigger an abort.  Second, Python calls to free,
    finalize, and dealloc are not allowed to propagate exceptions as
    this would potentially interrupt the GC or prevent a object from being
    freed.  Exceptions during the destructor path should be eaten or
    they may trigger randomly in places the user can't control (assigning
    a variable, leaving a scope).

    Many of these tests will be moved to other test units so that they
    are located with rest of the tests for that unit.  Tests that
    test for something other than SystemError should be separated from
    the rest of the fault test.

    This may be one of the most tedious files every written by human hands.
    Coffee is advised.

    """

    def setUp(self):
        common.JPypeTestCase.setUp(self)

    @common.requireInstrumentation
    def testJPArray_new(self):
        _jpype.fault("PyJPArray_new")
        with self.assertRaisesRegex(SystemError, "fault"):
            JArray(JInt)(5)

    @common.requireInstrumentation
    def testJPArray_init(self):
        _jpype.fault("PyJPArray_init")
        with self.assertRaisesRegex(SystemError, "fault"):
            JArray(JInt)(5)
        _jpype.fault("PyJPModule_getContext")
        with self.assertRaisesRegex(SystemError, "fault"):
            JArray(JInt)(5)

    # FIXME move these tests to a different test unit
    def testJPArray_initExc(self):
        with self.assertRaises(TypeError):
            _jpype._JArray("foo")
        with self.assertRaises(TypeError):
            JArray(JInt)(JArray(JDouble)([1, 2]))
        with self.assertRaises(TypeError):
            JArray(JInt)(JString)
        with self.assertRaises(ValueError):
            JArray(JInt)(-1)
        with self.assertRaises(ValueError):
            JArray(JInt)(10000000000)
        with self.assertRaises(TypeError):
            JArray(JInt)(object())
        self.assertEqual(len(JArray(JInt)(0)), 0)
        self.assertEqual(len(JArray(JInt)(10)), 10)
        self.assertEqual(len(JArray(JInt)([1, 2, 3])), 3)
        self.assertEqual(len(JArray(JInt)(JArray(JInt)([1, 2, 3]))), 3)

        class badlist(list):
            def __len__(self):
                return -1
        with self.assertRaises(ValueError):
            JArray(JInt)(badlist([1, 2, 3]))

    @common.requireInstrumentation
    def testJPArray_repr(self):
        ja = JArray(JInt)(5)
        _jpype.fault("PyJPArray_repr")
        with self.assertRaisesRegex(SystemError, "fault"):
            repr(ja)
        _jpype.fault("PyJPModule_getContext")
        with self.assertRaisesRegex(SystemError, "fault"):
            repr(ja)

    @common.requireInstrumentation
    def testJPArray_len(self):
        ja = JArray(JInt)(5)
        _jpype.fault("PyJPArray_len")
        with self.assertRaisesRegex(SystemError, "fault"):
            len(ja)
        _jpype.fault("PyJPArray_len")
        with self.assertRaisesRegex(SystemError, "fault"):
            ja.length
        _jpype.fault("PyJPModule_getContext")
        with self.assertRaisesRegex(SystemError, "fault"):
            len(ja)

    @common.requireInstrumentation
    def testJPArray_getArrayItem(self):
        ja = JArray(JInt)(5)
        _jpype.fault("PyJPArray_getArrayItem")
        with self.assertRaisesRegex(SystemError, "fault"):
            print(ja[0])
        _jpype.fault("PyJPModule_getContext")
        with self.assertRaisesRegex(SystemError, "fault"):
            print(ja[0])

    # FIXME move to another test unit
    def testJPArray_getArrayItemExc(self):
        ja = JArray(JInt)(5)
        with self.assertRaises(TypeError):
            ja[object()]
        with self.assertRaises(ValueError):
            ja[slice(0, 0, 0)]
        self.assertEqual(len(JArray(JInt)(5)[4:1]), 0)

    @common.requireInstrumentation
    def testJPArray_assignSubscript(self):
        ja = JArray(JInt)(5)
        _jpype.fault("PyJPArray_assignSubscript")
        with self.assertRaisesRegex(SystemError, "fault"):
            ja[0:2] = 1
        _jpype.fault("PyJPArray_assignSubscript")
        with self.assertRaisesRegex(SystemError, "fault"):
            ja[0] = 1
        _jpype.fault("PyJPModule_getContext")
        with self.assertRaisesRegex(SystemError, "fault"):
            ja[0:2] = 1

    def testJPArray_assignSubscriptExc(self):
        ja = JArray(JInt)(5)
        with self.assertRaises(ValueError):
            del ja[0:2]
        with self.assertRaises(ValueError):
            ja[slice(0, 0, 0)] = 1

    # FIXME investigate why the release is not happening
    # may indicate a leak. Disabling for now
    @common.unittest.SkipTest
    def testJPArray_releaseBuffer(self):
        _jpype.fault("PyJPArray_releaseBuffer")

        def f():
            ja = JArray(JInt)(5)
            m = memoryview(ja)
        with self.assertRaises(SystemError):
            f()

    @common.requireInstrumentation
    def testJPArray_getBuffer(self):
        _jpype.fault("PyJPArray_getBuffer")
        with self.assertRaisesRegex(SystemError, "fault"):
            ja = JArray(JInt, 2)(5)
            m = memoryview(ja)
            del m  # lgtm [py/unnecessary-delete]

    @common.requireInstrumentation
    def testJPArrayPrimitive_getBuffer(self):
        _jpype.fault("PyJPArrayPrimitive_getBuffer")

        def f():
            ja = JArray(JInt)(5)
            m = memoryview(ja)
            del m  # lgtm [py/unnecessary-delete]
        with self.assertRaisesRegex(SystemError, "fault"):
            f()
        with self.assertRaises(BufferError):
            memoryview(JArray(JInt, 2)([[1, 2], [1], [1, 2, 3]]))
        _jpype.fault("PyJPModule_getContext")
        with self.assertRaisesRegex(SystemError, "fault"):
            f()

    @common.requireInstrumentation
    def testJPArray_null(self):
        _jpype.fault("PyJPArray_init.null")
        null = JArray(JInt)(object())
        with self.assertRaises(ValueError):
            repr(null)
        with self.assertRaises(ValueError):
            len(null)
        with self.assertRaises(ValueError):
            null[0]
        with self.assertRaises(ValueError):
            null[0] = 1
        with self.assertRaises(ValueError):
            memoryview(null)
        null = JArray(JObject)(object())
        with self.assertRaises(ValueError):
            memoryview(null)
        _jpype.fault("PyJPModule_getContext")
        with self.assertRaisesRegex(SystemError, "fault"):
            memoryview(null)

    @common.requireInstrumentation
    def testJPClass_new(self):
        _jpype.fault("PyJPClass_new")
        with self.assertRaisesRegex(SystemError, "fault"):
            _jpype._JClass("foo", (object,), {})
        with self.assertRaises(TypeError):
            _jpype._JClass("foo", (object,), {})
        with self.assertRaises(TypeError):
            _jpype._JClass("foo", (_jpype._JObject,), {'__del__': None})

    @common.requireInstrumentation
    def testJPClass_init(self):
        _jpype.fault("PyJPClass_init")
        with self.assertRaises(SystemError):
            _jpype._JClass("foo", (object,), {})
        with self.assertRaises(TypeError):
            _jpype._JClass("foo", (object,), {})
        _jpype._JClass("foo", (_jpype._JObject,), {})

    @common.requireInstrumentation
    def testJPClass_getattro(self):
        js = JClass("java.lang.String")
        _jpype.fault("PyJPClass_getattro")
        with self.assertRaisesRegex(SystemError, "fault"):
            js.foo
        with self.assertRaises(TypeError):
            getattr(js, object())
        _jpype.fault("PyJPModule_getContext")
        with self.assertRaisesRegex(SystemError, "fault"):
            js.substring

    @common.requireInstrumentation
    def testJPClass_setattro(self):
        js = JClass("java.lang.String")
        _jpype.fault("PyJPClass_setattro")
        with self.assertRaisesRegex(SystemError, "fault"):
            js.substring = 1
        with self.assertRaises(TypeError):
            setattr(js, object(), 1)
        with self.assertRaises(AttributeError):
            js.substring = None
        _jpype.fault("PyJPModule_getContext")
        with self.assertRaises(SystemError):
            js.substring = 1

    @common.requireInstrumentation
    def testJPClass_subclasscheck(self):
        js = JClass("java.lang.String")
        _jpype.fault("PyJPClass_subclasscheck")
        with self.assertRaisesRegex(SystemError, "fault"):
            issubclass(js, JObject)
        _jpype.fault("PyJPModule_getContext")
        with self.assertRaisesRegex(SystemError, "fault"):
            issubclass(js, JObject)

    @common.requireInstrumentation
    def testJPClass_class(self):
        js = JClass("java.lang.String")
        _jpype.fault("PyJPClass_class")
        with self.assertRaisesRegex(SystemError, "fault"):
            js.class_
        with self.assertRaises(AttributeError):
            _jpype._JClass("foo", (_jpype.JObject,), {}).class_
        _jpype.fault("PyJPModule_getContext")
        with self.assertRaisesRegex(SystemError, "fault"):
            js.class_

    @common.requireInstrumentation
    def testJPClass_setClass(self):
        js = JClass("java.lang.String")
        _jpype.fault("PyJPClass_setClass")
        with self.assertRaisesRegex(SystemError, "fault"):
            js.class_ = None
        with self.assertRaises(TypeError):
            js.class_ = None
        with self.assertRaises(TypeError):
            js.class_ = JObject()
        _jpype.fault("PyJPModule_getContext")
        with self.assertRaises(SystemError):
            js.class_ = None

    @common.requireInstrumentation
    def testJPClass_hints(self):
        js = JClass("java.lang.String")
        _jpype.fault("PyJPClass_hints")
        with self.assertRaisesRegex(SystemError, "fault"):
            js._hints
        _jpype.fault("PyJPModule_getContext")
        with self.assertRaisesRegex(SystemError, "fault"):
            js._hints
        self.assertIsInstance(js._hints, _jpype._JClassHints)

    @common.requireInstrumentation
    def testJPClass_setHints(self):
        js = JClass("java.lang.String")
        _jpype.fault("PyJPClass_setHints")
        with self.assertRaisesRegex(SystemError, "fault"):
            js._hints = None

    @common.requireInstrumentation
    def testJPClass_cnaConvertToJava(self):
        js = JClass("java.lang.String")
        _jpype.fault("PyJPClass_canConvertToJava")
        with self.assertRaisesRegex(SystemError, "fault"):
            js._canConvertToJava("f")
        _jpype.fault("PyJPModule_getContext")
        with self.assertRaisesRegex(SystemError, "fault"):
            js._canConvertToJava("f")
        js._canConvertToJava("f")

    @common.requireInstrumentation
    def testJPClass_cast(self):
        js = JClass("java.lang.String")
        _jpype.fault("PyJPClass_cast")
        with self.assertRaisesRegex(SystemError, "fault"):
            js._cast("f")
        with self.assertRaises(TypeError):
            js._cast(object())
        _jpype.fault("PyJPModule_getContext")
        with self.assertRaisesRegex(SystemError, "fault"):
            js._cast(JObject(None))
        js._cast(JObject(None))

    @common.requireInstrumentation
    def testJPClass_convertToJava(self):
        js = JClass("java.lang.String")
        _jpype.fault("PyJPClass_convertToJava")
        with self.assertRaisesRegex(SystemError, "fault"):
            js._convertToJava("f")
        with self.assertRaises(TypeError):
            js._convertToJava(object())
        _jpype.fault("PyJPModule_getContext")
        with self.assertRaisesRegex(SystemError, "fault"):
            js._convertToJava("f")
        js._convertToJava("f")

    @common.requireInstrumentation
    def testJPClassHints_new(self):
        _jpype.fault("PyJPClassHints_new")
        with self.assertRaisesRegex(SystemError, "fault"):
            _jpype._JClassHints()
        _jpype._JClassHints()

    @common.requireInstrumentation
    def testJPClassHints_init(self):
        _jpype.fault("PyJPClassHints_init")
        with self.assertRaisesRegex(SystemError, "fault"):
            _jpype._JClassHints()
        _jpype._JClassHints()

    @common.requireInstrumentation
    def testJPClassHints_str(self):
        _jpype.fault("PyJPClassHints_str")
        with self.assertRaisesRegex(SystemError, "fault"):
            str(_jpype._JClassHints())
        str(_jpype._JClassHints())

    @common.requireInstrumentation
    def testJPClassHints_addAttributeConversion(self):
        _jpype.fault("PyJPClassHints_addAttributeConversion")
        with self.assertRaisesRegex(SystemError, "fault"):
            _jpype._JClassHints().addAttributeConversion("f", None)

        def f():
            pass
        with self.assertRaises(TypeError):
            _jpype._JClassHints().addAttributeConversion(None, f)
        with self.assertRaises(TypeError):
            _jpype._JClassHints().addAttributeConversion("f", None)
        _jpype._JClassHints().addAttributeConversion("f", f)

    @common.requireInstrumentation
    def testJPClassHints_addTypeConversion(self):
        _jpype.fault("PyJPClassHints_addTypeConversion")
        with self.assertRaisesRegex(SystemError, "fault"):
            _jpype._JClassHints().addTypeConversion("f", None)

        def f():
            pass
        with self.assertRaises(TypeError):
            _jpype._JClassHints().addTypeConversion(None, f, 1)
        with self.assertRaises(TypeError):
            _jpype._JClassHints().addTypeConversion(str, None, 1)
        _jpype._JClassHints().addTypeConversion(str, f, 1)


# pyjp_method.cpp:	JP_PY_TRY("PyJPMethod_new");
# pyjp_method.cpp:	JP_PY_TRY("PyJPMethod_dealloc");
# pyjp_method.cpp:	JP_PY_TRY("PyJPMethod_get");
# pyjp_method.cpp:	JP_PY_TRY("PyJPMethod_call");
# pyjp_method.cpp:	JP_PY_TRY("PyJPMethod_str");
# pyjp_method.cpp:	JP_PY_TRY("PyJPMethod_repr");
# pyjp_method.cpp:	JP_PY_TRY("PyJPMethod_getSelf");
# pyjp_method.cpp:	JP_PY_TRY("PyJPMethod_getName");
# pyjp_method.cpp:	JP_PY_TRY("PyJPMethod_getQualName");
# pyjp_method.cpp:	JP_PY_TRY("PyJPMethod_getDoc");
# pyjp_method.cpp:	JP_PY_TRY("PyJPMethod_getDoc");
# pyjp_method.cpp:	JP_PY_TRY("PyJPMethod_getAnnotations");
# pyjp_method.cpp:	JP_PY_TRY("PyJPMethod_getCodeAttr");
# pyjp_method.cpp:	JP_PY_TRY("PyJPMethod_isBeanAccessor");
# pyjp_method.cpp:	JP_PY_TRY("PyJPMethod_isBeanMutator");
# pyjp_method.cpp:	JP_PY_TRY("PyJPMethod_matchReport");
# pyjp_method.cpp:	JP_PY_TRY("PyJPMethod_dump");
# pyjp_module.cpp:	JP_PY_TRY("Py_GetAttrDescriptor");
# pyjp_module.cpp:	JP_PY_TRY("PyJPModule_startup");
# pyjp_module.cpp:	JP_PY_TRY("PyJPModule_shutdown");
# pyjp_module.cpp:	JP_PY_TRY("PyJPModule_attachThread");
# pyjp_module.cpp:	JP_PY_TRY("PyJPModule_attachThreadAsDaemon");
# pyjp_module.cpp:	JP_PY_TRY("PyJPModule_detachThread");
# pyjp_module.cpp:	JP_PY_TRY("PyJPModule_isThreadAttached");
# pyjp_module.cpp:	JP_PY_TRY("PyJPModule_convertToDirectByteBuffer");
# pyjp_module.cpp:	JP_PY_TRY("PyJPModule_getArrayType");
# pyjp_module.cpp:	JP_PY_TRY("PyJPModule_getClass");
# pyjp_module.cpp:	JP_PY_TRY("PyJPModule_getClass");
# pyjp_module.cpp:	JP_PY_TRY("examine");
# pyjp_module.cpp:	JP_PY_TRY("PyInit__jpype");

    @common.requireInstrumentation
    def testJPMonitor_init(self):
        jo = JClass("java.lang.Object")()
        _jpype.fault("PyJPMonitor_init")
        with self.assertRaisesRegex(SystemError, "fault"):
            _jpype._JMonitor(jo)
        _jpype.fault("PyJPModule_getContext")
        with self.assertRaisesRegex(SystemError, "fault"):
            _jpype._JMonitor(jo)
        _jpype._JMonitor(jo)

    @common.requireInstrumentation
    def testJPMonitor_str(self):
        jo = JClass("java.lang.Object")()
        jm = _jpype._JMonitor(jo)
        _jpype.fault("PyJPMonitor_str")
        with self.assertRaisesRegex(SystemError, "fault"):
            str(jm)
        _jpype.fault("PyJPModule_getContext")
        with self.assertRaisesRegex(SystemError, "fault"):
            str(jm)

    @common.requireInstrumentation
    def testJPMonitor_enter(self):
        jo = JClass("java.lang.Object")()
        _jpype.fault("PyJPMonitor_enter")
        with self.assertRaisesRegex(SystemError, "fault"):
            with _jpype._JMonitor(jo):
                pass
        _jpype.fault("PyJPModule_getContext")
        with self.assertRaisesRegex(SystemError, "fault"):
            with _jpype._JMonitor(jo):
                pass

    @common.requireInstrumentation
    def testJPMonitor_exit(self):
        jo = JClass("java.lang.Object")()
        _jpype.fault("PyJPMonitor_exit")
        with self.assertRaisesRegex(SystemError, "fault"):
            with _jpype._JMonitor(jo):
                pass

    @common.requireInstrumentation
    def testJPNumber_new(self):
        _jpype.fault("PyJPNumber_new")

        class MyNum(_jpype._JNumberLong):
            pass
        with self.assertRaisesRegex(SystemError, "fault"):
            JInt(1)
        with self.assertRaises(TypeError):
            MyNum(1)
        _jpype.fault("PyJPModule_getContext")
        with self.assertRaisesRegex(SystemError, "fault"):
            JInt(1)
        JInt(1)

    @common.requireInstrumentation
    def testJPNumberLong_int(self):
        ji = JInt(1)
        _jpype.fault("PyJPNumberLong_int")
        with self.assertRaisesRegex(SystemError, "fault"):
            int(ji)
        _jpype.fault("PyJPModule_getContext")
        with self.assertRaisesRegex(SystemError, "fault"):
            int(ji)
        int(ji)

    @common.requireInstrumentation
    def testJPNumberLong_float(self):
        ji = JInt(1)
        _jpype.fault("PyJPNumberLong_float")
        with self.assertRaisesRegex(SystemError, "fault"):
            float(ji)
        _jpype.fault("PyJPModule_getContext")
        with self.assertRaisesRegex(SystemError, "fault"):
            float(ji)
        float(ji)

    @common.requireInstrumentation
    def testJPNumberLong_str(self):
        ji = JInt(1)
        _jpype.fault("PyJPNumberLong_str")
        with self.assertRaisesRegex(SystemError, "fault"):
            str(ji)
        _jpype.fault("PyJPModule_getContext")
        with self.assertRaisesRegex(SystemError, "fault"):
            str(ji)
        str(ji)

    @common.requireInstrumentation
    def testJPNumberLong_repr(self):
        ji = JInt(1)
        _jpype.fault("PyJPNumberLong_repr")
        with self.assertRaisesRegex(SystemError, "fault"):
            repr(ji)
        _jpype.fault("PyJPModule_getContext")
        with self.assertRaisesRegex(SystemError, "fault"):
            repr(ji)
        repr(ji)

    @common.requireInstrumentation
    def testJPNumberLong_compare(self):
        ji = JInt(1)
        _jpype.fault("PyJPNumberLong_compare")
        with self.assertRaisesRegex(SystemError, "fault"):
            ji == 1
        _jpype.fault("PyJPModule_getContext")
        with self.assertRaisesRegex(SystemError, "fault"):
            ji == 1
        ji == 1

    @common.requireInstrumentation
    def testJPNumberLong_hash(self):
        ji = JInt(1)
        _jpype.fault("PyJPNumberLong_hash")
        with self.assertRaises(SystemError):
            hash(ji)
        _jpype.fault("PyJPModule_getContext")
        with self.assertRaises(SystemError):
            hash(ji)
        hash(ji)

    @common.requireInstrumentation
    def testJPObject_new(self):
        _jpype.fault("PyJPObject_new")
        with self.assertRaisesRegex(SystemError, "fault"):
            JString("a")
        _jpype.fault("PyJPModule_getContext")
        with self.assertRaisesRegex(SystemError, "fault"):
            JString("a")
        with self.assertRaises(TypeError):
            _jpype._JObject()
        JString("a")

    @common.requireInstrumentation
    def testJPObject_hash(self):
        jo = JClass("java.lang.Object")()
        _jpype.fault("PyJPObject_hash")
        with self.assertRaises(SystemError):
            hash(jo)
        _jpype.fault("PyJPModule_getContext")
        with self.assertRaises(SystemError):
            hash(jo)
        hash(jo)

    @common.requireInstrumentation
    def testJPProxy_new(self):
        _jpype.fault("PyJPProxy_new")
        with self.assertRaisesRegex(SystemError, "fault"):
            JProxy("java.io.Serializable", dict={})
        with self.assertRaises(TypeError):
            _jpype._JProxy(None, None)
        with self.assertRaises(TypeError):
            _jpype._JProxy(None, [])
        with self.assertRaises(TypeError):
            _jpype._JProxy(None, [type])
        _jpype.fault("JPProxy::JPProxy")
        with self.assertRaises(SystemError):
            _jpype._JProxy(None, [JClass("java.io.Serializable")])
        _jpype._JProxy(None, [JClass("java.io.Serializable")])

    # FIXME this needs special treatment. It should call __str__()
    # if toString is not defined.  Disable for now.
    @common.unittest.SkipTest
    def testJPProxy_str(self):
        # Java has a hidden requirement that toString be available
        @JImplements("java.util.function.DoubleUnaryOperator")
        class f(object):
            @JOverride
            def applyAsDouble(self, d):
                return d
        jo = JObject(f(), "java.util.function.DoubleUnaryOperator")
        raise RuntimeError(jo.toString())

    @common.requireInstrumentation
    def testJPProxy_dealloc(self):
        _jpype.fault("PyJPProxy_dealloc")

        def f():
            _jpype._JProxy(None, [JClass("java.io.Serializable")])
        f()

    @common.requireInstrumentation
    def testJPProxy_call(self):
        @JImplements("java.util.function.DoubleUnaryOperator")
        class f(object):
            @JOverride
            def applyAsDouble(self, d):
                if d == 2:
                    return None
                return d
        _jpype.fault("JPProxy::getProxy")
        with self.assertRaises(SystemError):
            JObject(f(), "java.util.function.DoubleUnaryOperator")
        jo = JObject(f(), "java.util.function.DoubleUnaryOperator")
        # FIXME special case Java does not reflect the SystemError back to Python
        # Instead it gets changed to a RuntimeError.  We will correct this
        # once we have pass through of Python exceptions.
        _jpype.fault("PyJPProxy_getCallable")
        with self.assertRaises(jpype.JException):
            jo.applyAsDouble(1)
        with self.assertRaises(jpype.JException):
            jo.applyAsDouble(2)

    def testJPProxy_void(self):
        @JImplements("java.util.function.Consumer")
        class f(object):
            @JOverride
            def accept(self, d):
                return None
        jo = JObject(f(), "java.util.function.Consumer")
        jo.accept(None)

    @common.requireInstrumentation
    def testJPProxy_void(self):
        @JImplements("java.util.function.Consumer")
        class f(object):
            @JOverride
            def accept(self, d):
                return None
        jo = JObject(f(), "java.util.function.Consumer")
        _jpype.fault("JPProxy::getArgs")
        jo.accept(None)

    @common.requireInstrumentation
    def testJPProxy_box_return(self):
        q = None
        @JImplements("java.util.function.Supplier")
        class f(object):
            @JOverride
            def get(self):
                return q
        jo = JObject(f(), "java.util.function.Supplier")
        self.assertEqual(jo.get(), None)
        q = 1.0
        self.assertIsInstance(jo.get(), java.lang.Double)
        q = 1
        self.assertIsInstance(jo.get(), java.lang.Long)
        q = "ABC"
        self.assertIsInstance(jo.get(), java.lang.String)
        q = object()
        with self.assertRaises(jpype.JException):
            jo.get()

    @common.requireInstrumentation
    def testJPValue_alloc(self):
        _jpype.fault("PyJPValue_alloc")
        with self.assertRaisesRegex(SystemError, "fault"):
            JInt(1)
        _jpype.fault("PyJPModule_getContext")
        with self.assertRaisesRegex(SystemError, "fault"):
            JInt(1)
        JInt(1)

    @common.requireInstrumentation
    def testJPValue_finalize(self):
        _jpype.fault("PyJPValue_finalize")
        a = JInt(1)
        del a  # lgtm [py/unnecessary-delete]

    @common.requireInstrumentation
    def testJPValue_str(self):
        js = JString("f")
        _jpype.fault("PyJPValue_str")
        with self.assertRaisesRegex(SystemError, "fault"):
            str(js)
        _jpype.fault("PyJPModule_getContext")
        with self.assertRaisesRegex(SystemError, "fault"):
            str(js)
        str(js)

    @common.requireInstrumentation
    def testJPObject_getattro(self):
        jo = JString("f")
        _jpype.fault("PyJPObject_getattro")
        with self.assertRaisesRegex(SystemError, "fault"):
            jo.substring
        _jpype.fault("PyJPModule_getContext")
        with self.assertRaisesRegex(SystemError, "fault"):
            jo.substring
        jo.substring

    @common.requireInstrumentation
    def testJPObject_setattro(self):
        jo = JString("f")
        _jpype.fault("PyJPObject_setattro")
        with self.assertRaisesRegex(SystemError, "fault"):
            jo.substring = None

    @common.requireInstrumentation
    def testJPField(self):
        jf = JClass("jpype.common.Fixture")
        jfi = jf()
        with self.assertRaises(AttributeError):
            jf.final_static_int_field = 2
        with self.assertRaises(AttributeError):
            jfi.final_int_field = 2
        _jpype.fault("JPField::setStaticAttribute")
        with self.assertRaisesRegex(SystemError, "fault"):
            jf.static_int_field = 2
        _jpype.fault("JPField::setAttribute")
        with self.assertRaisesRegex(SystemError, "fault"):
            jfi.int_field = 2
        _jpype.fault("JPField::getStaticAttribute")
        with self.assertRaisesRegex(SystemError, "fault"):
            i = jf.static_int_field
        _jpype.fault("JPField::getAttribute")
        with self.assertRaisesRegex(SystemError, "fault"):
            i = jfi.int_field
        si = jf.__dict__['static_int_field']
        str(si)
        repr(si)
        i = None
        _jpype.fault("PyJPField_get")
        with self.assertRaisesRegex(SystemError, "fault"):
            i = jfi.int_field
        self.assertEqual(i, None)
        _jpype.fault("PyJPField_set")
        with self.assertRaisesRegex(SystemError, "fault"):
            jfi.int_field = 2
        _jpype.fault("PyJPField_repr")
        with self.assertRaisesRegex(SystemError, "fault"):
            repr(si)

    @common.requireInstrumentation
    def testConvertString(self):
        _jpype.fault("JPObjectType::canConvertToJava")
        with self.assertRaisesRegex(SystemError, "fault"):
            JObject._convertToJava("foo")
        _jpype.fault("JPConversionString::matches")
        with self.assertRaisesRegex(SystemError, "fault"):
            JString._convertToJava("foo")

    @common.requireInstrumentation
    def testJPObject(self):
        jf = JClass("jpype.common.Fixture")
        jfi = JClass("jpype.common.Fixture")()
        _jpype.fault("JPClass::setStaticField")
        with self.assertRaisesRegex(SystemError, "fault"):
            jf.static_object_field = None
        _jpype.fault("JPClass::setField")
        with self.assertRaisesRegex(SystemError, "fault"):
            jfi.object_field = None
        i = None
        _jpype.fault("JPClass::getStaticField")
        with self.assertRaisesRegex(SystemError, "fault"):
            i = jf.static_object_field
        _jpype.fault("JPClass::getField")
        with self.assertRaisesRegex(SystemError, "fault"):
            i = jfi.object_field
        self.assertEqual(i, None)

    # FIXME this test triggers the wrong fault which normally
    # indicates a problem in the exception handling path.
    # AssertionError: "fault" does not match "NULL context in JPRef()
    # Disabling for now.
    @common.unittest.SkipTest
    @common.requireInstrumentation
    def testJPTypeManagerFindClass(self):
        ja = JArray(JInt, 2)([[1, 1], [1, 1]])
        _jpype.fault("JPTypeManager::findClass")
        with self.assertRaisesRegex(SystemError, "fault"):
            memoryview(ja)
        _jpype.fault("JPTypeManager::findClassByName")
        with self.assertRaisesRegex(SystemError, "fault"):
            JClass("foo.bar")
        jo = JString('a')
        _jpype.fault("JPTypeManager::findClassForObject")
        with self.assertRaisesRegex(SystemError, "fault"):
            jo.substring(0, 1)

    @common.requireInstrumentation
    def testJPTypeManagerPopulate(self):
        _jpype.fault("JPTypeManager::populateMembers")
        with self.assertRaisesRegex(SystemError, "fault"):
            JClass("java.math.MathContext")
        _jpype.fault("JPTypeManager::populateMethod")
        with self.assertRaisesRegex(SystemError, "fault"):
            JClass("java.math.MathContext")().getPrecision()
        JClass("java.math.MathContext")

    @common.requireInstrumentation
    def testJPArrayNew(self):
        ja = JArray(JInt)
        _jpype.fault("JPArray::JPArray")
        with self.assertRaisesRegex(SystemError, "fault"):
            ja(5)
        j = ja(5)
        _jpype.fault("JPArray::JPArraySlice")
        with self.assertRaisesRegex(SystemError, "fault"):
            j[0:2:1]

    @common.requireInstrumentation
    def testMethodPack(self):
        js = JString("a")
        _jpype.fault("JPMethod::packArgs")
        with self.assertRaisesRegex(SystemError, "fault"):
            js.substring(1)

    @common.requireInstrumentation
    def testJArrayClassConvertToVector(self):
        Path = JClass("java.nio.file.Paths")
        _jpype.fault("JPArrayClass::convertToJavaVector")
        with self.assertRaisesRegex(SystemError, "fault"):
            Path.get("foo", "bar")

    @common.requireInstrumentation
    def testJArrayGetJavaConversion(self):
        ja = JArray(JInt)
        _jpype.fault("JPArrayClass::getJavaConversion")
        with self.assertRaisesRegex(SystemError, "fault"):
            ja._canConvertToJava(object())

    @common.requireInstrumentation
    def testJArrayConvertToPythonObject(self):
        jl = JClass('java.util.ArrayList')()
        jl.add(JArray(JInt)(3))
        _jpype.fault("JPArrayClass::convertToPythonObject")
        with self.assertRaisesRegex(SystemError, "fault"):
            jl.get(0)

    @common.requireInstrumentation
    def testJBoxedGetJavaConversion(self):
        _jpype.fault("JPBoxedType::getJavaConversion")
        with self.assertRaisesRegex(SystemError, "fault"):
            java.lang.Boolean._canConvertToJava(object())
        _jpype.fault("JPBoxedType::getJavaConversion")
        with self.assertRaisesRegex(SystemError, "fault"):
            java.lang.Character._canConvertToJava(object())
        _jpype.fault("JPBoxedType::getJavaConversion")
        with self.assertRaisesRegex(SystemError, "fault"):
            java.lang.Byte._canConvertToJava(object())
        _jpype.fault("JPBoxedType::getJavaConversion")
        with self.assertRaisesRegex(SystemError, "fault"):
            java.lang.Short._canConvertToJava(object())
        _jpype.fault("JPBoxedType::getJavaConversion")
        with self.assertRaisesRegex(SystemError, "fault"):
            java.lang.Integer._canConvertToJava(object())
        _jpype.fault("JPBoxedType::getJavaConversion")
        with self.assertRaisesRegex(SystemError, "fault"):
            java.lang.Long._canConvertToJava(object())
        _jpype.fault("JPBoxedType::getJavaConversion")
        with self.assertRaisesRegex(SystemError, "fault"):
            java.lang.Float._canConvertToJava(object())
        _jpype.fault("JPBoxedType::getJavaConversion")
        with self.assertRaisesRegex(SystemError, "fault"):
            java.lang.Double._canConvertToJava(object())

    @common.requireInstrumentation
    def testJPJavaFrame(self):
        _jpype.fault("JPJavaFrame::JPJavaFrame::NewObjectA")
        with self.assertRaisesRegex(SystemError, "fault"):
            raise SystemError("fault")
        _jpype.fault("JPJavaFrame::JPJavaFrame::NewObject")
        with self.assertRaisesRegex(SystemError, "fault"):
            raise SystemError("fault")
        _jpype.fault("JPJavaFrame::NewDirectByteBuffer")
        with self.assertRaisesRegex(SystemError, "fault"):
            raise SystemError("fault")
        _jpype.fault("JPJavaFrame::GetPrimitiveArrayCritical")
        with self.assertRaisesRegex(SystemError, "fault"):
            raise SystemError("fault")
        _jpype.fault("JPJavaFrame::ReleasePrimitiveArrayCritical")
        with self.assertRaisesRegex(SystemError, "fault"):
            raise SystemError("fault")

    @common.requireInstrumentation
    def testJPJavaFrameByteField(self):
        fields = JClass("jpype.common.Fixture")()
        _jpype.fault("JPJavaFrame::GetStaticByteField")
        with self.assertRaisesRegex(SystemError, "fault"):
            print(fields.static_byte_field)
        _jpype.fault("JPJavaFrame::GetByteField")
        with self.assertRaisesRegex(SystemError, "fault"):
            print(fields.byte_field)
        _jpype.fault("JPJavaFrame::SetStaticByteField")
        with self.assertRaisesRegex(SystemError, "fault"):
            fields.static_byte_field = 1
        _jpype.fault("JPJavaFrame::SetByteField")
        with self.assertRaisesRegex(SystemError, "fault"):
            fields.byte_field = 0

    @common.requireInstrumentation
    def testJPJavaFrameByteMethods(self):
        cls = JClass("jpype.common.Fixture")
        obj = cls()
        _jpype.fault("JPJavaFrame::CallStaticByteMethodA")
        with self.assertRaisesRegex(SystemError, "fault"):
            cls.getStaticByte()
        _jpype.fault("JPJavaFrame::CallByteMethodA")
        with self.assertRaisesRegex(SystemError, "fault"):
            obj.getByte()
        _jpype.fault("JPJavaFrame::CallNonvirtualByteMethodA")
        with self.assertRaisesRegex(SystemError, "fault"):
            cls.getByte(obj)

    @common.requireInstrumentation
    def testJPJavaFrameShortField(self):
        fields = JClass("jpype.common.Fixture")()
        _jpype.fault("JPJavaFrame::GetStaticShortField")
        with self.assertRaisesRegex(SystemError, "fault"):
            print(fields.static_short_field)
        _jpype.fault("JPJavaFrame::GetShortField")
        with self.assertRaisesRegex(SystemError, "fault"):
            print(fields.short_field)
        _jpype.fault("JPJavaFrame::SetStaticShortField")
        with self.assertRaisesRegex(SystemError, "fault"):
            fields.static_short_field = 1
        _jpype.fault("JPJavaFrame::SetShortField")
        with self.assertRaisesRegex(SystemError, "fault"):
            fields.short_field = 1

    @common.requireInstrumentation
    def testJPJavaFrameShortMethod(self):
        cls = JClass("jpype.common.Fixture")
        obj = cls()
        _jpype.fault("JPJavaFrame::CallStaticShortMethodA")
        with self.assertRaisesRegex(SystemError, "fault"):
            cls.getStaticShort()
        _jpype.fault("JPJavaFrame::CallShortMethodA")
        with self.assertRaisesRegex(SystemError, "fault"):
            obj.getShort()
        _jpype.fault("JPJavaFrame::CallNonvirtualShortMethodA")
        with self.assertRaisesRegex(SystemError, "fault"):
            cls.getShort(obj)

    @common.requireInstrumentation
    def testJPJavaFrameIntField(self):
        fields = JClass("jpype.common.Fixture")()
        _jpype.fault("JPJavaFrame::GetStaticIntField")
        with self.assertRaisesRegex(SystemError, "fault"):
            print(fields.static_int_field)
        _jpype.fault("JPJavaFrame::GetIntField")
        with self.assertRaisesRegex(SystemError, "fault"):
            print(fields.int_field)
        _jpype.fault("JPJavaFrame::SetStaticIntField")
        with self.assertRaisesRegex(SystemError, "fault"):
            fields.static_int_field = 1
        _jpype.fault("JPJavaFrame::SetIntField")
        with self.assertRaisesRegex(SystemError, "fault"):
            fields.int_field = 1

    @common.requireInstrumentation
    def testJPJavaFrameIntMethod(self):
        cls = JClass("jpype.common.Fixture")
        obj = cls()
        _jpype.fault("JPJavaFrame::CallStaticIntMethodA")
        with self.assertRaisesRegex(SystemError, "fault"):
            cls.getStaticInt()
        _jpype.fault("JPJavaFrame::CallIntMethodA")
        with self.assertRaisesRegex(SystemError, "fault"):
            obj.getInt()
        _jpype.fault("JPJavaFrame::CallNonvirtualIntMethodA")
        with self.assertRaisesRegex(SystemError, "fault"):
            cls.getInt(obj)

    @common.requireInstrumentation
    def testJPJavaFrameLongField(self):
        fields = JClass("jpype.common.Fixture")()
        _jpype.fault("JPJavaFrame::GetStaticLongField")
        with self.assertRaisesRegex(SystemError, "fault"):
            print(fields.static_long_field)
        _jpype.fault("JPJavaFrame::GetLongField")
        with self.assertRaisesRegex(SystemError, "fault"):
            print(fields.long_field)
        _jpype.fault("JPJavaFrame::SetStaticLongField")
        with self.assertRaisesRegex(SystemError, "fault"):
            fields.static_long_field = 1
        _jpype.fault("JPJavaFrame::SetLongField")
        with self.assertRaisesRegex(SystemError, "fault"):
            fields.long_field = 1

    @common.requireInstrumentation
    def testJPJavaFrameLongMethod(self):
        cls = JClass("jpype.common.Fixture")
        obj = cls()
        _jpype.fault("JPJavaFrame::CallStaticLongMethodA")
        with self.assertRaisesRegex(SystemError, "fault"):
            cls.getStaticLong()
        _jpype.fault("JPJavaFrame::CallLongMethodA")
        with self.assertRaisesRegex(SystemError, "fault"):
            obj.getLong()
        _jpype.fault("JPJavaFrame::CallNonvirtualLongMethodA")
        with self.assertRaisesRegex(SystemError, "fault"):
            cls.getLong(obj)

    @common.requireInstrumentation
    def testJPJavaFrameFloatField(self):
        fields = JClass("jpype.common.Fixture")()
        _jpype.fault("JPJavaFrame::GetStaticFloatField")
        with self.assertRaisesRegex(SystemError, "fault"):
            print(fields.static_float_field)
        _jpype.fault("JPJavaFrame::GetFloatField")
        with self.assertRaisesRegex(SystemError, "fault"):
            print(fields.float_field)
        _jpype.fault("JPJavaFrame::SetStaticFloatField")
        with self.assertRaisesRegex(SystemError, "fault"):
            fields.static_float_field = 1
        _jpype.fault("JPJavaFrame::SetFloatField")
        with self.assertRaisesRegex(SystemError, "fault"):
            fields.float_field = 1

    @common.requireInstrumentation
    def testJPJavaFrameFloatMethod(self):
        cls = JClass("jpype.common.Fixture")
        obj = cls()
        _jpype.fault("JPJavaFrame::CallStaticFloatMethodA")
        with self.assertRaisesRegex(SystemError, "fault"):
            cls.getStaticFloat()
        _jpype.fault("JPJavaFrame::CallFloatMethodA")
        with self.assertRaisesRegex(SystemError, "fault"):
            obj.getFloat()
        _jpype.fault("JPJavaFrame::CallNonvirtualFloatMethodA")
        with self.assertRaisesRegex(SystemError, "fault"):
            cls.getFloat(obj)

    @common.requireInstrumentation
    def testJPJavaFrameDoubleField(self):
        fields = JClass("jpype.common.Fixture")()
        _jpype.fault("JPJavaFrame::GetStaticDoubleField")
        with self.assertRaisesRegex(SystemError, "fault"):
            print(fields.static_double_field)
        _jpype.fault("JPJavaFrame::GetDoubleField")
        with self.assertRaisesRegex(SystemError, "fault"):
            print(fields.double_field)
        _jpype.fault("JPJavaFrame::SetStaticDoubleField")
        with self.assertRaisesRegex(SystemError, "fault"):
            fields.static_double_field = 1
        _jpype.fault("JPJavaFrame::SetDoubleField")
        with self.assertRaisesRegex(SystemError, "fault"):
            fields.double_field = 1

    @common.requireInstrumentation
    def testJPJavaFrameDoubleMethod(self):
        cls = JClass("jpype.common.Fixture")
        obj = cls()
        _jpype.fault("JPJavaFrame::CallStaticDoubleMethodA")
        with self.assertRaisesRegex(SystemError, "fault"):
            cls.getStaticDouble()
        _jpype.fault("JPJavaFrame::CallDoubleMethodA")
        with self.assertRaisesRegex(SystemError, "fault"):
            obj.getDouble()
        _jpype.fault("JPJavaFrame::CallNonvirtualDoubleMethodA")
        with self.assertRaisesRegex(SystemError, "fault"):
            cls.getDouble(obj)

    @common.requireInstrumentation
    def testJPJavaFrameCharField(self):
        fields = JClass("jpype.common.Fixture")()
        _jpype.fault("JPJavaFrame::GetStaticCharField")
        with self.assertRaisesRegex(SystemError, "fault"):
            print(fields.static_char_field)
        _jpype.fault("JPJavaFrame::GetCharField")
        with self.assertRaisesRegex(SystemError, "fault"):
            print(fields.char_field)
        _jpype.fault("JPJavaFrame::SetStaticCharField")
        with self.assertRaisesRegex(SystemError, "fault"):
            fields.static_char_field = 1
        _jpype.fault("JPJavaFrame::SetCharField")
        with self.assertRaisesRegex(SystemError, "fault"):
            fields.char_field = 1

    @common.requireInstrumentation
    def testJPJavaFrameCharMethod(self):
        cls = JClass("jpype.common.Fixture")
        obj = cls()
        _jpype.fault("JPJavaFrame::CallStaticCharMethodA")
        with self.assertRaisesRegex(SystemError, "fault"):
            cls.getStaticChar()
        _jpype.fault("JPJavaFrame::CallCharMethodA")
        with self.assertRaisesRegex(SystemError, "fault"):
            obj.getChar()
        _jpype.fault("JPJavaFrame::CallNonvirtualCharMethodA")
        with self.assertRaisesRegex(SystemError, "fault"):
            cls.getChar(obj)

    @common.requireInstrumentation
    def testJPJavaFrameBooleanField(self):
        fields = JClass("jpype.common.Fixture")()
        _jpype.fault("JPJavaFrame::GetStaticBooleanField")
        with self.assertRaisesRegex(SystemError, "fault"):
            print(fields.static_bool_field)
        _jpype.fault("JPJavaFrame::GetBooleanField")
        with self.assertRaisesRegex(SystemError, "fault"):
            print(fields.bool_field)
        _jpype.fault("JPJavaFrame::SetStaticBooleanField")
        with self.assertRaisesRegex(SystemError, "fault"):
            fields.static_bool_field = 1
        _jpype.fault("JPJavaFrame::SetBooleanField")
        with self.assertRaisesRegex(SystemError, "fault"):
            fields.bool_field = 1

    @common.requireInstrumentation
    def testJPJavaFrameBooleanMethod(self):
        cls = JClass("jpype.common.Fixture")
        obj = cls()
        _jpype.fault("JPJavaFrame::CallStaticBooleanMethodA")
        with self.assertRaisesRegex(SystemError, "fault"):
            cls.getStaticBool()
        _jpype.fault("JPJavaFrame::CallBooleanMethodA")
        with self.assertRaisesRegex(SystemError, "fault"):
            obj.getBool()
        _jpype.fault("JPJavaFrame::CallNonvirtualBooleanMethodA")
        with self.assertRaisesRegex(SystemError, "fault"):
            cls.getBool(obj)

    @common.requireInstrumentation
    def testJPJavaFrameObjectField(self):
        fields = JClass("jpype.common.Fixture")()
        _jpype.fault("JPJavaFrame::GetStaticObjectField")
        with self.assertRaisesRegex(SystemError, "fault"):
            print(fields.static_object_field)
        _jpype.fault("JPJavaFrame::GetObjectField")
        with self.assertRaisesRegex(SystemError, "fault"):
            print(fields.object_field)
        _jpype.fault("JPJavaFrame::SetStaticObjectField")
        with self.assertRaisesRegex(SystemError, "fault"):
            fields.static_object_field = None
        _jpype.fault("JPJavaFrame::SetObjectField")
        with self.assertRaisesRegex(SystemError, "fault"):
            fields.object_field = None

    @common.requireInstrumentation
    def testJPJavaFrameObjectMethod(self):
        cls = JClass("jpype.common.Fixture")
        obj = cls()
        _jpype.fault("JPJavaFrame::CallStaticObjectMethodA")
        with self.assertRaisesRegex(SystemError, "fault"):
            cls.getStaticObject()
        _jpype.fault("JPJavaFrame::CallObjectMethodA")
        with self.assertRaisesRegex(SystemError, "fault"):
            obj.getObject()
        _jpype.fault("JPJavaFrame::CallNonvirtualObjectMethodA")
        with self.assertRaisesRegex(SystemError, "fault"):
            cls.getObject(obj)

    @common.requireInstrumentation
    def testJPJavaFrameBooleanArray(self):
        _jpype.fault("JPJavaFrame::NewBooleanArray")
        with self.assertRaisesRegex(SystemError, "fault"):
            JArray(JBoolean)(1)
        ja = JArray(JBoolean)(5)
        _jpype.fault("JPJavaFrame::SetBooleanArrayRegion")
        with self.assertRaisesRegex(SystemError, "fault"):
            ja[0] = 0
        _jpype.fault("JPJavaFrame::GetBooleanArrayRegion")
        with self.assertRaisesRegex(SystemError, "fault"):
            print(ja[0])
        _jpype.fault("JPJavaFrame::GetBooleanArrayElements")
        with self.assertRaisesRegex(SystemError, "fault"):
            memoryview(ja[0:3])
        _jpype.fault("JPJavaFrame::ReleaseBooleanArrayElements")
        with self.assertRaisesRegex(SystemError, "fault"):
            ja[0:3] = bytes([1, 2, 3])
        _jpype.fault("JPJavaFrame::ReleaseBooleanArrayElements")
        with self.assertRaisesRegex(SystemError, "fault"):
            jpype.JObject(ja[::2], jpype.JObject)
        _jpype.fault("JPJavaFrame::ReleaseBooleanArrayElements")

        def f():
            # Special case no fault is allowed
            memoryview(ja[0:3])
        f()

    @common.requireInstrumentation
    def testJPJavaFrameMonitor(self):
        jo = JClass("java.lang.Object")()
        _jpype.fault("JPJavaFrame::MonitorEnter")
        with self.assertRaisesRegex(SystemError, "fault"):
            with syncronized(jo):
                pass
        _jpype.fault("JPJavaFrame::MonitorExit")
        with self.assertRaisesRegex(SystemError, "fault"):
            with syncronized(jo):
                pass

    @common.requireInstrumentation
    def testJPJavaFrameMonitor(self):
        _jpype.fault("JPJavaFrame::FromReflectedMethod")
        with self.assertRaisesRegex(SystemError, "fault"):
            raise SystemError("fault")
        _jpype.fault("JPJavaFrame::FromReflectedField")
        with self.assertRaisesRegex(SystemError, "fault"):
            raise SystemError("fault")
        _jpype.fault("JPJavaFrame::FindClass")
        with self.assertRaisesRegex(SystemError, "fault"):
            raise SystemError("fault")

    @common.requireInstrumentation
    def testJPJavaFrameObjectArray(self):
        _jpype.fault("JPJavaFrame::NewObjectArray")
        with self.assertRaisesRegex(SystemError, "fault"):
            JArray(JObject)(1)
        ja = JArray(JObject)(1)
        _jpype.fault("JPJavaFrame::SetObjectArrayElement")
        with self.assertRaisesRegex(SystemError, "fault"):
            ja[0] = None
        _jpype.fault("JPJavaFrame::GetObjectArrayElement")
        with self.assertRaisesRegex(SystemError, "fault"):
            print(ja[0])

    @common.requireInstrumentation
    def testJPJavaFrameVoidMethod(self):
        _jpype.fault("JPJavaFrame::CallStaticVoidMethodA")
        with self.assertRaisesRegex(SystemError, "fault"):
            raise SystemError("fault")
        _jpype.fault("JPJavaFrame::CallVoidMethodA")
        with self.assertRaisesRegex(SystemError, "fault"):
            raise SystemError("fault")
        _jpype.fault("JPJavaFrame::CallVoidMethodA")
        with self.assertRaisesRegex(SystemError, "fault"):
            raise SystemError("fault")
        _jpype.fault("None")

    @common.requireInstrumentation
    def testJPJavaFrameAssignable(self):
        _jpype.fault("JPJavaFrame::IsAssignableFrom")
        with self.assertRaisesRegex(SystemError, "fault"):
            issubclass(JString, JObject)

    @common.requireInstrumentation
    def testJPJavaFrameString(self):
        _jpype.fault("JPJavaFrame::NewString")
        with self.assertRaisesRegex(SystemError, "fault"):
            JString("aa")
        _jpype.fault("JPJavaFrame::GetStringUTFChars")
        with self.assertRaisesRegex(SystemError, "fault"):
            str(JString("a"))
        _jpype.fault("JPJavaFrame::ReleaseStringUTFChars")
        # Releas must not pass the exception because it would
        # cause an abort.
        str(JString("a"))
        _jpype.fault("JPJavaFrame::GetStringUTFLength")
        with self.assertRaisesRegex(SystemError, "fault"):
            str(JString("a"))

    @common.requireInstrumentation
    def testJPJavaFrameArrayLength(self):
        _jpype.fault("JPJavaFrame::GetArrayLength")
        with self.assertRaisesRegex(SystemError, "fault"):
            JArray(JInt)(5)

# These are very hard to hit as they are all startup routines
#        _jpype.fault("JPJavaFrame::GetMethodID")
#        with self.assertRaisesRegex(SystemError, "fault"):
#            raise SystemError("fault")
#        _jpype.fault("JPJavaFrame::GetStaticMethodID")
#        with self.assertRaisesRegex(SystemError, "fault"):
#            raise SystemError("fault")
#        _jpype.fault("JPJavaFrame::GetFieldID")
#        with self.assertRaisesRegex(SystemError, "fault"):
#            raise SystemError("fault")
#        _jpype.fault("JPJavaFrame::DefineClass")
#        with self.assertRaisesRegex(SystemError, "fault"):
#            raise SystemError("fault")
#        _jpype.fault("JPJavaFrame::RegisterNatives")
#        with self.assertRaisesRegex(SystemError, "fault"):
#            raise SystemError("fault")

    @common.requireInstrumentation
    def testJPClassTypeGetJavaConversion(self):
        jc = JClass("java.lang.StringBuilder")
        _jpype.fault("JPClass::getJavaConversion")
        with self.assertRaisesRegex(SystemError, "fault"):
            jc._canConvertToJava(object())

        @jpype.JConversion("java.lang.StringBuilder", exact=object)
        def f(self, obj):
            raise SystemError("fail")
        with self.assertRaisesRegex(SystemError, "fail"):
            jc._convertToJava(object())
