add_library(glad src/glad.c)

target_include_directories(glad PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# On Windows, link against opengl32.lib
if (WIN32)
    target_link_libraries(glad PUBLIC opengl32)
endif()