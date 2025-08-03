cmake_minimum_required(VERSION 3.16)
project(mlx90640-reader)

# Add the source files for this specific app
set(SOURCES
    src/main.cpp
    src/MLX90640Reader.cpp
    src/MLX90640Reader.h
    # add more if needed
)

# Add executable
add_executable(${PROJECT_NAME} ${SOURCES})

# Link to shared library (libduosight)
target_link_libraries(${PROJECT_NAME}
    PRIVATE
        duosight
)

# Include libduosight headers
target_include_directories(${PROJECT_NAME}
    PRIVATE
        ${PROJECT_SOURCE_DIR}/../libduosight/include
)
