add_library(colony_core STATIC
    src/placeholder.cpp
)

target_include_directories(colony_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

set_project_warnings(colony_core)

if(NOT COLONY_SANITIZER STREQUAL "none")
    enable_sanitizers(colony_core ${COLONY_SANITIZER})
endif()