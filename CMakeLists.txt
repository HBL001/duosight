set(CMAKE_CXX_STANDARD 17)

cmake_minimum_required(VERSION 3.16)
project(duosight)

# Core library and reader app
add_subdirectory(libduosight)
add_subdirectory(mlx90640-reader)

# Unit test target for i2cUtils
add_executable(test_i2cUtils
    unit-tests/test_i2cUtils.cpp
)

target_link_libraries(test_i2cUtils
    PRIVATE
        duosight
)

target_include_directories(test_i2cUtils
    PRIVATE
        ${CMAKE_SOURCE_DIR}/libduosight/include
)
