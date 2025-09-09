cmake_minimum_required(VERSION 3.20)

project(epiCBattle LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

include(FetchContent)

# Fetch raylib
set(FETCHCONTENT_QUIET OFF)
FetchContent_Declare(
  raylib
  GIT_REPOSITORY https://github.com/raysan5/raylib.git
  GIT_TAG 5.0
)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(raylib)

add_executable(epiCBattle
  src/main.cpp
  src/game_states.h
)

target_include_directories(epiCBattle PRIVATE src)

target_link_libraries(epiCBattle PRIVATE raylib)

if (WIN32)
  target_compile_definitions(epiCBattle PRIVATE NOMINMAX)
endif()

# Resources directory for runtime
add_custom_target(copy_assets ALL
  COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/models $<TARGET_FILE_DIR:epiCBattle>/models
  BYPRODUCTS $<TARGET_FILE_DIR:epiCBattle>/models
)

add_dependencies(epiCBattle copy_assets)


