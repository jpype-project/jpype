cmake_minimum_required(VERSION 3.16)

if(CMAKE_VERSION VERSION_LESS 3.25 AND CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(LINUX true)
endif()

project(jpype-native)

option(ENABLE_TRACING "Enable tracing in the extension" OFF)
option(ENABLE_COVERAGE "Enable compilation for coverage measurement" OFF)
option(ENABLE_BUILD_JAR "Enable building of the Java support library JAR" ON)
option(BUILD_TEST_HARNESS "Enable building of the Java test harness" ON)
# Optionally set Python GIL disabled flag
# set(PY_GIL_DISABLED ON)
set(CMAKE_CXX_STANDARD 14)
set(SRC_JAR "${CMAKE_CURRENT_SOURCE_DIR}/org.jpype.jar")
set(EDITABLE FALSE)

find_package(Python3 COMPONENTS Interpreter Development.Module REQUIRED)
find_package(Python3 COMPONENTS Development.Embed)
find_package(Java COMPONENTS Development) # Java is optional during the build process, if a cached JAR exists.
find_package(JNI)

message(DEBUG "Java_FOUND='${Java_FOUND}'")
message(DEBUG "Java_JAVA_EXECUTABLE='${Java_JAVA_EXECUTABLE}'")
message(DEBUG "Java_JAVAC_EXECUTABLE='${Java_JAVAC_EXECUTABLE}'")
message(DEBUG "Java_Development_FOUND='${Java_Development_FOUND}'")


if(NOT JNI_FOUND)
    message(STATUS "System JNI headers not found. Falling back to bundled jni.h.")
    set(JNI_INCLUDE_DIRS "${CMAKE_SOURCE_DIR}/native/jni_include")
endif()

message(STATUS "Final JNI include dirs: ${JNI_INCLUDE_DIRS}")


if(SKBUILD)
    set(DEST_EXTENSION ${SKBUILD_PLATLIB_DIR})
    if(SKBUILD_STATE STREQUAL "editable")
        set(EDITABLE TRUE)
        # redirect mode keeps __file__ pointing at the real source tree, so
        # runtime jar lookup (relative to __file__) needs it here, not in
        # the editable install staging dir.
        set(DEST_JAR "${CMAKE_SOURCE_DIR}/")
    else()
        set(DEST_JAR ${SKBUILD_PLATLIB_DIR})
    endif()
else()
    set(DEST_JAR ${CMAKE_CURRENT_BINARY_DIR})
    set(DEST_EXTENSION ${CMAKE_CURRENT_BINARY_DIR})
endif()

# we need to enforce debug builds, if we want to collect coverage.
if(ENABLE_COVERAGE)
    message(WARNING "ENABLE_COVERAGE set — forcing Debug build for coverage correctness")
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
    # JaCoCo can only report line coverage for classes with line number tables.
    set(ANT_ARGS "-Djavac.debug=true")
endif()


set(JPYPE_JAR_FN "org.jpype.jar")
set(DEST_JAR_FN "${DEST_JAR}/${JPYPE_JAR_FN}")

message(STATUS "Destination Python extension: ${DEST_EXTENSION}")
message(STATUS "Destination JAR: ${DEST_JAR}")

# Development builds (editable or plain CMake) with a JDK always compile the
# JAR from source (this requires Ant), so Java changes are picked up.  In an
# editable build, SRC_JAR is the previous build's output (DEST_JAR is the
# source dir), so treating it as a cache would freeze the Java code.
# Wheel builds keep using the cached JAR and need no Ant.
if(Java_Development_FOUND AND (NOT SKBUILD OR EDITABLE))
    message(STATUS "JDK available (dev/editable build). Building JAR from source.")
    set(ENABLE_BUILD_JAR ON)
# We want to cache DEST_JAR, so people do not need a JDK, but only a C++ compiler to build jpype.
elseif(EXISTS "${SRC_JAR}")
    message(STATUS "FOUND ${SRC_JAR}. Using from cache.")
    file(COPY_FILE "${SRC_JAR}" "${DEST_JAR_FN}")
elseif(Java_Development_FOUND)
    message(STATUS "No cached JAR found, Java dev toolchain available. Forcing building JAR.")
    set(ENABLE_BUILD_JAR ON CACHE BOOL "Build JAR automatically" FORCE)
else()
    if(NOT ENABLE_BUILD_JAR)
        message(FATAL_ERROR "JAR building disabled, but NO cached JAR and NO JDK found!")
    endif()
endif()

# Build Java support library
if(ENABLE_BUILD_JAR)
    message(STATUS "Building JAR")

    find_program(ANT_EXECUTABLE ant REQUIRED)
    # this is the build artifact of Ant.
    set(JAR_OUTPUT "${CMAKE_SOURCE_DIR}/native/build/lib/${JPYPE_JAR_FN}")

    # Copy the Ant build artifact to current cmake output dir (required for editable installs).
    set(DEST_FILE "${CMAKE_CURRENT_BINARY_DIR}/${JPYPE_JAR_FN}")

    # Run Ant on every build: CMake doesn't know the Java sources, and Ant
    # only recompiles what changed, so this is cheap when nothing did.
    # Both commands are in one target, so they run in order (also on MSBuild).
    add_custom_target(jpype_jar ALL
            COMMAND "${ANT_EXECUTABLE}" -f "${CMAKE_SOURCE_DIR}/native/build.xml" ${ANT_ARGS}
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "${JAR_OUTPUT}" "${DEST_FILE}"
            BYPRODUCTS "${JAR_OUTPUT}" "${DEST_FILE}"
            WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
            COMMENT "Building ${JAR_OUTPUT} with Apache Ant"
    )
    # finally install the jar to DEST_JAR, which makes it available for wheel building.
    install(
            FILES "${DEST_FILE}"
            DESTINATION ${DEST_JAR}
    )
endif()

# Only auto-enable the test harness for local development — never for an
# actual sdist/wheel build, where CMAKE_SOURCE_DIR is a disposable
# extraction dir and the compiled classes would just be discarded.
if(Java_Development_FOUND AND (NOT SKBUILD OR ${EDITABLE}))
    message(STATUS "Forcing: Build Test Java classes, because JDK is available (dev/editable build).")
    set(BUILD_TEST_HARNESS ON CACHE BOOL "Build Test Java classes automatically" FORCE)
endif()

if(BUILD_TEST_HARNESS AND (NOT SKBUILD OR ${EDITABLE}))
    find_program(ANT_EXECUTABLE ant REQUIRED)
    set(TEST_HARNESS_OUTPUT ${CMAKE_SOURCE_DIR})

    # Run Ant on every build (it is incremental), so changes to the harness
    # sources are picked up.
    add_custom_target(build_tests ALL
        COMMAND "${ANT_EXECUTABLE}" -f "test/build.xml"
        WORKING_DIRECTORY "${TEST_HARNESS_OUTPUT}"
        COMMENT "Building JPype Java Test Harness via Ant"
    )
elseif(BUILD_TEST_HARNESS)
    message(STATUS "Skipping Java test harness build (SKBUILD_STATE=${SKBUILD_STATE}) — dev/editable only.")
endif()

add_subdirectory(native)
