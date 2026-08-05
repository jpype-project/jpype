# JPype C++ Python extension

include_directories(
        common/include
        python/include
        ${JNI_INCLUDE_DIRS}
)

set(JPYPE_NATIVE_SRC
        common/jp_array.cpp
        common/jp_arrayclass.cpp
        common/jp_booleantype.cpp
        common/jp_boxedtype.cpp
        common/jp_buffer.cpp
        common/jp_buffertype.cpp
        common/jp_bytetype.cpp
        common/jp_chartype.cpp
        common/jp_class.cpp
        common/jp_classhints.cpp
        common/jp_classloader.cpp
        common/jp_classtype.cpp
        common/jp_context.cpp
        common/jp_conversioncache.cpp
        common/jp_convert.cpp
        common/jp_doubletype.cpp
        common/jp_encoding.cpp
        common/jp_exception.cpp
        common/jp_field.cpp
        common/jp_floattype.cpp
        common/jp_functional.cpp
        common/jp_gc.cpp
        common/jp_inttype.cpp
        common/jp_javaframe.cpp
        common/jp_longtype.cpp
        common/jp_method.cpp
        common/jp_methoddispatch.cpp
        common/jp_monitor.cpp
        common/jp_numbertype.cpp
        common/jp_objecttype.cpp
        common/jp_platform.cpp
        common/jp_primitivetype.cpp
        common/jp_proxy.cpp
        common/jp_reference_queue.cpp
        common/jp_shorttype.cpp
        common/jp_stringtype.cpp
        common/jp_tracer.cpp
        common/jp_typefactory.cpp
        common/jp_typemanager.cpp
        common/jp_value.cpp
        common/jp_voidtype.cpp
        python/jp_pythontypes.cpp
        python/pyjp_array.cpp
        python/pyjp_buffer.cpp
        python/pyjp_char.cpp
        python/pyjp_class.cpp
        python/pyjp_classhints.cpp
        python/pyjp_field.cpp
        python/pyjp_method.cpp
        python/pyjp_module.cpp
        python/pyjp_monitor.cpp
        python/pyjp_number.cpp
        python/pyjp_object.cpp
        python/pyjp_package.cpp
        python/pyjp_proxy.cpp
        python/pyjp_value.cpp
)

Python3_add_library(_jpype MODULE ${JPYPE_NATIVE_SRC})
target_link_libraries(_jpype PRIVATE Python3::Module)
# Set target properties for scikit-build export
set_target_properties(_jpype PROPERTIES
        PREFIX ""  # remove 'lib' prefix
        OUTPUT_NAME "_jpype"
        LIBRARY_OUTPUT_DIRECTORY ${DEST_EXTENSION}
)

if(ENABLE_TRACING)
    add_compile_definitions(JP_TRACING_ENABLE)
endif()

# Bind coverage directly to the target
if(ENABLE_COVERAGE AND NOT WIN32)
    message(STATUS "Adding coverage instrumentation flags to _jpype")

    target_compile_definitions(_jpype PRIVATE JP_INSTRUMENTATION)

    # Target-specific options are harder for CMake to overwrite
    target_compile_options(_jpype PRIVATE -O0 --coverage -ftest-coverage -ggdb)
    target_link_options(_jpype PRIVATE --coverage)
endif()

# Apply Windows-specific configuration
if(WIN32)
    # Define WIN32 macro (Windows-specific code)
    target_compile_definitions(_jpype PRIVATE WIN32=1)
    # Required: .pyd suffix
    set_target_properties(_jpype PROPERTIES SUFFIX ".pyd")

    # Required: export all symbols for Python module
    set_target_properties(_jpype PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)

    # Ensure /MD and exception model are set
    target_compile_options(_jpype PRIVATE /EHsc /MD)
elseif(APPLE)
    # 1. Force the compiler to target the native Apple libc++
    target_compile_options(_jpype PRIVATE "-stdlib=libc++")
    target_link_options(_jpype PRIVATE "-stdlib=libc++")

    # 2. Tell the linker to prioritize dynamic lookup (.dylib) over static (.a)
    target_link_options(_jpype PRIVATE "-Wl,-search_paths_first")
    # Keep frame pointers for better stack unwinding.
    add_compile_options(-fno-omit-frame-pointer)
    add_custom_command(TARGET _jpype POST_BUILD
            COMMAND sh -c "otool -L $<TARGET_FILE:_jpype> | grep -q libc++.1.dylib || (echo 'FATAL: _jpype.so not dynamically linked against libc++!' >&2 && exit 1)"
            VERBATIM
    )
elseif(LINUX OR UNIX) # make sure APPLE is handled before this.
    target_link_libraries(_jpype PRIVATE dl)
    target_link_options(_jpype PRIVATE "-Wl,-l:libstdc++.so.6")
    # 16-byte std::atomic<T> (JPConversionCache) isn't backed by a native
    # lock-free instruction on every target this ships to, so GCC/Clang emit
    # calls to __atomic_*_16, which live in libatomic rather than libc.
    target_link_libraries(_jpype PRIVATE atomic)

    # Keep frame pointers for better stack unwinding.
    add_compile_options(-fno-omit-frame-pointer)

    # even on OSX Python uses .so suffix instead of .dylib
    set_target_properties(_jpype PROPERTIES SUFFIX ".so")

    # safe-guard to prevent static linkage of libstdc++
    add_custom_command(TARGET _jpype POST_BUILD
            COMMAND sh -c "readelf -d $<TARGET_FILE:_jpype> | grep -q libstdc++.so.6 || (echo 'FATAL: _jpype.so not dynamically linked against libstdc++!' >&2 && exit 1)"
            VERBATIM
    )
else()
    message(WARNING "unhandled OS found!")
endif()


install(
    TARGETS _jpype
    LIBRARY DESTINATION ${DEST_EXTENSION} # Catches Linux/macOS (.so) and Windows MODULE (.pyd)
    RUNTIME DESTINATION ${DEST_EXTENSION} # Catch-all for Windows just in case it treats the .pyd as a DLL

    # We deliberately LEAVE OUT the ARCHIVE DESTINATION here.
    # This prevents Windows from shoving the unnecessary .lib export file into your final Python wheel!
)