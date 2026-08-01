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
    message(DEBUG "SKBuild invocation")
    set(DEST_JAR ${SKBUILD_PLATLIB_DIR})
    set(DEST_EXTENSION ${SKBUILD_PLATLIB_DIR})
else()
    # in-place build, set output to cmakes output dir.
    set(DEST_JAR ${CMAKE_CURRENT_BINARY_DIR})
    set(DEST_EXTENSION ${CMAKE_CURRENT_BINARY_DIR})
endif()

# we need to enforce debug builds, if we want to collect coverage.
if(ENABLE_COVERAGE)
    message(WARNING "ENABLE_COVERAGE set — forcing Debug build for coverage correctness")
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()


set(JPYPE_JAR_FN "org.jpype.jar")
set(DEST_JAR_FN "${DEST_JAR}/${JPYPE_JAR_FN}")

message(STATUS "Destination Python extension: ${DEST_EXTENSION}")
message(STATUS "Destination JAR: ${DEST_JAR}")

# We want to cache DEST_JAR, so people do not need a JDK, but only a C++ compiler to build jpype.
if(EXISTS "${SRC_JAR}")
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
    set(DEST_FILE "${CMAKE_CURRENT_BINARY_DIR}/${JPYPE_JAR_FN}")
    add_custom_command(
            OUTPUT "${DEST_FILE}"
            COMMAND ${CMAKE_COMMAND} -E copy "${JAR_OUTPUT}" "${DEST_FILE}"
            DEPENDS "${JAR_OUTPUT}"   # ensures it only runs after the source exists
            COMMENT "Copying ${JAR_OUTPUT} to internal destination"
    )
    add_custom_target(copy_jar ALL DEPENDS "${DEST_FILE}")
    # Explicitly link targets so MSBuild doesn't run them in parallel.
    add_dependencies(copy_jar jpype_jar)
    # finally install the jar to DEST_JAR, which makes it available for wheel building.
    install(
            FILES "${DEST_FILE}"
            DESTINATION ${DEST_JAR}
    )
endif()

# If javac is there just do it. Disabled by pip build by default.
if(Java_Development_FOUND)
    message(STATUS "Forcing: Build Test Java classes, because JDK is available.")
    set(BUILD_TEST_HARNESS ON CACHE BOOL "Build Test Java classes automatically" FORCE)
endif()

# fixme: if installed from sdist and run from git clone, the classes end up in the wheel (at least tmp build dir) and cannot be found.
# 1. copy classes output to cwd?
# 2. install classes somewhere and point conftest classpath towards it?
#
if(BUILD_TEST_HARNESS) # This requires a JDK of course.
    find_program(ANT_EXECUTABLE ant REQUIRED)
    
    # We define a representative output file to help CMake track dependencies
    # todo: cmake_source_dir is in tmp, if installing from sdist!
    set(TEST_HARNESS_OUTPUT ${CMAKE_CURRENT_LIST_DIR})
    message(STATUS "test harness output: ${TEST_HARNESS_OUTPUT}")
    set(TEST_SENTINEL "${TEST_HARNESS_OUTPUT}/test/classes/org/jpype/JPypeTest.class")

    add_custom_command(
        OUTPUT "${TEST_SENTINEL}"
        COMMAND "${ANT_EXECUTABLE}" -f "test/build.xml"
        WORKING_DIRECTORY "${TEST_HARNESS_OUTPUT}"
        DEPENDS jpype_jar  # Ensure the main org.jpype.jar is built first
        COMMENT "Building JPype Java Test Harness via Ant"
    )

    add_custom_target(build_tests ALL DEPENDS "${TEST_SENTINEL}")
endif()

add_subdirectory(native)
