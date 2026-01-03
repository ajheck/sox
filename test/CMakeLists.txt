# Test executable
add_executable(sox_tests main.c)

include(FetchContent)

FetchContent_Declare(
    minunit
    GIT_REPOSITORY https://github.com/siu/minunit.git
    GIT_TAG        master
)
FetchContent_MakeAvailable(minunit)

# Create functions that can be found using extern for testing
target_compile_definitions(sox PRIVATE SOX_MAKE_HELPERS_EXTERN)

# Add additional source
target_sources(sox_tests PRIVATE 
    test_helpers.c
    test_send.c
    test_inits.c
)

# Link against the main library
target_link_libraries(sox_tests PRIVATE sox)

# Test-specific include directories
target_include_directories(sox_tests PRIVATE 
    ${minunit_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
)
