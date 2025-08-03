cmake_minimum_required(VERSION 3.16)
project(mlx90640-reader)

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
        m
)
