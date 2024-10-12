cmake_minimum_required (VERSION 3.30)

project("_jpype" LANGUAGES CXX)

set (Python_FIND_ABI "ANY" "ANY" "ANY" "ON")
set(Python_Interpreter_GIL_DISABLED ON)
find_package(Python3 REQUIRED COMPONENTS Interpreter Development)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_SHARED_MODULE_SUFFIX ".cp313t-win_amd64.pyd")

set(PROJECT_ROOT "${CMAKE_CURRENT_SOURCE_DIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_ROOT})

set(D_SRC_COMMON ${PROJECT_ROOT}/native/common)
set(D_SRC_PYTHON ${PROJECT_ROOT}/native/python)

file(GLOB SrcFiles ${D_SRC_COMMON}/*.cpp ${D_SRC_PYTHON}/*.cpp)

add_compile_options(
"$<$<COMPILE_LANGUAGE:CXX>:-Od;>"
)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /DPy_GIL_DISABLED=1 /W3 /GL")
#target_sources(${PROJECT_NAME} MODULE ${SrcFiles})
add_library(${PROJECT_NAME} MODULE ${SrcFiles})
target_include_directories(${PROJECT_NAME} PRIVATE "C:\\Program Files\\Python313\\include" ${PROJECT_ROOT}/native/jni_include ${D_SRC_COMMON}/include  ${D_SRC_PYTHON}/include)
#target_link_libraries(${PROJECT_NAME} PRIVATE Python3::Module)
target_link_directories(${PROJECT_NAME} PRIVATE "C:\\Program Files\\Python313\\libs")
