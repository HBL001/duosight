# libduosight/CMakeLists.txt

# Define the library target
add_library(duosight STATIC
    src/i2c_utils.cpp
    src/platform_abstraction.cpp
    # add more source files as needed
)

# Include the public headers
target_include_directories(duosight
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Add any required system libraries here
target_link_libraries(duosight
    PUBLIC
        # e.g., -lpthread or -lrt if needed
)
