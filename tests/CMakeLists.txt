include_directories("${CMAKE_SOURCE_DIR}/src")

# Define the test executable
add_executable(cheetah-tests
    main.cpp
    FlowTableTests.cpp
)

# Link the test executable with Google Test and MyLibrary
target_link_libraries(cheetah-tests PumaSDK gtest pthread)
