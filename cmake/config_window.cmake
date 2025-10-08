# =========================
# config_windows.cmake
# =========================

# Arguments: target name and debug flag
# Usage: config_windows(my_extension ON)

function(config_windows target debug)
    message(STATUS "Applying Windows-specific build settings for target ${target}")

    # 1. Define WIN32 macro (Windows-specific code)
    target_compile_definitions(${target} PRIVATE WIN32=1)

    # 2. C++ standard
    target_compile_features(${target} PRIVATE cxx_std_14)

    # 3. Runtime and exception flags
    target_compile_options(${target} PRIVATE /O2 /EHsc /MD)

    # 4. Optional debug info
    if(${debug})
        message(STATUS "Adding debug info (/Zi)")
        target_compile_options(${target} PRIVATE /Zi)
    endif()

    # 5. Optional: Py_GIL_DISABLED macro
    # If your CMake build knows Python is built without GIL
    # You can set a CMake variable PY_GIL_DISABLED=ON and then:
    if(DEFINED PY_GIL_DISABLED AND PY_GIL_DISABLED)
        target_compile_definitions(${target} PRIVATE Py_GIL_DISABLED=1)
    endif()
endfunction()
