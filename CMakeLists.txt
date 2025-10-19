cmake_minimum_required(VERSION 3.16)
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
#set(JAVA_INCLUDE_PATH "${CMAKE_SOURCE_DIR}/native/jni_include" ${JAVA_INCLUDE_PATH})
find_package(JNI REQUIRED)
include(UseJava)


if(SKBUILD)
    message("SKBuild invocation")
    set(DEST_JAR ${SKBUILD_PLATLIB_DIR}) # todo: jar should go to JPype Python package?
    set(DEST_EXTENSION ${SKBUILD_PLATLIB_DIR})
else()
    # in-place build, set output to cmakes output dir.
    set(DEST_JAR ${CMAKE_CURRENT_BINARY_DIR})
    set(DEST_EXTENSION ${CMAKE_CURRENT_BINARY_DIR})
endif()

if(ENABLE_BUILD_JAR)
    message("build jar")

    find_program(ANT_EXECUTABLE ant REQUIRED)
    # this is the build artifact of Ant.
    set(JAR_OUTPUT "${CMAKE_SOURCE_DIR}/native/build/lib/org.jpype.jar")

    # we also need to copy the JAR_OUTPUT to CMAKE_CURRENT_BINARY_DIR
    add_custom_command(
            OUTPUT "${JAR_OUTPUT}"
            COMMAND "${ANT_EXECUTABLE}" -f "${CMAKE_SOURCE_DIR}/native/build.xml"
            WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
            COMMENT "Building ${JAR_OUTPUT} with Apache Ant"
    )
    # link ant command with output jar
    add_custom_target(jpype_jar ALL DEPENDS "${JAR_OUTPUT}")

    # Copy the Ant build artifact to current cmake output dir (required for editable installs).
    set(DEST_FILE "${CMAKE_CURRENT_BINARY_DIR}/org.jpype.jar")
    add_custom_command(
            OUTPUT "${DEST_FILE}"
            COMMAND ${CMAKE_COMMAND} -E copy "${JAR_OUTPUT}" "${DEST_FILE}"
            DEPENDS "${JAR_OUTPUT}"   # ensures it only runs after the source exists
            COMMENT "Copying ${JAR_OUTPUT} to internal destination"
    )
    add_custom_target(copy_jar ALL DEPENDS "${DEST_FILE}")

    # finally install the jar to DEST_JAR, which makes it available for wheel building.
    install(
            FILES "${DEST_FILE}"
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

Python3_add_library(_jpype MODULE ${JPYPE_NATIVE_SRC})
target_link_libraries(_jpype PRIVATE Python3::Python)
# Set target properties for scikit-build export
set_target_properties(_jpype PROPERTIES
        PREFIX ""  # remove 'lib' prefix
)

#fixme: causes problems on osx (would need to build shared lib, not module.
#set_target_properties(_jpype PROPERTIES VERSION "1.6.1.dev0")

# Apply Windows-specific configuration
if(WIN32)
    # Define WIN32 macro (Windows-specific code)
    target_compile_definitions(_jpype PRIVATE WIN32=1)
    # Required: .pyd suffix
    set_target_properties(_jpype PROPERTIES SUFFIX ".pyd")

    # Required: export all symbols for Python module
    set_target_properties(_jpype PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)

    set_target_properties(_jpype PROPERTIES     OUTPUT_NAME "_jpype")

    # Ensure /MD and exception model are set
    target_compile_options(_jpype PRIVATE /O2 /EHsc /MD)
else()
    # Additional libraries for non-Windows
    target_link_libraries(_jpype PRIVATE
            dl # dynamic linker
    )
    # even on OSX Python uses .so suffix instead of .dylib
    set_target_properties(_jpype PROPERTIES SUFFIX ".so")
endif()


# Only stage the extension into the wheel if scikit-build-core is building
#if(SKBUILD)
#    message(DEBUG "install _jpype extension")
#    install(TARGETS _jpype
#            LIBRARY DESTINATION ${DEST_EXTENSION}
#            RUNTIME DESTINATION ${DEST_EXTENSION}
#    )
#else()
#    message(DEBUG "no skbuild env")
#endif()
# cross-platform install
install(TARGETS _jpype
        LIBRARY DESTINATION ${Python_SITELIB})