# Script used to help generate stubs for typefactory.

# Native files were compiled to hold parameter names
#   javac -Xlint:unchecked -parameters -d classes native/java/org/jpype/manager/*

import jpype
import jpype.imports
jpype.startJVM(classpath='classes')

from java.lang.reflect import Modifier

className = "org.jpype.manager.TypeFactoryNative"
cppName = "JPTypeFactory"
prefix = "JPTypeFactory_"
cls = jpype.JClass(className)

jname_map = {
    'void': 'void',
    'boolean': 'jboolean',
    'byte': 'jbyte',
    'char': 'jchar',
    'short': 'jshort',
    'int': 'jint',
    'long': 'jlong',
    'float': 'jfloat',
    'double': 'jdouble',
    'class [J': 'jlongArray',
    'class java.lang.String': 'jstring',
    'class java.lang.Class': 'jclass',
}

def jconvert(classType):
    return jname_map.get(str(classType), 'jobject')

jspec_map = {
    'void': 'V',
    'boolean': 'Z',
    'byte': 'B',
    'char': 'C',
    'short': 'S',
    'int': 'I',
    'long': 'J',
    'float': 'F',
    'double': 'D',
}


def jspec(classType):
    desc = str(classType.getName())
    if desc in jspec_map:
        return jspec_map[desc]
    if desc.startswith('['):
        return desc
    return "L%s;" % desc


def unpackJavaMethod(method):
    out = dict()
    signature = ['(']
    params = ["JNIEnv *env"]
    name = method.getName()
    retType = jconvert(method.getReturnType())
    if not Modifier.isStatic(method.getModifiers()):
        params.append("jobject self")
    for param in method.getParameters():
        paramType = jconvert(param.getType())
        paramName = str(param.getName())
        params.append("%s %s" % (paramType, paramName))
        signature.append(jspec(param.getType()))
    signature.append(')')
    signature.append(jspec(method.getReturnType()))
    out['method'] = method
    out['name'] = str(method.getName())
    out['signature'] = "".join(signature)
    out['handler'] = "%s%s" % (prefix, name)
    out['decl'] = ('JNIEXPORT %s JNICALL %s%s(%s)' %
                   (retType, prefix, name, ', '.join(params)))
    return out


# Select native methods
methods = [unpackJavaMethod(m) for m in cls.class_.getDeclaredMethods(
) if Modifier.isNative(m.getModifiers())]
for method in methods:
    print(method['decl'])
    print(
        """{
	JP_TRACE_IN("%s");
	JPJavaFrame frame(env);
	try {


		return;
	} catch (JPypeException& ex)
	{
		ex.toJava();
	} catch (...)
	{
		frame.ThrowNew(JPJni::s_RuntimeExceptionClass, "unknown error occurred");
	}

	return NULL;
	JP_TRACE_OUT;
}

"""%(method['name']))


print("""
void %s::init()
{
	JPJavaFrame frame(32);
	JP_TRACE_IN("%s::init");

	jclass cls = JPClassLoader::findClass("%s");

	JNINativeMethod method[%s];
""" % (cppName, cppName, className, len(methods)))

for i, method in enumerate(methods):
    print("""\
	method[%s].name = (char*) "%s";
	method[%s].signature = (char*) "%s";
	method[%s].fnPtr = (void*) &%s;
""" % (i, method['name'], i, method['signature'], i, method['handler']))

print("""
	frame.GetMethodID(cls, "<init>", "()V");
	frame.RegisterNatives(cls, method, %s);
	JP_TRACE_OUT;
}


""" % (len(methods)))
