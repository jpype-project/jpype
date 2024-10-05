cmake_minimum_required (VERSION 3.20)

project("_jpype" LANGUAGES CXX)

find_package(Python3 REQUIRED COMPONENTS Interpreter Development)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_SHARED_MODULE_SUFFIX ".cp312-win_amd64.pyd")

set(PROJECT_ROOT "${CMAKE_CURRENT_SOURCE_DIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_ROOT})

set(D_SRC_COMMON ${PROJECT_ROOT}/native/common)
set(D_SRC_PYTHON ${PROJECT_ROOT}/native/python)

file(GLOB SrcFiles ${D_SRC_COMMON}/*.cpp ${D_SRC_PYTHON}/*.cpp)

add_compile_options(
"$<$<COMPILE_LANGUAGE:CXX>:-Od;>"
)


set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W3 /GL")
#target_sources(${PROJECT_NAME} MODULE ${SrcFiles})
add_library(${PROJECT_NAME} MODULE ${SrcFiles})
target_include_directories(${PROJECT_NAME} PRIVATE ${PROJECT_ROOT}/native/jni_include ${D_SRC_COMMON}/include  ${D_SRC_PYTHON}/include)
target_link_libraries(${PROJECT_NAME} PRIVATE Python3::Module)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
