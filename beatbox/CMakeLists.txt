# CMake Build Configuration for root of project
cmake_minimum_required(VERSION 3.18)
project(my_hello_world VERSION 1.0 DESCRIPTION "Starter project" LANGUAGES C)

# Compiler options (inherited by sub-folders)
set(CMAKE_C_STANDARD 11)
add_compile_options(-Wall -Werror -g -Wpedantic -Wextra)
add_compile_options(-fdiagnostics-color)

# Enable address sanitizer
# (Comment this out to make your code faster)
# add_compile_options(-fsanitize=address)
# add_link_options(-fsanitize=address)

# What folders to build
add_subdirectory(hal)  
add_subdirectory(app)
add_subdirectory(lcd)
add_subdirectory(lgpio)

# Support GPIO
find_library(GPIOD_LIBRARY gpiod)
target_link_libraries(beatbox LINK_PRIVATE gpiod)
target_link_libraries(hal LINK_PRIVATE gpiod)