include(GoogleTest)

set(TEST_SOURCES
    test_timer.cpp
    test_profiler.cpp
    test_exporter.cpp
    test_process_manager.cpp
)

add_executable(runscope_tests ${TEST_SOURCES})

target_link_libraries(runscope_tests
    runscope_core
    GTest::gtest
    GTest::gtest_main
)

if(APPLE)
    target_link_libraries(runscope_tests c++)
endif()

target_include_directories(runscope_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

gtest_discover_tests(runscope_tests)
