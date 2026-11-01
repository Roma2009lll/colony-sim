include(FetchContent)

FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0
)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_executable(unit_tests
    unit/GridTests.cpp
    unit/IdTests.cpp
    unit/RngTests.cpp
    world/MapLoaderTests.cpp
    world/TileTests.cpp
    world/WorldTests.cpp
)

target_link_libraries(unit_tests PRIVATE
    colony_core
    gtest_main
)

set_project_warnings(unit_tests)

# Каталог з тестовими даними (карти) для тестів, що читають файли.
target_compile_definitions(unit_tests PRIVATE
    COLONY_TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/data")

if(COLONY_SANITIZER AND NOT COLONY_SANITIZER STREQUAL "none")
    enable_sanitizers(unit_tests "${COLONY_SANITIZER}")
endif()

include(GoogleTest)
gtest_discover_tests(unit_tests PROPERTIES LABELS "unit")