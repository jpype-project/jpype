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
        with self.assertRaises(SystemError):
            JArray(JInt)(5)

    def testJPArray_init(self):
        _jpype.fault("PyJPArray_init")
        with self.assertRaises(SystemError):
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
        with self.assertRaises(SystemError):
            JArray(JInt)(5)

    def testJPArray_repr(self):
        ja = JArray(JInt)(5)
        _jpype.fault("PyJPArray_repr")
        with self.assertRaises(SystemError):
            repr(ja)
        _jpype.fault("PyJPModule_getContext")
        with self.assertRaises(SystemError):
            repr(ja)

    def testJPArray_len(self):
        ja = JArray(JInt)(5)
        _jpype.fault("PyJPArray_len")
        with self.assertRaises(SystemError):
            len(ja)
        _jpype.fault("PyJPArray_len")
        with self.assertRaises(SystemError):
            ja.length
        _jpype.fault("PyJPModule_getContext")
        with self.assertRaises(SystemError):
            len(ja)

    def testJPArray_getArrayItem(self):
        ja = JArray(JInt)(5)
        _jpype.fault("PyJPArray_getArrayItem")
        with self.assertRaises(SystemError):
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
        with self.assertRaises(SystemError):
            ja[0:2] = 1
        _jpype.fault("PyJPArray_assignSubscript")
        with self.assertRaises(SystemError):
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
        with self.assertRaises(SystemError):
            ja = JArray(JInt,2)(5)
            m = memoryview(ja)
            del m # lgtm [py/unnecessary-delete]

    def testJPArrayPrimitive_getBuffer(self):
        _jpype.fault("PyJPArrayPrimitive_getBuffer")
        def f():
            ja = JArray(JInt)(5)
            m = memoryview(ja)
            del m # lgtm [py/unnecessary-delete]
        with self.assertRaises(SystemError):
            f()
        with self.assertRaises(BufferError):
            memoryview(JArray(JInt,2)([[1,2],[1],[1,2,3]]))
        _jpype.fault("PyJPModule_getContext")
        with self.assertRaises(SystemError):
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
        with self.assertRaises(SystemError):
            memoryview(null)

    def testJPClass_new(self):
        _jpype.fault("PyJPClass_new")
        with self.assertRaises(SystemError):
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
        with self.assertRaises(SystemError):
            js.foo
        with self.assertRaises(TypeError):
            getattr(js, object())
        _jpype.fault("PyJPModule_getContext")
        with self.assertRaises(SystemError):
            js.substring

    def testJPClass_setattro(self):
        js = JClass("java.lang.String")
        _jpype.fault("PyJPClass_setattro")
        with self.assertRaises(SystemError):
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
        with self.assertRaises(SystemError):
            issubclass(js, JObject)
        _jpype.fault("PyJPModule_getContext")
        with self.assertRaises(SystemError):
            issubclass(js, JObject)

    def testJPClass_class(self):
        js = JClass("java.lang.String")
        _jpype.fault("PyJPClass_class")
        with self.assertRaises(SystemError):
            js.class_
        with self.assertRaises(AttributeError):
            _jpype._JClass("foo",(_jpype.JObject,), {}).class_
        _jpype.fault("PyJPModule_getContext")
        with self.assertRaises(SystemError):
            js.class_

    def testJPClass_setClass(self):
        js = JClass("java.lang.String")
        _jpype.fault("PyJPClass_setClass")
        with self.assertRaises(SystemError):
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
        with self.assertRaises(SystemError):
            js._hints
        _jpype.fault("PyJPModule_getContext")
        with self.assertRaises(SystemError):
            js._hints
        self.assertIsInstance(js._hints, _jpype._JClassHints)

    def testJPClass_setHints(self):
        js = JClass("java.lang.String")
        _jpype.fault("PyJPClass_setHints")
        with self.assertRaises(SystemError):
            js._hints = None

    def testJPClass_cnaConvertToJava(self):
        js = JClass("java.lang.String")
        _jpype.fault("PyJPClass_canConvertToJava")
        with self.assertRaises(SystemError):
            js._canConvertToJava("f")
        _jpype.fault("PyJPModule_getContext")
        with self.assertRaises(SystemError):
            js._canConvertToJava("f")
        js._canConvertToJava("f")

    def testJPClass_cast(self):
        js = JClass("java.lang.String")
        _jpype.fault("PyJPClass_cast")
        with self.assertRaises(SystemError):
            js._cast("f")
        with self.assertRaises(TypeError):
            js._cast(object())
        _jpype.fault("PyJPModule_getContext")
        with self.assertRaises(SystemError):
            js._cast(JObject(None))
        js._cast(JObject(None))

    def testJPClass_convertToJava(self):
        js = JClass("java.lang.String")
        _jpype.fault("PyJPClass_convertToJava")
        with self.assertRaises(SystemError):
            js._convertToJava("f")
        with self.assertRaises(TypeError):
            js._convertToJava(object())
        _jpype.fault("PyJPModule_getContext")
        with self.assertRaises(SystemError):
            js._convertToJava("f")
        js._convertToJava("f")

    def testJPClassHints_new(self):
        _jpype.fault("PyJPClassHints_new")
        with self.assertRaises(SystemError):
            _jpype._JClassHints()
        _jpype._JClassHints()

    def testJPClassHints_init(self):
        _jpype.fault("PyJPClassHints_init")
        with self.assertRaises(SystemError):
            _jpype._JClassHints()
        _jpype._JClassHints()

    def testJPClassHints_str(self):
        _jpype.fault("PyJPClassHints_str")
        with self.assertRaises(SystemError):
            str(_jpype._JClassHints())
        str(_jpype._JClassHints())

    def testJPClassHints_addAttributeConversion(self):
        _jpype.fault("PyJPClassHints_addAttributeConversion")
        with self.assertRaises(SystemError):
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
        with self.assertRaises(SystemError):
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
        with self.assertRaises(SystemError):
            _jpype._JMonitor(jo)
        _jpype.fault("PyJPModule_getContext")
        with self.assertRaises(SystemError):
            _jpype._JMonitor(jo)
        _jpype._JMonitor(jo)

    def testJPMonitor_str(self):
        jo = JClass("java.lang.Object")()
        jm = _jpype._JMonitor(jo)
        _jpype.fault("PyJPMonitor_str")
        with self.assertRaises(SystemError):
            str(jm)
        _jpype.fault("PyJPModule_getContext")
        with self.assertRaises(SystemError):
            str(jm)

    def testJPMonitor_enter(self):
        jo = JClass("java.lang.Object")()
        _jpype.fault("PyJPMonitor_enter")
        with self.assertRaises(SystemError):
            with _jpype._JMonitor(jo):
                pass
        _jpype.fault("PyJPModule_getContext")
        with self.assertRaises(SystemError):
            with _jpype._JMonitor(jo):
                pass

    def testJPMonitor_exit(self):
        jo = JClass("java.lang.Object")()
        _jpype.fault("PyJPMonitor_exit")
        with self.assertRaises(SystemError):
            with _jpype._JMonitor(jo):
                pass

    def testJPNumber_new(self):
        _jpype.fault("PyJPNumber_new")
        class MyNum(_jpype._JNumberLong):
            pass
        with self.assertRaises(SystemError):
            JInt(1)
        with self.assertRaises(TypeError):
            MyNum(1)
        _jpype.fault("PyJPModule_getContext")
        with self.assertRaises(SystemError):
            JInt(1)
        JInt(1)

    def testJPNumberLong_int(self):
        ji = JInt(1)
        _jpype.fault("PyJPNumberLong_int")
        with self.assertRaises(SystemError):
            int(ji)
        _jpype.fault("PyJPModule_getContext")
        with self.assertRaises(SystemError):
            int(ji)
        int(ji)

    def testJPNumberLong_float(self):
        ji = JInt(1)
        _jpype.fault("PyJPNumberLong_float")
        with self.assertRaises(SystemError):
            float(ji)
        _jpype.fault("PyJPModule_getContext")
        with self.assertRaises(SystemError):
            float(ji)
        float(ji)

    def testJPNumberLong_str(self):
        ji = JInt(1)
        _jpype.fault("PyJPNumberLong_str")
        with self.assertRaises(SystemError):
            str(ji)
        _jpype.fault("PyJPModule_getContext")
        with self.assertRaises(SystemError):
            str(ji)
        str(ji)

    def testJPNumberLong_repr(self):
        ji = JInt(1)
        _jpype.fault("PyJPNumberLong_repr")
        with self.assertRaises(SystemError):
            repr(ji)
        _jpype.fault("PyJPModule_getContext")
        with self.assertRaises(SystemError):
            repr(ji)
        repr(ji)

    def testJPNumberLong_compare(self):
        ji = JInt(1)
        _jpype.fault("PyJPNumberLong_compare")
        with self.assertRaises(SystemError):
            ji==1
        _jpype.fault("PyJPModule_getContext")
        with self.assertRaises(SystemError):
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
        with self.assertRaises(SystemError):
            int(jd)
        _jpype.fault("PyJPModule_getContext")
        with self.assertRaises(SystemError):
            int(jd)
        int(jd)

    def testJPNumberFloat_float(self):
        jd = JDouble(1)
        _jpype.fault("PyJPNumberFloat_float")
        with self.assertRaises(SystemError):
            float(jd)
        _jpype.fault("PyJPModule_getContext")
        with self.assertRaises(SystemError):
            float(jd)
        float(jd)

    def testJPNumberFloat_str(self):
        jd = JDouble(1)
        _jpype.fault("PyJPNumberFloat_str")
        with self.assertRaises(SystemError):
            str(jd)
        _jpype.fault("PyJPModule_getContext")
        with self.assertRaises(SystemError):
            str(jd)
        str(jd)

    def testJPNumberFloat_repr(self):
        jd = JDouble(1)
        _jpype.fault("PyJPNumberFloat_repr")
        with self.assertRaises(SystemError):
            repr(jd)
        _jpype.fault("PyJPModule_getContext")
        with self.assertRaises(SystemError):
            repr(jd)
        repr(jd)

    def testJPNumberFloat_compare(self):
        jd = JDouble(1)
        _jpype.fault("PyJPNumberFloat_compare")
        with self.assertRaises(SystemError):
            jd==1
        _jpype.fault("PyJPModule_getContext")
        with self.assertRaises(SystemError):
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
        with self.assertRaises(SystemError):
            JChar("a")
        _jpype.fault("PyJPModule_getContext")
        with self.assertRaises(SystemError):
            JChar("a")
        JChar("a")

    def testJPChar_str(self):
        jc = JChar("a")
        _jpype.fault("PyJPChar_str")
        with self.assertRaises(SystemError):
            str(jc)
        _jpype.fault("PyJPModule_getContext")
        str(jc)

    def testJPBoolean_str(self):
        jb = JBoolean(True)
        _jpype.fault("PyJPBoolean_str")
        with self.assertRaises(SystemError):
            str(jb)
        _jpype.fault("PyJPModule_getContext")
        str(jb)

    def testJPObject_new(self):
        _jpype.fault("PyJPObject_new")
        with self.assertRaises(SystemError):
            JString("a")
        _jpype.fault("PyJPModule_getContext")
        with self.assertRaises(SystemError):
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
        with self.assertRaises(SystemError):
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
        with self.assertRaises(SystemError):
            JInt(1)
        _jpype.fault("PyJPModule_getContext")
        with self.assertRaises(SystemError):
            JInt(1)
        JInt(1)

    def testJPValue_finalize(self):
        _jpype.fault("PyJPValue_finalize")
        a = JInt(1)
        del a # lgtm [py/unnecessary-delete] 

    def testJPValue_str(self):
        js = JString("f")
        _jpype.fault("PyJPValue_str")
        with self.assertRaises(SystemError):
            str(js)
        _jpype.fault("PyJPModule_getContext")
        with self.assertRaises(SystemError):
            str(js)
        str(js)

    def testJPObject_getattro(self):
        jo = JString("f")
        _jpype.fault("PyJPObject_getattro")
        with self.assertRaises(SystemError):
            jo.substring
        _jpype.fault("PyJPModule_getContext")
        with self.assertRaises(SystemError):
            jo.substring
        jo.substring

    def testJPObject_setattro(self):
        jo = JString("f")
        _jpype.fault("PyJPObject_setattro")
        with self.assertRaises(SystemError):
            jo.substring = None

    def testJPBooleanType(self):
        ja = JArray(JBoolean)(5) # lgtm [py/similar-function]
        _jpype.fault("JPBooleanType::setArrayRange")
        with self.assertRaises(SystemError):
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
        with self.assertRaises(SystemError):
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
        with self.assertRaises(SystemError):
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
        with self.assertRaises(SystemError):
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
        with self.assertRaises(SystemError):
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
        with self.assertRaises(SystemError):
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
        with self.assertRaises(SystemError):
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
        with self.assertRaises(SystemError):
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
        with self.assertRaises(SystemError):
            jf.static_int = 2
        _jpype.fault("JPField::setAttribute")
        with self.assertRaises(SystemError):
            jfi.member_int = 2
        _jpype.fault("JPField::getStaticAttribute")
        with self.assertRaises(SystemError):
            i = jf.static_int
        _jpype.fault("JPField::getAttribute")
        with self.assertRaises(SystemError):
            i = jfi.member_int
        si = jf.__dict__['static_int']
        str(si)
        repr(si)
        i =  None
        _jpype.fault("PyJPField_get")
        with self.assertRaises(SystemError):
            i = jfi.member_int
        self.assertEqual(i, None)
        _jpype.fault("PyJPField_set")
        with self.assertRaises(SystemError):
            jfi.member_int = 2
        _jpype.fault("PyJPField_repr")
        with self.assertRaises(SystemError):
            repr(si)

    def testConvertString(self):
        _jpype.fault("JPObjectType::canConvertToJava")
        with self.assertRaises(SystemError):
            JObject._convertToJava("foo")
        _jpype.fault("JPConversionString::matches")
        with self.assertRaises(SystemError):
            JString._convertToJava("foo")


    def testJPObject(self):
        jf = JClass("jpype.fields.Fields")
        jfi = JClass("jpype.fields.Fields")()
        _jpype.fault("JPClass::setStaticField")
        with self.assertRaises(SystemError):
            jf.static_object = None
        _jpype.fault("JPClass::setField")
        with self.assertRaises(SystemError):
            jfi.member_object = None
        i = None
        _jpype.fault("JPClass::getStaticField")
        with self.assertRaises(SystemError):
            i = jf.static_object
        _jpype.fault("JPClass::getField")
        with self.assertRaises(SystemError):
            i = jfi.member_object
        self.assertEqual(i, None)
