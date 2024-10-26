# Define the test executable
add_executable(Cheetah-tests
    CheetahTests.cpp
)

# Link the test executable with Google Test and MyLibrary
target_link_libraries(Cheetah-tests PumaSDK gtest gtest_main pthread)

# Add the test to CTest
add_test(NAME Cheetah-tests COMMAND Cheetah-tests)
