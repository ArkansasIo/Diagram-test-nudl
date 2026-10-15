cmake_minimum_required(VERSION 3.20)
project(DiagramTestNUDL VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(mlta
    src/engine.cpp
)
target_include_directories(mlta PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
if(MSVC)
    target_compile_options(mlta PRIVATE /W4 /permissive-)
else()
    target_compile_options(mlta PRIVATE -Wall -Wextra -Wpedantic -Wconversion)
endif()

add_executable(mlta_demo examples/demo.cpp)
target_link_libraries(mlta_demo PRIVATE mlta)

enable_testing()
add_executable(mlta_tests tests/test_engine.cpp)
target_link_libraries(mlta_tests PRIVATE mlta)
add_test(NAME mlta_tests COMMAND mlta_tests)
