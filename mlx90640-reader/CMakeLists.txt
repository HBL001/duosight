cmake_minimum_required(VERSION 3.16)
project(mlx90640-reader)

# Find required Qt5 modules
find_package(Qt5 REQUIRED COMPONENTS Widgets Gui Core)

# Sources
set(SOURCES
    src/main.cpp
    src/MLX90640Reader.cpp
)

# Executable
add_executable(${PROJECT_NAME} ${SOURCES})

# Include headers
target_include_directories(${PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../libduosight/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../mlx90640-library/headers
        ${CMAKE_CURRENT_SOURCE_DIR}/include 
)

# Link libraries
target_link_libraries(${PROJECT_NAME}
    PRIVATE
        duosight
        Qt5::Widgets
        Qt5::Gui
        Qt5::Core
        m
)
