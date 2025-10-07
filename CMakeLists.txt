cmake_minimum_required(VERSION 3.20)
project(jpype-native)

set(CMAKE_CXX_STANDARD 11)

find_package(Python3 COMPONENTS Interpreter Development REQUIRED)
find_package(Java REQUIRED COMPONENTS Development)
find_package(JNI REQUIRED)  # if you need the native interface

include(UseJava)  # or include(Java) in older setups

# jar
file(GLOB_RECURSE JAVA_SOURCES "native/jpype_module/*.java")
add_jar(jpype_jar
        SOURCES ${JAVA_SOURCES}
        INCLUDE_JARS ${SOME_DEP_JAR}
        OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}"
)


# C++ Python extension
include_directories(
        native/common/include
        native/jni_include
        native/python/include
)

set(JPYPE_NATIVE_SRC
        native/common/jp_array.cpp
        native/common/jp_arrayclass.cpp
        native/common/jp_booleantype.cpp
        native/common/jp_boxedtype.cpp
        native/common/jp_buffer.cpp
        native/common/jp_buffertype.cpp
        native/common/jp_bytetype.cpp
        native/common/jp_chartype.cpp
        native/common/jp_class.cpp
        native/common/jp_classhints.cpp
        native/common/jp_classloader.cpp
        native/common/jp_classtype.cpp
        native/common/jp_context.cpp
        native/common/jp_convert.cpp
        native/common/jp_doubletype.cpp
        native/common/jp_encoding.cpp
        native/common/jp_exception.cpp
        native/common/jp_field.cpp
        native/common/jp_floattype.cpp
        native/common/jp_functional.cpp
        native/common/jp_gc.cpp
        native/common/jp_inttype.cpp
        native/common/jp_javaframe.cpp
        native/common/jp_longtype.cpp
        native/common/jp_method.cpp
        native/common/jp_methoddispatch.cpp
        native/common/jp_monitor.cpp
        native/common/jp_numbertype.cpp
        native/common/jp_objecttype.cpp
        native/common/jp_platform.cpp
        native/common/jp_primitivetype.cpp
        native/common/jp_proxy.cpp
        native/common/jp_reference_queue.cpp
        native/common/jp_shorttype.cpp
        native/common/jp_stringtype.cpp
        native/common/jp_tracer.cpp
        native/common/jp_typefactory.cpp
        native/common/jp_typemanager.cpp
        native/common/jp_value.cpp
        native/common/jp_voidtype.cpp
        native/python/jp_pythontypes.cpp
        native/python/pyjp_array.cpp
        native/python/pyjp_buffer.cpp
        native/python/pyjp_char.cpp
        native/python/pyjp_class.cpp
        native/python/pyjp_classhints.cpp
        native/python/pyjp_field.cpp
        native/python/pyjp_method.cpp
        native/python/pyjp_module.cpp
        native/python/pyjp_monitor.cpp
        native/python/pyjp_number.cpp
        native/python/pyjp_object.cpp
        native/python/pyjp_package.cpp
        native/python/pyjp_proxy.cpp
        native/python/pyjp_value.cpp
)

Python3_add_library(_jpype SHARED ${JPYPE_NATIVE_SRC})
target_link_libraries(_jpype ${PYTHON_LIBRARIES})
