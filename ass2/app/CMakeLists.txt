# Build the app, using the HAL

include_directories(include)
file(GLOB MY_SOURCES "src/*.c")
add_executable(light_sampler ${MY_SOURCES})

# Make use of the libraries
target_link_libraries(light_sampler LINK_PRIVATE hal)
target_link_libraries(light_sampler LINK_PRIVATE lcd)
target_link_libraries(light_sampler LINK_PRIVATE lgpio)

# Copy executable to final location (change `light_sampler` to project name as needed)
add_custom_command(TARGET light_sampler POST_BUILD 
  COMMAND "${CMAKE_COMMAND}" -E copy 
     "$<TARGET_FILE:light_sampler>"
     "~/cmpt433/public/myApps/light_sampler" 
  COMMENT "Copying ARM executable to public NFS directory")
