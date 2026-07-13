# JPype C++ Python extension

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
)
set_target_properties(_jpype PROPERTIES OUTPUT_NAME "_jpype")

# C++ Python extension
include_directories(
        common/include
        python/include
        ${JNI_INCLUDE_DIRS}
)


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
else()
    # Additional libraries for non-Windows
    target_link_libraries(_jpype PRIVATE
            dl # dynamic linker
    )
    # Keep frame pointers for better stack unwinding.
    add_compile_options(-fno-omit-frame-pointer)

    # even on OSX Python uses .so suffix instead of .dylib
    set_target_properties(_jpype PROPERTIES SUFFIX ".so")
endif()

# Only stage the extension into the wheel if scikit-build-core is building
if(SKBUILD)
    message(STATUS "install _jpype extension to ${DEST_EXTENSION}")
    install(TARGETS _jpype
            LIBRARY DESTINATION ${DEST_EXTENSION}
            RUNTIME DESTINATION ${DEST_EXTENSION}
    )
endif()
