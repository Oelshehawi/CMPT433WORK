# CMakeList.txt for HAL
#   Build a library (`hal`) which exposes the header files as "hal/*.h"
#   Use header as: #include "hal/button.h"

include_directories(hal/include)

# Include directories for LCD libraries
include_directories(${CMAKE_SOURCE_DIR}/lcd/include)

# Find all C source files, including those in subdirectories
file(GLOB_RECURSE MY_SOURCES "src/*.c")

add_library(hal STATIC ${MY_SOURCES})

target_include_directories(hal PUBLIC include)

# Link against the LCD libraries (adjusted to use correct library name)
find_library(GPIOD_LIBRARY gpiod)
target_link_libraries(hal LINK_PRIVATE gpiod)
target_link_libraries(hal LINK_PRIVATE lcd)
