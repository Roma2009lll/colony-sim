add_library(colony_core STATIC
    src/world/MapLoader.cpp
    src/decision/Decision.cpp
    src/sim/CommandLog.cpp
    src/sim/Resolver.cpp
    src/sim/Simulation.cpp
    src/sim/StateHash.cpp
)

target_include_directories(colony_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(colony_core PUBLIC cxx_std_20)

set_project_warnings(colony_core)

if(COLONY_SANITIZER AND NOT COLONY_SANITIZER STREQUAL "none")
    enable_sanitizers(colony_core "${COLONY_SANITIZER}")
endif()  