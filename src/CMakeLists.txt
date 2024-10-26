include_directories("${CMAKE_CURRENT_SOURCE_DIR}")

# Modules
add_subdirectory(Common)

add_executable("${PROJECT_NAME}" main.cpp)

target_link_libraries("${PROJECT_NAME}" PRIVATE 
    PumaSDK
)
