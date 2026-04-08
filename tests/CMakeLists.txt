find_package(doctest REQUIRED COMPONENTS doctest doctest_discover_tests)

add_executable(TestDriver)

target_sources(TestDriver
    PRIVATE
        main.cpp

    PRIVATE
    FILE_SET HEADERS
    BASE_DIRS .
    FILES
        string_conversions.hpp
)

target_link_libraries(TestDriver
    PRIVATE 
        ToyMakerCompilerFlags
        doctest::doctest
)

add_subdirectory(ToyMaker_Main)

include(doctest)
doctest_discover_tests(TestDriver)
