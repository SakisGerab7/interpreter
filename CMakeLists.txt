cmake_minimum_required(VERSION 3.16)

project(interp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

file(GLOB SRC_FILES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

add_executable(interp ${SRC_FILES})

target_include_directories(interp PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

target_compile_options(interp PRIVATE
    -O3
    -g
    # -pg
    -fsanitize=address
)

target_link_options(interp PRIVATE
    # -pg
    -fsanitize=address
)
