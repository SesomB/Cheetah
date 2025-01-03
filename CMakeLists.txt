cmake_minimum_required(VERSION 3.16)

project(cheetah)

set (CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
SET(CMAKE_CXX_FLAGS "-march=native")

add_subdirectory(lib)
add_subdirectory(src)

enable_testing()
add_subdirectory(tests)
add_subdirectory(benchmarks)

# Optional: Define any compiler options or flags here
target_compile_options("${PROJECT_NAME}" PRIVATE 
    -Wall 
    -Wextra 
    # -Werror
)
