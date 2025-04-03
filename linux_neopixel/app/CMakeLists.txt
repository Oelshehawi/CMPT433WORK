# Build the app, using the HAL

include_directories(include)
file(GLOB MY_SOURCES "src/*.c")
add_executable(linux_neopixel ${MY_SOURCES})

# Make use of the libraries
target_link_libraries(linux_neopixel LINK_PRIVATE hal)
target_link_libraries(linux_neopixel LINK_PRIVATE lcd)
target_link_libraries(linux_neopixel LINK_PRIVATE lgpio)

# Copy executable to final location
add_custom_command(TARGET linux_neopixel POST_BUILD 
  COMMAND "${CMAKE_COMMAND}" -E copy 
     "$<TARGET_FILE:linux_neopixel>"
     "~/cmpt433/public/myApps/linux_neopixel" 
  COMMENT "Copying ARM executable to public NFS directory")
