cmake_minimum_required(VERSION 3.20)
project(jpype-native)

option(ENABLE_TRACING "Enable tracing in the extension" OFF)
option(ENABLE_COVERAGE "Enable compilation for coverage measurement" OFF)
option(ENABLE_BUILD_JAR "Enable building of the Java JAR" ON)
# Optionally set Python GIL disabled flag
# set(PY_GIL_DISABLED ON)
set(CMAKE_CXX_STANDARD 14)

find_package(Python3 COMPONENTS Interpreter Development REQUIRED)
find_package(Java REQUIRED COMPONENTS Development)
# todo: add fallback jni.h in case it aint found (no jdk required, or not found).
set(JNI_INCLUDE_DIRS "${CMAKE_SOURCE_DIR}/native/jni_include" ${JNI_INCLUDE_DIRS})
find_package(JNI REQUIRED)
include(UseJava)
message("JAVA_INCLUDE_PATH: ${JNI}")

if(SKBUILD)
    set(DEST_JAR ${SKBUILD_PLATLIB_DIR}/jpype) # jar should go to jpype python package.
    set(DEST_EXTENSION ${SKBUILD_PLATLIB_DIR})
else()
    set(DEST_JAR ${CMAKE_CURRENT_BINARY_DIR})
    set(DEST_EXTENSION ${CMAKE_CURRENT_BINARY_DIR})
endif()

if(ENABLE_BUILD_JAR)
    message("build jar")
    file(GLOB_RECURSE JAVA_SOURCES "native/jpype_module/src/main/java/*.java")
    #set(SOME_DEP_JAR="asm-8.0.1.jar") # tODO: test lib gets included.
    set(JAR_FILE "org.jpype") # note: .jar suffix will be appended by add_jar
    add_jar(${JAR_FILE}
            SOURCES ${JAVA_SOURCES}
            #INCLUDE_JARS ${SOME_DEP_JAR}
            OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}"
    )
    # install in Jar in JPypes Python package data directory.
    install(FILES
            ${CMAKE_CURRENT_BINARY_DIR}/${JAR_FILE}.jar
            DESTINATION ${DEST_JAR}
    )
endif()


if(ENABLE_TRACING)
    add_compile_definitions(JP_TRACING_ENABLE)
endif()

if(ENABLE_COVERAGE)
    if(NOT CMAKE_BUILD_TYPE)
        message(STATUS "ENABLE_COVERAGE set — forcing Debug build for coverage correctness")
        set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
    elseif(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
        message(WARNING "ENABLE_COVERAGE set but CMAKE_BUILD_TYPE is ${CMAKE_BUILD_TYPE}; coverage may be unreliable")
    endif()

    add_compile_definitions(JP_INSTRUMENTATION)
    message(STATUS "Adding coverage instrumentation flags")
    if(MSVC)
        warn("Coverage not supported on windows.")
    endif()
    # Enable debug symbols
    add_compile_options(-ggdb)

    # Enable coverage instrumentation (GCC/Clang)
    add_compile_options(-O0 --coverage -ftest-coverage)
    add_link_options(--coverage)
endif()

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
target_link_libraries(_jpype PRIVATE ${PYTHON_LIBRARIES})

# Apply Windows-specific configuration
if(WIN32)
    # Define WIN32 macro (Windows-specific code)
    target_compile_definitions(${target} PRIVATE WIN32=1)
    # Runtime and exception flags
    target_compile_options(${target} PRIVATE /O2 /EHsc /MD)
else()
    # Additional libraries for non-Windows
    target_link_libraries(_jpype PRIVATE
            dl # dynamic linker
    )
endif()


# Set target properties for scikit-build export
set_target_properties(_jpype PROPERTIES
        PREFIX ""  # remove 'lib' prefix
        #LIBRARY_OUTPUT_DIRECTORY "${JPYPE_WHEEL_OUTPUT_DIR}"
        #LIBRARY_OUTPUT_DIRECTORY "$<$<CONFIG:Debug>:${JPYPE_WHEEL_OUTPUT_DIR}/Debug>"
        #LIBRARY_OUTPUT_DIRECTORY_RELEASE "$<$<CONFIG:Release>:${JPYPE_WHEEL_OUTPUT_DIR}/Release>"
        #LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${Python3_SITEARCH}" # todo: this is only desired for building wheel, not during development
)

# Only stage the extension into the wheel if scikit-build-core is building
if(SKBUILD)
    message("installing _jpype")
    install(TARGETS _jpype
            LIBRARY DESTINATION ${DEST_EXTENSION}
            RUNTIME DESTINATION ${DEST_EXTENSION}
    )
else()
    message("no skbuild env")
endif()
