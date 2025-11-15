add_executable(basic_example basic_example.cpp)
target_link_libraries(basic_example runscope_core)

add_executable(target_app target_app.cpp)

add_executable(attach_test attach_test.cpp)
target_link_libraries(attach_test runscope_core)

if(APPLE)
    target_link_libraries(basic_example c++)
endif()

if(RUNSCOPE_BUILD_IMGUI)
    add_executable(profiler_app profiler_app.cpp)
    target_link_libraries(profiler_app runscope_core runscope_imgui)

    if(APPLE)
        target_link_libraries(profiler_app c++)
    endif()
endif()
