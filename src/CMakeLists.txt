set(RUNSCOPE_HEADERS
    ${CMAKE_SOURCE_DIR}/include/runscope/timer.hpp
    ${CMAKE_SOURCE_DIR}/include/runscope/profile_data.hpp
    ${CMAKE_SOURCE_DIR}/include/runscope/profiler.hpp
    ${CMAKE_SOURCE_DIR}/include/runscope/exporter.hpp
    ${CMAKE_SOURCE_DIR}/include/runscope/process_manager.hpp
    ${CMAKE_SOURCE_DIR}/include/runscope/runscope.hpp
)

set(RUNSCOPE_SOURCES
    exporter.cpp
    core/clock.cpp
    core/profiler_session.cpp
    core/profiler_engine.cpp
    core/scope_profiler.cpp
    platform/process_enumerator.cpp
    platform/process_attacher.cpp
    analysis/statistics.cpp
    export/exporter.cpp
)

add_library(runscope_core STATIC ${RUNSCOPE_SOURCES})
target_include_directories(runscope_core PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)
target_compile_features(runscope_core PUBLIC cxx_std_20)

if(RUNSCOPE_BUILD_IMGUI)
    set(IMGUI_SOURCES
        ${imgui_SOURCE_DIR}/imgui.cpp
        ${imgui_SOURCE_DIR}/imgui_draw.cpp
        ${imgui_SOURCE_DIR}/imgui_tables.cpp
        ${imgui_SOURCE_DIR}/imgui_widgets.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
    )
    
    add_library(runscope_imgui
        ui/profiler_ui.cpp
        ${IMGUI_SOURCES}
    )
    
    target_include_directories(runscope_imgui PUBLIC
        ${CMAKE_SOURCE_DIR}/include
        ${imgui_SOURCE_DIR}
        ${imgui_SOURCE_DIR}/backends
    )
    
    target_link_libraries(runscope_imgui PUBLIC
        runscope_core
        glfw
    )
    
    find_package(OpenGL REQUIRED)
    target_link_libraries(runscope_imgui PUBLIC OpenGL::GL)
    
    if(UNIX AND NOT APPLE)
        target_link_libraries(runscope_imgui PUBLIC dl pthread)
    endif()
endif()
