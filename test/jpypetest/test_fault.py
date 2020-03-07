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

    """

    def setUp(self):
        common.JPypeTestCase.setUp(self)
        if not hasattr(_jpype, "fault"):
            raise common.unittest.SkipTest("no instrumentation")

    def testJPArray_new(self):
        _jpype.fault("PyJPArray_new")
        with self.assertRaisesRegex(SystemError, "fault"):
            JArray(JInt)(5)

    def testJPArray_init(self):
        _jpype.fault("PyJPArray_init")
        with self.assertRaisesRegex(SystemError, "fault"):
            JArray(JInt)(5)
        with self.assertRaises(TypeError):
            _jpype._JArray("foo")
        with self.assertRaises(TypeError):
            JArray(JInt)(JArray(JDouble)([1,2]))
        with self.assertRaises(TypeError):
            JArray(JInt)(JString)
        with self.assertRaises(ValueError):
            JArray(JInt)(-1)
        with self.assertRaises(ValueError):
            JArray(JInt)(10000000000)
        with self.assertRaises(TypeError):
            JArray(JInt)(object())
        self.assertEqual(len(JArray(JInt)(0)),0)
        self.assertEqual(len(JArray(JInt)(10)),10)
        self.assertEqual(len(JArray(JInt)([1,2,3])),3)
        self.assertEqual(len(JArray(JInt)(JArray(JInt)([1,2,3]))), 3)
        class badlist(list):
            def __len__(self):
                return -1
        with self.assertRaises(ValueError):
            JArray(JInt)(badlist([1,2,3]))
        _jpype.fault("PyJPModule_getContext")
        with self.assertRaisesRegex(SystemError, "fault"):
            JArray(JInt)(5)

    def testJPArray_repr(self):
        ja = JArray(JInt)(5)
        _jpype.fault("PyJPArray_repr")
        with self.assertRaisesRegex(SystemError, "fault"):
            repr(ja)
        _jpype.fault("PyJPModule_getContext")
        with self.assertRaisesRegex(SystemError, "fault"):
            repr(ja)

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

    def testJPArray_getArrayItem(self):
        ja = JArray(JInt)(5)
        _jpype.fault("PyJPArray_getArrayItem")
        with self.assertRaisesRegex(SystemError, "fault"):
            ja[0]
        with self.assertRaises(TypeError):
            ja[object()]
        with self.assertRaises(ValueError):
            ja[slice(0,0,0)]
        self.assertEqual(len(JArray(JInt)(5)[4:1]),0)
        _jpype.fault("PyJPModule_getContext")
        with self.assertRaises(SystemError):
            ja[0]

    def testJPArray_assignSubscript(self):
        ja = JArray(JInt)(5)
        _jpype.fault("PyJPArray_assignSubscript")
        with self.assertRaisesRegex(SystemError, "fault"):
            ja[0:2] = 1
        _jpype.fault("PyJPArray_assignSubscript")
        with self.assertRaisesRegex(SystemError, "fault"):
            ja[0] = 1
        with self.assertRaises(ValueError):
            del ja[0:2]
        with self.assertRaises(ValueError):
            ja[slice(0,0,0)] = 1
        _jpype.fault("PyJPModule_getContext")
        with self.assertRaises(SystemError):
            ja[0:2] = 1

# FIXME investigate why the release is not happening
#    def testJPArray_releaseBuffer(self):
#        _jpype.fault("PyJPArray_releaseBuffer")
#        def f():
#            ja = JArray(JInt)(5)
#            m = memoryview(ja)
#        with self.assertRaises(SystemError):
#            f()

    def testJPArray_getBuffer(self):
        _jpype.fault("PyJPArray_getBuffer")
        with self.assertRaisesRegex(SystemError, "fault"):
            ja = JArray(JInt,2)(5)
            m = memoryview(ja)
            del m # lgtm [py/unnecessary-delete]

    def testJPArrayPrimitive_getBuffer(self):
        _jpype.fault("PyJPArrayPrimitive_getBuffer")
        def f():
            ja = JArray(JInt)(5)
            m = memoryview(ja)
            del m # lgtm [py/unnecessary-delete]
        with self.assertRaisesRegex(SystemError, "fault"):
            f()
        with self.assertRaises(BufferError):
            memoryview(JArray(JInt,2)([[1,2],[1],[1,2,3]]))
        _jpype.fault("PyJPModule_getContext")
        with self.assertRaisesRegex(SystemError, "fault"):
            f()

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
            null[0]=1
        with self.assertRaises(ValueError):
            memoryview(null)
        null = JArray(JObject)(object())
        with self.assertRaises(ValueError):
            memoryview(null)
        _jpype.fault("PyJPModule_getContext")
        with self.assertRaisesRegex(SystemError, "fault"):
            memoryview(null)

    def testJPClass_new(self):
        _jpype.fault("PyJPClass_new")
        with self.assertRaisesRegex(SystemError, "fault"):
            _jpype._JClass("foo", (object,), {})
        with self.assertRaises(TypeError):
            _jpype._JClass("foo", (object,), {})
        with self.assertRaises(TypeError):
            _jpype._JClass("foo", (_jpype._JObject,), {'__del__':None})

    def testJPClass_init(self):
        _jpype.fault("PyJPClass_init")
        with self.assertRaises(SystemError):
            _jpype._JClass("foo", (object,), {})
        with self.assertRaises(TypeError    ):
            _jpype._JClass("foo", (object,), {})
        _jpype._JClass("foo", (_jpype._JObject,), {})

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

    def testJPClass_subclasscheck(self):
        js = JClass("java.lang.String")
        _jpype.fault("PyJPClass_subclasscheck")
        with self.assertRaisesRegex(SystemError, "fault"):
            issubclass(js, JObject)
        _jpype.fault("PyJPModule_getContext")
        with self.assertRaisesRegex(SystemError, "fault"):
            issubclass(js, JObject)

    def testJPClass_class(self):
        js = JClass("java.lang.String")
        _jpype.fault("PyJPClass_class")
        with self.assertRaisesRegex(SystemError, "fault"):
            js.class_
        with self.assertRaises(AttributeError):
            _jpype._JClass("foo",(_jpype.JObject,), {}).class_
        _jpype.fault("PyJPModule_getContext")
        with self.assertRaisesRegex(SystemError, "fault"):
            js.class_

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

    def testJPClass_hints(self):
        js = JClass("java.lang.String")
        _jpype.fault("PyJPClass_hints")
        with self.assertRaisesRegex(SystemError, "fault"):
            js._hints
        _jpype.fault("PyJPModule_getContext")
        with self.assertRaisesRegex(SystemError, "fault"):
            js._hints
        self.assertIsInstance(js._hints, _jpype._JClassHints)

    def testJPClass_setHints(self):
        js = JClass("java.lang.String")
        _jpype.fault("PyJPClass_setHints")
        with self.assertRaisesRegex(SystemError, "fault"):
            js._hints = None

    def testJPClass_cnaConvertToJava(self):
        js = JClass("java.lang.String")
        _jpype.fault("PyJPClass_canConvertToJava")
        with self.assertRaisesRegex(SystemError, "fault"):
            js._canConvertToJava("f")
        _jpype.fault("PyJPModule_getContext")
        with self.assertRaisesRegex(SystemError, "fault"):
            js._canConvertToJava("f")
        js._canConvertToJava("f")

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

    def testJPClassHints_new(self):
        _jpype.fault("PyJPClassHints_new")
        with self.assertRaisesRegex(SystemError, "fault"):
            _jpype._JClassHints()
        _jpype._JClassHints()

    def testJPClassHints_init(self):
        _jpype.fault("PyJPClassHints_init")
        with self.assertRaisesRegex(SystemError, "fault"):
            _jpype._JClassHints()
        _jpype._JClassHints()

    def testJPClassHints_str(self):
        _jpype.fault("PyJPClassHints_str")
        with self.assertRaisesRegex(SystemError, "fault"):
            str(_jpype._JClassHints())
        str(_jpype._JClassHints())

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


#pyjp_method.cpp:	JP_PY_TRY("PyJPMethod_new");
#pyjp_method.cpp:	JP_PY_TRY("PyJPMethod_dealloc");
#pyjp_method.cpp:	JP_PY_TRY("PyJPMethod_get");
#pyjp_method.cpp:	JP_PY_TRY("PyJPMethod_call");
#pyjp_method.cpp:	JP_PY_TRY("PyJPMethod_str");
#pyjp_method.cpp:	JP_PY_TRY("PyJPMethod_repr");
#pyjp_method.cpp:	JP_PY_TRY("PyJPMethod_getSelf");
#pyjp_method.cpp:	JP_PY_TRY("PyJPMethod_getName");
#pyjp_method.cpp:	JP_PY_TRY("PyJPMethod_getQualName");
#pyjp_method.cpp:	JP_PY_TRY("PyJPMethod_getDoc");
#pyjp_method.cpp:	JP_PY_TRY("PyJPMethod_getDoc");
#pyjp_method.cpp:	JP_PY_TRY("PyJPMethod_getAnnotations");
#pyjp_method.cpp:	JP_PY_TRY("PyJPMethod_getCodeAttr");
#pyjp_method.cpp:	JP_PY_TRY("PyJPMethod_isBeanAccessor");
#pyjp_method.cpp:	JP_PY_TRY("PyJPMethod_isBeanMutator");
#pyjp_method.cpp:	JP_PY_TRY("PyJPMethod_matchReport");
#pyjp_method.cpp:	JP_PY_TRY("PyJPMethod_dump");
#pyjp_module.cpp:	JP_PY_TRY("Py_GetAttrDescriptor");
#pyjp_module.cpp:	JP_PY_TRY("PyJPModule_startup");
#pyjp_module.cpp:	JP_PY_TRY("PyJPModule_shutdown");
#pyjp_module.cpp:	JP_PY_TRY("PyJPModule_attachThread");
#pyjp_module.cpp:	JP_PY_TRY("PyJPModule_attachThreadAsDaemon");
#pyjp_module.cpp:	JP_PY_TRY("PyJPModule_detachThread");
#pyjp_module.cpp:	JP_PY_TRY("PyJPModule_isThreadAttached");
#pyjp_module.cpp:	JP_PY_TRY("PyJPModule_convertToDirectByteBuffer");
#pyjp_module.cpp:	JP_PY_TRY("PyJPModule_getArrayType");
#pyjp_module.cpp:	JP_PY_TRY("PyJPModule_getClass");
#pyjp_module.cpp:	JP_PY_TRY("PyJPModule_getClass");
#pyjp_module.cpp:	JP_PY_TRY("examine");
#pyjp_module.cpp:	JP_PY_TRY("PyInit__jpype");

    def testJPMonitor_init(self):
        jo = JClass("java.lang.Object")()
        _jpype.fault("PyJPMonitor_init")
        with self.assertRaisesRegex(SystemError, "fault"):
            _jpype._JMonitor(jo)
        _jpype.fault("PyJPModule_getContext")
        with self.assertRaisesRegex(SystemError, "fault"):
            _jpype._JMonitor(jo)
        _jpype._JMonitor(jo)

    def testJPMonitor_str(self):
        jo = JClass("java.lang.Object")()
        jm = _jpype._JMonitor(jo)
        _jpype.fault("PyJPMonitor_str")
        with self.assertRaisesRegex(SystemError, "fault"):
            str(jm)
        _jpype.fault("PyJPModule_getContext")
        with self.assertRaisesRegex(SystemError, "fault"):
            str(jm)

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

    def testJPMonitor_exit(self):
        jo = JClass("java.lang.Object")()
        _jpype.fault("PyJPMonitor_exit")
        with self.assertRaisesRegex(SystemError, "fault"):
            with _jpype._JMonitor(jo):
                pass

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

    def testJPNumberLong_int(self):
        ji = JInt(1)
        _jpype.fault("PyJPNumberLong_int")
        with self.assertRaisesRegex(SystemError, "fault"):
            int(ji)
        _jpype.fault("PyJPModule_getContext")
        with self.assertRaisesRegex(SystemError, "fault"):
            int(ji)
        int(ji)

    def testJPNumberLong_float(self):
        ji = JInt(1)
        _jpype.fault("PyJPNumberLong_float")
        with self.assertRaisesRegex(SystemError, "fault"):
            float(ji)
        _jpype.fault("PyJPModule_getContext")
        with self.assertRaisesRegex(SystemError, "fault"):
            float(ji)
        float(ji)

    def testJPNumberLong_str(self):
        ji = JInt(1)
        _jpype.fault("PyJPNumberLong_str")
        with self.assertRaisesRegex(SystemError, "fault"):
            str(ji)
        _jpype.fault("PyJPModule_getContext")
        with self.assertRaisesRegex(SystemError, "fault"):
            str(ji)
        str(ji)

    def testJPNumberLong_repr(self):
        ji = JInt(1)
        _jpype.fault("PyJPNumberLong_repr")
        with self.assertRaisesRegex(SystemError, "fault"):
            repr(ji)
        _jpype.fault("PyJPModule_getContext")
        with self.assertRaisesRegex(SystemError, "fault"):
            repr(ji)
        repr(ji)

    def testJPNumberLong_compare(self):
        ji = JInt(1)
        _jpype.fault("PyJPNumberLong_compare")
        with self.assertRaisesRegex(SystemError, "fault"):
            ji==1
        _jpype.fault("PyJPModule_getContext")
        with self.assertRaisesRegex(SystemError, "fault"):
            ji==1
        ji==1

    def testJPNumberLong_hash(self):
        ji = JInt(1)
        _jpype.fault("PyJPNumberLong_hash")
        with self.assertRaises(SystemError):
            hash(ji)
        _jpype.fault("PyJPModule_getContext")
        with self.assertRaises(SystemError):
            hash(ji)
        hash(ji)

    def testJPNumberFloat_int(self):
        jd = JDouble(1)
        _jpype.fault("PyJPNumberFloat_int")
        with self.assertRaisesRegex(SystemError, "fault"):
            int(jd)
        _jpype.fault("PyJPModule_getContext")
        with self.assertRaisesRegex(SystemError, "fault"):
            int(jd)
        int(jd)

    def testJPNumberFloat_float(self):
        jd = JDouble(1)
        _jpype.fault("PyJPNumberFloat_float")
        with self.assertRaisesRegex(SystemError, "fault"):
            float(jd)
        _jpype.fault("PyJPModule_getContext")
        with self.assertRaisesRegex(SystemError, "fault"):
            float(jd)
        float(jd)

    def testJPNumberFloat_str(self):
        jd = JDouble(1)
        _jpype.fault("PyJPNumberFloat_str")
        with self.assertRaisesRegex(SystemError, "fault"):
            str(jd)
        _jpype.fault("PyJPModule_getContext")
        with self.assertRaisesRegex(SystemError, "fault"):
            str(jd)
        str(jd)

    def testJPNumberFloat_repr(self):
        jd = JDouble(1)
        _jpype.fault("PyJPNumberFloat_repr")
        with self.assertRaisesRegex(SystemError, "fault"):
            repr(jd)
        _jpype.fault("PyJPModule_getContext")
        with self.assertRaisesRegex(SystemError, "fault"):
            repr(jd)
        repr(jd)

    def testJPNumberFloat_compare(self):
        jd = JDouble(1)
        _jpype.fault("PyJPNumberFloat_compare")
        with self.assertRaisesRegex(SystemError, "fault"):
            jd==1
        _jpype.fault("PyJPModule_getContext")
        with self.assertRaisesRegex(SystemError, "fault"):
            jd==1
        jd==1

    def testJPNumberFloat_hash(self):
        jd = JDouble(1)
        _jpype.fault("PyJPNumberFloat_hash")
        with self.assertRaises(SystemError):
            hash(jd)
        _jpype.fault("PyJPModule_getContext")
        with self.assertRaises(SystemError):
            hash(jd)
        hash(jd)

    def testJPChar_new(self):
        _jpype.fault("PyJPChar_new")
        with self.assertRaisesRegex(SystemError, "fault"):
            JChar("a")
        _jpype.fault("PyJPModule_getContext")
        with self.assertRaisesRegex(SystemError, "fault"):
            JChar("a")
        JChar("a")

    def testJPChar_str(self):
        jc = JChar("a")
        _jpype.fault("PyJPChar_str")
        with self.assertRaisesRegex(SystemError, "fault"):
            str(jc)
        _jpype.fault("PyJPModule_getContext")
        str(jc)

    def testJPBoolean_str(self):
        jb = JBoolean(True)
        _jpype.fault("PyJPBoolean_str")
        with self.assertRaisesRegex(SystemError, "fault"):
            str(jb)
        _jpype.fault("PyJPModule_getContext")
        str(jb)

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

    def testJPObject_hash(self):
        jo = JClass("java.lang.Object")()
        _jpype.fault("PyJPObject_hash")
        with self.assertRaises(SystemError):
            hash(jo)
        _jpype.fault("PyJPModule_getContext")
        with self.assertRaises(SystemError):
            hash(jo)
        hash(jo)

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

# FIXME this needs special treatment
#    def testJPProxy_str(self):
#        # Java has a hidden requirement that toString be available
#        @JImplements("java.util.function.DoubleUnaryOperator")
#        class f(object):
#            @JOverride
#            def applyAsDouble(self, d):
#                return d
#        jo = JObject(f(), "java.util.function.DoubleUnaryOperator")
#        raise RuntimeError(jo.toString())
 
    def testJPProxy_dealloc(self):
        _jpype.fault("PyJPProxy_dealloc")
        def f():
            _jpype._JProxy(None, [JClass("java.io.Serializable")])
        f()  

    def testJPProxy_call(self):
        @JImplements("java.util.function.DoubleUnaryOperator")
        class f(object):
            @JOverride
            def applyAsDouble(self, d):
                if d==2:
                    return None
                return d
        _jpype.fault("JPProxy::getProxy")
        with self.assertRaises(SystemError):
            JObject(f(), "java.util.function.DoubleUnaryOperator")
        jo = JObject(f(), "java.util.function.DoubleUnaryOperator")
        # FIXME special case Java does not reflect the SystemError back to Python
        _jpype.fault("PyJPProxy_getCallable")
        with self.assertRaises(jpype.JException):
            jo.applyAsDouble(1)
        # FIXME segfault on this one, needs investigation
#        _jpype.fault("JPype_InvocationHandler_hostInvoke")
#        with self.assertRaises(jpype.JException):
#            jo.applyAsDouble(1)
        with self.assertRaises(jpype.JException):
            jo.applyAsDouble(2)

    def testJPProxy_void(self):
        @JImplements("java.util.function.Consumer")
        class f(object):
            @JOverride
            def accept(self, d):
                return None
        jo = JObject(f(), "java.util.function.Consumer")
        # FIXME segfaults
        # jo.accept(None)

    def testJPProxy_box_return(self):
        q = None
        @JImplements("java.util.function.Supplier")
        class f(object):
            @JOverride
            def get(self):
                return q
        jo = JObject(f(), "java.util.function.Supplier")
        self.assertEqual(jo.get(), None)
        q=1.0
        self.assertIsInstance(jo.get(), java.lang.Double)
        q=1
        self.assertIsInstance(jo.get(), java.lang.Long)
        q="ABC"
        self.assertIsInstance(jo.get(), java.lang.String)
        q=object()
        with self.assertRaises(jpype.JException):
            jo.get()
  
    def testJPValue_alloc(self):
        _jpype.fault("PyJPValue_alloc")
        with self.assertRaisesRegex(SystemError, "fault"):
            JInt(1)
        _jpype.fault("PyJPModule_getContext")
        with self.assertRaisesRegex(SystemError, "fault"):
            JInt(1)
        JInt(1)

    def testJPValue_finalize(self):
        _jpype.fault("PyJPValue_finalize")
        a = JInt(1)
        del a # lgtm [py/unnecessary-delete] 

    def testJPValue_str(self):
        js = JString("f")
        _jpype.fault("PyJPValue_str")
        with self.assertRaisesRegex(SystemError, "fault"):
            str(js)
        _jpype.fault("PyJPModule_getContext")
        with self.assertRaisesRegex(SystemError, "fault"):
            str(js)
        str(js)

    def testJPObject_getattro(self):
        jo = JString("f")
        _jpype.fault("PyJPObject_getattro")
        with self.assertRaisesRegex(SystemError, "fault"):
            jo.substring
        _jpype.fault("PyJPModule_getContext")
        with self.assertRaisesRegex(SystemError, "fault"):
            jo.substring
        jo.substring

    def testJPObject_setattro(self):
        jo = JString("f")
        _jpype.fault("PyJPObject_setattro")
        with self.assertRaisesRegex(SystemError, "fault"):
            jo.substring = None

    def testJPBooleanType(self):
        ja = JArray(JBoolean)(5) # lgtm [py/similar-function]
        _jpype.fault("JPBooleanType::setArrayRange")
        with self.assertRaisesRegex(SystemError, "fault"):
            ja[1:3] = [0,0]
        with self.assertRaises(TypeError):
            ja[1] = object()
        jf = JClass("jpype.fields.Fields")
        with self.assertRaises(TypeError):
            jf.static_bool = object()
        with self.assertRaises(TypeError):
            jf().member_bool = object()

    def testJPCharType(self):
        ja = JArray(JChar)(5) # lgtm [py/similar-function]
        _jpype.fault("JPCharType::setArrayRange")
        with self.assertRaisesRegex(SystemError, "fault"):
            ja[1:3] = [0,0]
        with self.assertRaises(TypeError):
            ja[1] = object()
        jf = JClass("jpype.fields.Fields")
        with self.assertRaises(TypeError):
            jf.static_char = object()
        with self.assertRaises(TypeError):
            jf().member_char = object()

    def testJPByteType(self):
        ja = JArray(JByte)(5) # lgtm [py/similar-function]
        _jpype.fault("JPByteType::setArrayRange")
        with self.assertRaisesRegex(SystemError, "fault"):
            ja[1:3] = [0,0]
        with self.assertRaises(TypeError):
            ja[1] = object()
        jf = JClass("jpype.fields.Fields")
        with self.assertRaises(TypeError):
            jf.static_byte = object()
        with self.assertRaises(TypeError):
            jf().member_byte = object()

    def testJPShortType(self):
        ja = JArray(JShort)(5) # lgtm [py/similar-function]
        _jpype.fault("JPShortType::setArrayRange")
        with self.assertRaisesRegex(SystemError, "fault"):
            ja[1:3] = [0,0]
        with self.assertRaises(TypeError):
            ja[1] = object()
        jf = JClass("jpype.fields.Fields")
        with self.assertRaises(TypeError):
            jf.static_short = object()
        with self.assertRaises(TypeError):
            jf().member_short = object()

    def testJPIntType(self):
        ja = JArray(JInt)(5) # lgtm [py/similar-function]
        _jpype.fault("JPIntType::setArrayRange")
        with self.assertRaisesRegex(SystemError, "fault"):
            ja[1:3] = [0,0]
        with self.assertRaises(TypeError):
            ja[1] = object()
        jf = JClass("jpype.fields.Fields")
        with self.assertRaises(TypeError):
            jf.static_int = object()
        with self.assertRaises(TypeError):
            jf().member_int = object()

    def testJPLongType(self):
        ja = JArray(JLong)(5) # lgtm [py/similar-function]
        _jpype.fault("JPLongType::setArrayRange")
        with self.assertRaisesRegex(SystemError, "fault"):
            ja[1:3] = [0,0]
        with self.assertRaises(TypeError):
            ja[1] = object()
        jf = JClass("jpype.fields.Fields")
        with self.assertRaises(TypeError):
            jf.static_long = object()
        with self.assertRaises(TypeError):
            jf().member_long = object()

    def testJPFloatType(self):
        ja = JArray(JFloat)(5) # lgtm [py/similar-function]
        _jpype.fault("JPFloatType::setArrayRange")
        with self.assertRaisesRegex(SystemError, "fault"):
            ja[1:3] = [0,0]
        with self.assertRaises(TypeError):
            ja[1] = object()
        jf = JClass("jpype.fields.Fields")
        with self.assertRaises(TypeError):
            jf.static_float = object()
        with self.assertRaises(TypeError):
            jf().member_float = object()

    def testJPDoubleType(self):
        ja = JArray(JDouble)(5) # lgtm [py/similar-function]
        _jpype.fault("JPDoubleType::setArrayRange")
        with self.assertRaisesRegex(SystemError, "fault"):
            ja[1:3] = [0,0]
        with self.assertRaises(TypeError):
            ja[1] = object()
        jf = JClass("jpype.fields.Fields")
        with self.assertRaises(TypeError):
            jf.static_double = object()
        with self.assertRaises(TypeError):
            jf().member_double = object()

    def testJPField(self):
        jf = JClass("jpype.fields.Fields")
        jfi = jf()
        with self.assertRaises(AttributeError):
            jf.final_static_int = 2
        with self.assertRaises(AttributeError):
            jfi.final_member_int = 2
        _jpype.fault("JPField::setStaticAttribute")
        with self.assertRaisesRegex(SystemError, "fault"):
            jf.static_int = 2
        _jpype.fault("JPField::setAttribute")
        with self.assertRaisesRegex(SystemError, "fault"):
            jfi.member_int = 2
        _jpype.fault("JPField::getStaticAttribute")
        with self.assertRaisesRegex(SystemError, "fault"):
            i = jf.static_int
        _jpype.fault("JPField::getAttribute")
        with self.assertRaisesRegex(SystemError, "fault"):
            i = jfi.member_int
        si = jf.__dict__['static_int']
        str(si)
        repr(si)
        i =  None
        _jpype.fault("PyJPField_get")
        with self.assertRaisesRegex(SystemError, "fault"):
            i = jfi.member_int
        self.assertEqual(i, None)
        _jpype.fault("PyJPField_set")
        with self.assertRaisesRegex(SystemError, "fault"):
            jfi.member_int = 2
        _jpype.fault("PyJPField_repr")
        with self.assertRaisesRegex(SystemError, "fault"):
            repr(si)

    def testConvertString(self):
        _jpype.fault("JPObjectType::canConvertToJava")
        with self.assertRaisesRegex(SystemError, "fault"):
            JObject._convertToJava("foo")
        _jpype.fault("JPConversionString::matches")
        with self.assertRaisesRegex(SystemError, "fault"):
            JString._convertToJava("foo")


    def testJPObject(self):
        jf = JClass("jpype.fields.Fields")
        jfi = JClass("jpype.fields.Fields")()
        _jpype.fault("JPClass::setStaticField")
        with self.assertRaisesRegex(SystemError, "fault"):
            jf.static_object = None
        _jpype.fault("JPClass::setField")
        with self.assertRaisesRegex(SystemError, "fault"):
            jfi.member_object = None
        i = None
        _jpype.fault("JPClass::getStaticField")
        with self.assertRaisesRegex(SystemError, "fault"):
            i = jf.static_object
        _jpype.fault("JPClass::getField")
        with self.assertRaisesRegex(SystemError, "fault"):
            i = jfi.member_object
        self.assertEqual(i, None)

    @common.unittest.SkipTest
    def testJPTypeManagerFindClass(self):
        ja = JArray(JInt,2)([[1,1],[1,1]])
        _jpype.fault("JPTypeManager::findClass")
        with self.assertRaisesRegex(SystemError, "fault"):
            memoryview(ja)
        _jpype.fault("JPTypeManager::findClassByName")
        with self.assertRaisesRegex(SystemError, "fault"):
            JClass("foo.bar")
        jo = JString('a')
        _jpype.fault("JPTypeManager::findClassForObject")
        with self.assertRaisesRegex(SystemError, "fault"):
            jo.substring(0,1)

    def testJPTypeManagerPopulate(self):
        _jpype.fault("JPTypeManager::populateMembers")
        with self.assertRaisesRegex(SystemError, "fault"):
            JClass("java.math.MathContext")
        _jpype.fault("JPTypeManager::populateMethod")
        with self.assertRaisesRegex(SystemError, "fault"):
            JClass("java.math.MathContext")().getPrecision()
        JClass("java.math.MathContext")

    def testJPArrayNew(self):
        ja = JArray(JInt)
        _jpype.fault("JPArray::JPArray")
        with self.assertRaisesRegex(SystemError, "fault"):
            ja(5)
        j = ja(5)
        _jpype.fault("JPArray::JPArraySlice")
        with self.assertRaisesRegex(SystemError, "fault"):
            j[0:2:1]

    def testMethodPack(self):
        js = JString("a")
        _jpype.fault("JPMethod::packArgs")
        with self.assertRaisesRegex(SystemError, "fault"):
            js.substring(1)

    def testJArrayClassConvertToVector(self):
        Path = JClass("java.nio.file.Paths")
        _jpype.fault("JPArrayClass::convertToJavaVector")
        with self.assertRaisesRegex(SystemError, "fault"):
            Path.get("foo","bar")

    def testJArrayGetJavaConversion(self):
        ja = JArray(JInt)
        _jpype.fault("JPArrayClass::getJavaConversion")
        with self.assertRaisesRegex(SystemError, "fault"):
            ja._canConvertToJava(object())

    def testJArrayConvertToPythonObject(self):
        jl = JClass('java.util.ArrayList')()
        jl.add(JArray(JInt)(3))
        _jpype.fault("JPArrayClass::convertToPythonObject")
        with self.assertRaisesRegex(SystemError, "fault"):
            jl.get(0)

    def testJBooleanGetJavaConversion(self):
        _jpype.fault("JPBooleanType::getJavaConversion")
        with self.assertRaisesRegex(SystemError, "fault"):
            JBoolean._canConvertToJava(object())

    def testJCharGetJavaConversion(self):
        _jpype.fault("JPCharType::getJavaConversion")
        with self.assertRaisesRegex(SystemError, "fault"):
            JChar._canConvertToJava(object())

    def testJByteGetJavaConversion(self):
        _jpype.fault("JPByteType::getJavaConversion")
        with self.assertRaisesRegex(SystemError, "fault"):
            JByte._canConvertToJava(object())

    def testJShortGetJavaConversion(self):
        _jpype.fault("JPShortType::getJavaConversion")
        with self.assertRaisesRegex(SystemError, "fault"):
            JShort._canConvertToJava(object())

    def testJIntGetJavaConversion(self):
        _jpype.fault("JPIntType::getJavaConversion")
        with self.assertRaisesRegex(SystemError, "fault"):
            JInt._canConvertToJava(object())

    def testJLongGetJavaConversion(self):
        _jpype.fault("JPLongType::getJavaConversion")
        with self.assertRaisesRegex(SystemError, "fault"):
            JLong._canConvertToJava(object())

    def testJFloatGetJavaConversion(self):
        _jpype.fault("JPFloatType::getJavaConversion")
        with self.assertRaisesRegex(SystemError, "fault"):
            JFloat._canConvertToJava(object())

    def testJDoubleGetJavaConversion(self):
        _jpype.fault("JPDoubleType::getJavaConversion")
        with self.assertRaisesRegex(SystemError, "fault"):
            JDouble._canConvertToJava(object())

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

    def testJPJavaFrame(self):
        fields = JClass("jpype.fields.Fields")()
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

    def testJPJavaFrameByteField(self):
        fields = JClass("jpype.fields.Fields")()
        _jpype.fault("JPJavaFrame::GetStaticByteField")
        with self.assertRaisesRegex(SystemError, "fault"):
            print(fields.static_byte)
        _jpype.fault("JPJavaFrame::GetByteField")
        with self.assertRaisesRegex(SystemError, "fault"):
            print(fields.member_byte)
        _jpype.fault("JPJavaFrame::SetStaticByteField")
        with self.assertRaisesRegex(SystemError, "fault"):
            fields.static_byte = 1
        _jpype.fault("JPJavaFrame::SetByteField")
        with self.assertRaisesRegex(SystemError, "fault"):
            fields.member_byte = 0

    def testJPJavaFrameByteMethods(self):
        cls = JClass("jpype.fields.Fields")
        obj = cls()
        _jpype.fault("JPJavaFrame::CallStaticByteMethodA")
        with self.assertRaisesRegex(SystemError, "fault"):
            cls.getStaticByte()
        _jpype.fault("JPJavaFrame::CallByteMethodA")
        with self.assertRaisesRegex(SystemError, "fault"):
            obj.getMemberByte()
        _jpype.fault("JPJavaFrame::CallNonvirtualByteMethodA")
        with self.assertRaisesRegex(SystemError, "fault"):
            cls.getMemberByte(obj)

    def testJPJavaFrameShortField(self):
        fields = JClass("jpype.fields.Fields")()
        _jpype.fault("JPJavaFrame::GetStaticShortField")
        with self.assertRaisesRegex(SystemError, "fault"):
            print(fields.static_short)
        _jpype.fault("JPJavaFrame::GetShortField")
        with self.assertRaisesRegex(SystemError, "fault"):
            print(fields.member_short)
        _jpype.fault("JPJavaFrame::SetStaticShortField")
        with self.assertRaisesRegex(SystemError, "fault"):
            fields.static_short = 1
        _jpype.fault("JPJavaFrame::SetShortField")
        with self.assertRaisesRegex(SystemError, "fault"):
            fields.member_short = 1

    def testJPJavaFrameShortMethod(self):
        cls = JClass("jpype.fields.Fields")
        obj = cls()
        _jpype.fault("JPJavaFrame::CallStaticShortMethodA")
        with self.assertRaisesRegex(SystemError, "fault"):
            cls.getStaticShort()
        _jpype.fault("JPJavaFrame::CallShortMethodA")
        with self.assertRaisesRegex(SystemError, "fault"):
            obj.getMemberShort()
        _jpype.fault("JPJavaFrame::CallNonvirtualShortMethodA")
        with self.assertRaisesRegex(SystemError, "fault"):
            cls.getMemberShort(obj)

    def testJPJavaFrameIntField(self):
        fields = JClass("jpype.fields.Fields")()
        _jpype.fault("JPJavaFrame::GetStaticIntField")
        with self.assertRaisesRegex(SystemError, "fault"):
            print(fields.static_int)
        _jpype.fault("JPJavaFrame::GetIntField")
        with self.assertRaisesRegex(SystemError, "fault"):
            print(fields.member_int)
        _jpype.fault("JPJavaFrame::SetStaticIntField")
        with self.assertRaisesRegex(SystemError, "fault"):
            fields.static_int = 1
        _jpype.fault("JPJavaFrame::SetIntField")
        with self.assertRaisesRegex(SystemError, "fault"):
            fields.member_int = 1

    def testJPJavaFrameIntMethod(self):
        cls = JClass("jpype.fields.Fields")
        obj = cls()
        _jpype.fault("JPJavaFrame::CallStaticIntMethodA")
        with self.assertRaisesRegex(SystemError, "fault"):
            cls.getStaticInt()
        _jpype.fault("JPJavaFrame::CallIntMethodA")
        with self.assertRaisesRegex(SystemError, "fault"):
            obj.getMemberInt()
        _jpype.fault("JPJavaFrame::CallNonvirtualIntMethodA")
        with self.assertRaisesRegex(SystemError, "fault"):
            cls.getMemberInt(obj)

    def testJPJavaFrameLongField(self):
        fields = JClass("jpype.fields.Fields")()
        _jpype.fault("JPJavaFrame::GetStaticLongField")
        with self.assertRaisesRegex(SystemError, "fault"):
            print(fields.static_long)
        _jpype.fault("JPJavaFrame::GetLongField")
        with self.assertRaisesRegex(SystemError, "fault"):
            print(fields.member_long)
        _jpype.fault("JPJavaFrame::SetStaticLongField")
        with self.assertRaisesRegex(SystemError, "fault"):
            fields.static_long = 1
        _jpype.fault("JPJavaFrame::SetLongField")
        with self.assertRaisesRegex(SystemError, "fault"):
            fields.member_long = 1

    def testJPJavaFrameLongMethod(self):
        cls = JClass("jpype.fields.Fields")
        obj = cls()
        _jpype.fault("JPJavaFrame::CallStaticLongMethodA")
        with self.assertRaisesRegex(SystemError, "fault"):
            cls.getStaticLong()
        _jpype.fault("JPJavaFrame::CallLongMethodA")
        with self.assertRaisesRegex(SystemError, "fault"):
            obj.getMemberLong()
        _jpype.fault("JPJavaFrame::CallNonvirtualLongMethodA")
        with self.assertRaisesRegex(SystemError, "fault"):
            cls.getMemberLong(obj)

    def testJPJavaFrameFloatField(self):
        fields = JClass("jpype.fields.Fields")()
        _jpype.fault("JPJavaFrame::GetStaticFloatField")
        with self.assertRaisesRegex(SystemError, "fault"):
            print(fields.static_float)
        _jpype.fault("JPJavaFrame::GetFloatField")
        with self.assertRaisesRegex(SystemError, "fault"):
            print(fields.member_float)
        _jpype.fault("JPJavaFrame::SetStaticFloatField")
        with self.assertRaisesRegex(SystemError, "fault"):
            fields.static_float = 1
        _jpype.fault("JPJavaFrame::SetFloatField")
        with self.assertRaisesRegex(SystemError, "fault"):
            fields.member_float = 1

    def testJPJavaFrameFloatMethod(self):
        cls = JClass("jpype.fields.Fields")
        obj = cls()
        _jpype.fault("JPJavaFrame::CallStaticFloatMethodA")
        with self.assertRaisesRegex(SystemError, "fault"):
            cls.getStaticFloat()
        _jpype.fault("JPJavaFrame::CallFloatMethodA")
        with self.assertRaisesRegex(SystemError, "fault"):
            obj.getMemberFloat()
        _jpype.fault("JPJavaFrame::CallNonvirtualFloatMethodA")
        with self.assertRaisesRegex(SystemError, "fault"):
            cls.getMemberFloat(obj)

    def testJPJavaFrameDoubleField(self):
        fields = JClass("jpype.fields.Fields")()
        _jpype.fault("JPJavaFrame::GetStaticDoubleField")
        with self.assertRaisesRegex(SystemError, "fault"):
            print(fields.static_double)
        _jpype.fault("JPJavaFrame::GetDoubleField")
        with self.assertRaisesRegex(SystemError, "fault"):
            print(fields.member_double)
        _jpype.fault("JPJavaFrame::SetStaticDoubleField")
        with self.assertRaisesRegex(SystemError, "fault"):
            fields.static_double = 1
        _jpype.fault("JPJavaFrame::SetDoubleField")
        with self.assertRaisesRegex(SystemError, "fault"):
            fields.member_double = 1

    def testJPJavaFrameDoubleMethod(self):
        cls = JClass("jpype.fields.Fields")
        obj = cls()
        _jpype.fault("JPJavaFrame::CallStaticDoubleMethodA")
        with self.assertRaisesRegex(SystemError, "fault"):
            cls.getStaticDouble()
        _jpype.fault("JPJavaFrame::CallDoubleMethodA")
        with self.assertRaisesRegex(SystemError, "fault"):
            obj.getMemberDouble()
        _jpype.fault("JPJavaFrame::CallNonvirtualDoubleMethodA")
        with self.assertRaisesRegex(SystemError, "fault"):
            cls.getMemberDouble(obj)

    def testJPJavaFrameCharField(self):
        fields = JClass("jpype.fields.Fields")()
        _jpype.fault("JPJavaFrame::GetStaticCharField")
        with self.assertRaisesRegex(SystemError, "fault"):
            print(fields.static_char)
        _jpype.fault("JPJavaFrame::GetCharField")
        with self.assertRaisesRegex(SystemError, "fault"):
            print(fields.member_char)
        _jpype.fault("JPJavaFrame::SetStaticCharField")
        with self.assertRaisesRegex(SystemError, "fault"):
            fields.static_char = 1
        _jpype.fault("JPJavaFrame::SetCharField")
        with self.assertRaisesRegex(SystemError, "fault"):
            fields.member_char = 1

    def testJPJavaFrameCharMethod(self):
        cls = JClass("jpype.fields.Fields")
        obj = cls()
        _jpype.fault("JPJavaFrame::CallStaticCharMethodA")
        with self.assertRaisesRegex(SystemError, "fault"):
            cls.getStaticChar()
        _jpype.fault("JPJavaFrame::CallCharMethodA")
        with self.assertRaisesRegex(SystemError, "fault"):
            obj.getMemberChar()
        _jpype.fault("JPJavaFrame::CallNonvirtualCharMethodA")
        with self.assertRaisesRegex(SystemError, "fault"):
            cls.getMemberChar(obj)

    def testJPJavaFrameBooleanField(self):
        fields = JClass("jpype.fields.Fields")()
        _jpype.fault("JPJavaFrame::GetStaticBooleanField")
        with self.assertRaisesRegex(SystemError, "fault"):
            print(fields.static_bool)
        _jpype.fault("JPJavaFrame::GetBooleanField")
        with self.assertRaisesRegex(SystemError, "fault"):
            print(fields.member_bool)
        _jpype.fault("JPJavaFrame::SetStaticBooleanField")
        with self.assertRaisesRegex(SystemError, "fault"):
            fields.static_bool = 1
        _jpype.fault("JPJavaFrame::SetBooleanField")
        with self.assertRaisesRegex(SystemError, "fault"):
            fields.member_bool = 1

    def testJPJavaFrameBooleanMethod(self):
        cls = JClass("jpype.fields.Fields")
        obj = cls()
        _jpype.fault("JPJavaFrame::CallStaticBooleanMethodA")
        with self.assertRaisesRegex(SystemError, "fault"):
            cls.getStaticBool()
        _jpype.fault("JPJavaFrame::CallBooleanMethodA")
        with self.assertRaisesRegex(SystemError, "fault"):
            obj.getMemberBool()
        _jpype.fault("JPJavaFrame::CallNonvirtualBooleanMethodA")
        with self.assertRaisesRegex(SystemError, "fault"):
            cls.getMemberBool(obj)

    def testJPJavaFrameObjectField(self):
        fields = JClass("jpype.fields.Fields")()
        _jpype.fault("JPJavaFrame::GetStaticObjectField")
        with self.assertRaisesRegex(SystemError, "fault"):
            print(fields.static_object)
        _jpype.fault("JPJavaFrame::GetObjectField")
        with self.assertRaisesRegex(SystemError, "fault"):
            print(fields.member_object)
        _jpype.fault("JPJavaFrame::SetStaticObjectField")
        with self.assertRaisesRegex(SystemError, "fault"):
            fields.static_object = None
        _jpype.fault("JPJavaFrame::SetObjectField")
        with self.assertRaisesRegex(SystemError, "fault"):
            fields.member_object = None

    def testJPJavaFrameObjectMethod(self):
        cls = JClass("jpype.fields.Fields")
        obj = cls()
        _jpype.fault("JPJavaFrame::CallStaticObjectMethodA")
        with self.assertRaisesRegex(SystemError, "fault"):
            cls.getStaticObject()
        _jpype.fault("JPJavaFrame::CallObjectMethodA")
        with self.assertRaisesRegex(SystemError, "fault"):
            obj.getMemberObject()
        _jpype.fault("JPJavaFrame::CallNonvirtualObjectMethodA")
        with self.assertRaisesRegex(SystemError, "fault"):
            cls.getMemberObject(obj)


    def testJPJavaFrameByteArray(self):
        _jpype.fault("JPJavaFrame::NewByteArray")
        with self.assertRaisesRegex(SystemError, "fault"):
            JArray(JByte)(1)
        ja = JArray(JByte)(5)
        _jpype.fault("JPJavaFrame::SetByteArrayRegion")
        with self.assertRaisesRegex(SystemError, "fault"):
            ja[0] = 0
        _jpype.fault("JPJavaFrame::GetByteArrayRegion")
        with self.assertRaisesRegex(SystemError, "fault"):
            print(ja[0])
#        _jpype.fault("JPJavaFrame::GetByteArrayElements")
#        with self.assertRaisesRegex(SystemError, "fault"):
#            memoryview(ja[0:3])
#        _jpype.fault("JPJavaFrame::ReleaseByteArrayElements")
#        with self.assertRaisesRegex(SystemError, "fault"):
#            memoryview(ja[0:3])

    def testJPJavaFrameShortArray(self):
        _jpype.fault("JPJavaFrame::NewShortArray")
        with self.assertRaisesRegex(SystemError, "fault"):
            JArray(JShort)(1)
        _jpype.fault("JPJavaFrame::SetShortArrayRegion")
        with self.assertRaisesRegex(SystemError, "fault"):
            raise SystemError("fault")
        _jpype.fault("JPJavaFrame::GetShortArrayRegion")
        with self.assertRaisesRegex(SystemError, "fault"):
            raise SystemError("fault")
        _jpype.fault("JPJavaFrame::GetShortArrayElements")
        with self.assertRaisesRegex(SystemError, "fault"):
            raise SystemError("fault")
        _jpype.fault("JPJavaFrame::ReleaseShortArrayElements")
        with self.assertRaisesRegex(SystemError, "fault"):
            raise SystemError("fault")

    def testJPJavaFrameIntArray(self):
        _jpype.fault("JPJavaFrame::NewIntArray")
        with self.assertRaisesRegex(SystemError, "fault"):
            JArray(JInt)(1)
        _jpype.fault("JPJavaFrame::SetIntArrayRegion")
        with self.assertRaisesRegex(SystemError, "fault"):
            raise SystemError("fault")
        _jpype.fault("JPJavaFrame::GetIntArrayRegion")
        with self.assertRaisesRegex(SystemError, "fault"):
            raise SystemError("fault")
        _jpype.fault("JPJavaFrame::GetIntArrayElements")
        with self.assertRaisesRegex(SystemError, "fault"):
            raise SystemError("fault")
        _jpype.fault("JPJavaFrame::ReleaseIntArrayElements")
        with self.assertRaisesRegex(SystemError, "fault"):
            raise SystemError("fault")

    def testJPJavaFrameLongArray(self):
        _jpype.fault("JPJavaFrame::NewLongArray")
        with self.assertRaisesRegex(SystemError, "fault"):
            JArray(JLong)(1)
        _jpype.fault("JPJavaFrame::SetLongArrayRegion")
        with self.assertRaisesRegex(SystemError, "fault"):
            raise SystemError("fault")
        _jpype.fault("JPJavaFrame::GetLongArrayRegion")
        with self.assertRaisesRegex(SystemError, "fault"):
            raise SystemError("fault")
        _jpype.fault("JPJavaFrame::GetLongArrayElements")
        with self.assertRaisesRegex(SystemError, "fault"):
            raise SystemError("fault")
        _jpype.fault("JPJavaFrame::ReleaseLongArrayElements")
        with self.assertRaisesRegex(SystemError, "fault"):
            raise SystemError("fault")

    def testJPJavaFrameFloatArray(self):
        _jpype.fault("JPJavaFrame::NewFloatArray")
        with self.assertRaisesRegex(SystemError, "fault"):
            JArray(JFloat)(1)
        _jpype.fault("JPJavaFrame::SetFloatArrayRegion")
        with self.assertRaisesRegex(SystemError, "fault"):
            raise SystemError("fault")
        _jpype.fault("JPJavaFrame::GetFloatArrayRegion")
        with self.assertRaisesRegex(SystemError, "fault"):
            raise SystemError("fault")
        _jpype.fault("JPJavaFrame::GetFloatArrayElements")
        with self.assertRaisesRegex(SystemError, "fault"):
            raise SystemError("fault")
        _jpype.fault("JPJavaFrame::ReleaseFloatArrayElements")
        with self.assertRaisesRegex(SystemError, "fault"):
            raise SystemError("fault")

    def testJPJavaFrameDoubleArray(self):
        _jpype.fault("JPJavaFrame::NewDoubleArray")
        with self.assertRaisesRegex(SystemError, "fault"):
            JArray(JDouble)(1)
        _jpype.fault("JPJavaFrame::SetDoubleArrayRegion")
        with self.assertRaisesRegex(SystemError, "fault"):
            raise SystemError("fault")
        _jpype.fault("JPJavaFrame::GetDoubleArrayRegion")
        with self.assertRaisesRegex(SystemError, "fault"):
            raise SystemError("fault")
        _jpype.fault("JPJavaFrame::GetDoubleArrayElements")
        with self.assertRaisesRegex(SystemError, "fault"):
            raise SystemError("fault")
        _jpype.fault("JPJavaFrame::ReleaseDoubleArrayElements")
        with self.assertRaisesRegex(SystemError, "fault"):
            raise SystemError("fault")

    def testJPJavaFrameCharArray(self):
        _jpype.fault("JPJavaFrame::NewCharArray")
        with self.assertRaisesRegex(SystemError, "fault"):
            JArray(JChar)(1)
        _jpype.fault("JPJavaFrame::SetCharArrayRegion")
        with self.assertRaisesRegex(SystemError, "fault"):
            raise SystemError("fault")
        _jpype.fault("JPJavaFrame::GetCharArrayRegion")
        with self.assertRaisesRegex(SystemError, "fault"):
            raise SystemError("fault")
        _jpype.fault("JPJavaFrame::GetCharArrayElements")
        with self.assertRaisesRegex(SystemError, "fault"):
            raise SystemError("fault")
        _jpype.fault("JPJavaFrame::ReleaseCharArrayElements")
        with self.assertRaisesRegex(SystemError, "fault"):
            raise SystemError("fault")

    def testJPJavaFrameBooleanArray(self):
        _jpype.fault("JPJavaFrame::NewBooleanArray")
        with self.assertRaisesRegex(SystemError, "fault"):
            JArray(JBoolean)(1)
        _jpype.fault("JPJavaFrame::SetBooleanArrayRegion")
        with self.assertRaisesRegex(SystemError, "fault"):
            raise SystemError("fault")
        _jpype.fault("JPJavaFrame::GetBooleanArrayRegion")
        with self.assertRaisesRegex(SystemError, "fault"):
            raise SystemError("fault")
        _jpype.fault("JPJavaFrame::GetBooleanArrayElements")
        with self.assertRaisesRegex(SystemError, "fault"):
            raise SystemError("fault")
        _jpype.fault("JPJavaFrame::ReleaseBooleanArrayElements")
        with self.assertRaisesRegex(SystemError, "fault"):
            raise SystemError("fault")


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


    def testJPJavaFrameAssignable(self):
        _jpype.fault("JPJavaFrame::IsAssignableFrom")
        with self.assertRaisesRegex(SystemError, "fault"):
            issubclass(JString, JObject)


    def testJPJavaFrameString(self):
        _jpype.fault("JPJavaFrame::NewString")
        with self.assertRaisesRegex(SystemError, "fault"):
            JString("aa")
        _jpype.fault("JPJavaFrame::GetStringUTFChars")
        with self.assertRaisesRegex(SystemError, "fault"):
            str(JString("a"))
        # FIXME Segfaults
#        _jpype.fault("JPJavaFrame::ReleaseStringUTFChars")
#        with self.assertRaisesRegex(SystemError, "fault"):
#            str(JString("a"))
        _jpype.fault("JPJavaFrame::GetStringUTFLength")
        with self.assertRaisesRegex(SystemError, "fault"):
            str(JString("a"))


    def testJPJavaFrameArrayLength(self):
        _jpype.fault("JPJavaFrame::GetArrayLength")
        with self.assertRaisesRegex(SystemError, "fault"):
            JArray(JInt)(5)

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
