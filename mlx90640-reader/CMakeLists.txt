cmake_minimum_required(VERSION 3.16)
project(mlx90640-reader)

# Sources for this target
set(SOURCES
    src/main.cpp
    src/MLX90640Reader.cpp
)

# Executable target
add_executable(${PROJECT_NAME} ${SOURCES})

# Link to shared duosight library
target_link_libraries(${PROJECT_NAME}
    PRIVATE
        duosight
)

# Include headers from duosight and mlx90640-library
target_include_directories(${PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../libduosight/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../mlx90640-library/headers
)

# Link MLX90640_API.c manually from submodule
target_sources(${PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../mlx90640-library/functions/MLX90640_API.c
)

# Link standard math library if needed
target_link_libraries(${PROJECT_NAME}
    PRIVATE
        m
)
