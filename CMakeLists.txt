set(CMAKE_CXX_STANDARD 17)

cmake_minimum_required(VERSION 3.16)
project(duosight)

# 🧩 Add this block for Qt5
find_package(Qt5 REQUIRED COMPONENTS Widgets Gui Core)

# Core library and reader app
add_subdirectory(libduosight)
add_subdirectory(mlx90640-reader)

# Unit test: i2cUtils only
add_executable(test_i2cUtils
    unit-tests/test_i2cUtils.cpp
)
target_link_libraries(test_i2cUtils PRIVATE duosight)
target_include_directories(test_i2cUtils PRIVATE
    ${CMAKE_SOURCE_DIR}/libduosight/include
)

# Unit test: MLX90640Reader (requires its .cpp)
add_executable(test_mlx90640_reader
    unit-tests/test_mlx90640_reader.cpp
    mlx90640-reader/src/MLX90640Reader.cpp
)
target_link_libraries(test_mlx90640_reader PRIVATE duosight)
target_include_directories(test_mlx90640_reader PRIVATE
    ${CMAKE_SOURCE_DIR}/libduosight/include
    ${CMAKE_SOURCE_DIR}/mlx90640-reader/include
)

# Unit test: MLX90640 GUI (Qt5 standalone)
add_executable(test_mlx90640_gui
    unit-tests/test_mlx90640_gui.cpp
    mlx90640-reader/src/MLX90640Reader.cpp
)
target_link_libraries(test_mlx90640_gui PRIVATE duosight Qt5::Widgets Qt5::Gui Qt5::Core)
target_include_directories(test_mlx90640_gui PRIVATE
    ${CMAKE_SOURCE_DIR}/libduosight/include
    ${CMAKE_SOURCE_DIR}/mlx90640-reader/include
)
