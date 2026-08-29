# Define the test executable
add_executable(engine_tests
    PlayerProgress_test.cpp
)

# Link the test executable against our engine and Google Test
target_link_libraries(engine_tests PRIVATE
    engine
    gtest_main
)

# Include Google Test in this project
include(GoogleTest)

# Automatically discover and add all tests from the executable to CTest
gtest_discover_tests(engine_tests)
