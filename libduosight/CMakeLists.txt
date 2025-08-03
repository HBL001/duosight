# libduosight/CMakeLists.txt

# Define the library target
add_library(duosight STATIC
    src/i2cUtils.cpp
    src/mlx90640Transport.cpp                           # ← transport layer with I2C handlers
    ${CMAKE_CURRENT_SOURCE_DIR}/../mlx90640-library/functions/MLX90640_API.c  # ← Melexis API core
)


# Include the public headers
target_include_directories(duosight
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/mlx90640-library/headers 
)

# Add any required system libraries here
target_link_libraries(duosight
    PUBLIC
        # e.g., -lpthread or -lrt if needed
)
