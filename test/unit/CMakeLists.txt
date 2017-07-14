#
# This file needs to be updated whenever a new test is added.
#
# all tests can be run from the build directory with `make test`

# Trivial example using gtest and gmock. Can be directly added here.
# To only run this single test, run from inside the build directory (cd test/unit && ./example)
add_executable(example example.cpp)
target_link_libraries(example gtest gmock_main)
add_test(NAME example_unit_test COMMAND example)

# Configuration parsing and tests
# To only run this single test, run from inside the build directory '(cd test/unit && ./configuration/configuration)'
add_subdirectory(configuration)
add_test(NAME configuration_unit_test
        WORKING_DIRECTORY
        COMMAND configuration)

# SipiImage tests
# To only run this single test, run from inside the build directory '(cd test/unit && ./sipiimage/sipiimage)'
add_subdirectory(sipiimage)
add_test(NAME sipiimage_unit_test
        WORKING_DIRECTORY
        COMMAND sipiimage)