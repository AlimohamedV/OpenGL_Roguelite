cmake_minimum_required(VERSION 3.15)
project(DungeonRoguelite LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── Source files ──
set(SOURCES
    game/main.cpp
    game/world.cpp
    game/renderer.cpp
)

set(HEADERS
    game/types.h
    game/world.h
    game/renderer.h
    game/config.h
)

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# ── Include directories ──
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/game
    ${CMAKE_SOURCE_DIR}/lib
)

# ── Find OpenGL ──
find_package(OpenGL REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE OpenGL::GL OpenGL::GLU)

# ── Find FreeGLUT ──
# Try find_package first (works on Linux/macOS with system packages)
find_package(GLUT QUIET)
if(GLUT_FOUND)
    target_include_directories(${PROJECT_NAME} PRIVATE ${GLUT_INCLUDE_DIR})
    target_link_libraries(${PROJECT_NAME} PRIVATE ${GLUT_LIBRARIES})
else()
    # Fallback: MSYS2/MinGW — link directly
    message(STATUS "GLUT not found via find_package, trying direct link...")
    target_link_libraries(${PROJECT_NAME} PRIVATE freeglut)
endif()

# ── Platform-specific ──
if(WIN32)
    # Hide console window in release builds
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        set_target_properties(${PROJECT_NAME} PROPERTIES WIN32_EXECUTABLE TRUE)
    endif()
endif()

# ── Optimization ──
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_options(${PROJECT_NAME} PRIVATE -O2)
endif()

# ── Copy assets to build directory ──
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/assets $<TARGET_FILE_DIR:${PROJECT_NAME}>/assets
    COMMENT "Copying assets to build directory"
)

message(STATUS "===========================================")
message(STATUS " Dungeon Roguelite — Build configured")
message(STATUS " Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "===========================================")
