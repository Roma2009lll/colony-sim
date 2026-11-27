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
    unit/SlotMapTests.cpp
    world/MapLoaderTests.cpp
    world/TileTests.cpp
    world/WorldTests.cpp
    agents/AgentTests.cpp
    agents/NeedsTests.cpp
    pathfinding/AStarTests.cpp
    pathfinding/BFSTest.cpp
    pathfinding/GridGraphTests.cpp
    tasks/PriorityPolicyTests.cpp
    tasks/TaskBoardTests.cpp
    tasks/TaskTests.cpp
    ai/DecisionTest.cpp
    sim/CommandLogTests.cpp
    sim/ResolverTests.cpp
    sim/SimulationTests.cpp
    sim/StateHashTests.cpp
)

target_compile_features(unit_tests PRIVATE cxx_std_20)

target_link_libraries(unit_tests PRIVATE
    colony_core
    gtest_main
)

set_project_warnings(unit_tests)

target_compile_definitions(unit_tests PRIVATE
    COLONY_TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/data")

if(COLONY_SANITIZER AND NOT COLONY_SANITIZER STREQUAL "none")
    enable_sanitizers(unit_tests "${COLONY_SANITIZER}")
endif()

include(GoogleTest)
gtest_discover_tests(unit_tests PROPERTIES LABELS "unit")

if(TARGET colony_cli)
    set(CLI_MAPS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/data/maps")
    set(CLI_CMDS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/data/commands")
    set(CLI_VALID_MAP "${CLI_MAPS_DIR}/valid_colony.txt")

    add_test(NAME cli_help COMMAND $<TARGET_FILE:colony_cli> --help)
    set_tests_properties(cli_help PROPERTIES
        LABELS "integration"
        PASS_REGULAR_EXPRESSION "Usage:.*--map"
    )

    add_test(NAME cli_missing_map COMMAND $<TARGET_FILE:colony_cli> --seed 42)
    set_tests_properties(cli_missing_map PROPERTIES
        LABELS "integration"
        WILL_FAIL TRUE
    )

    add_test(NAME cli_nonexistent_map COMMAND $<TARGET_FILE:colony_cli> --map "${CLI_MAPS_DIR}/no_such_map.txt")
    set_tests_properties(cli_nonexistent_map PROPERTIES
        LABELS "integration"
        WILL_FAIL TRUE
    )

    add_test(NAME cli_invalid_seed COMMAND $<TARGET_FILE:colony_cli> --map "${CLI_VALID_MAP}" --seed not_a_number)
    set_tests_properties(cli_invalid_seed PROPERTIES
        LABELS "integration"
        WILL_FAIL TRUE
    )

    foreach(SCENARIO valid_basic valid_unsorted_and_comments valid_comments_only)
        add_test(
            NAME "cli_${SCENARIO}"
            COMMAND $<TARGET_FILE:colony_cli>
            --map "${CLI_VALID_MAP}"
            --commands "${CLI_CMDS_DIR}/${SCENARIO}.txt"
            --seed 42
            --ticks 35
        )
        set_tests_properties("cli_${SCENARIO}" PROPERTIES
            LABELS "integration"
            PASS_REGULAR_EXPRESSION "Simulation finished at tick 35 \\(seed: 42\\);Final state hash: [0-9a-f]{16}"
        )
    endforeach()

    foreach(BAD_SCENARIO invalid_unknown_cmd invalid_args invalid_trailing_tokens)
        add_test(
            NAME "cli_${BAD_SCENARIO}"
            COMMAND $<TARGET_FILE:colony_cli>
            --map "${CLI_VALID_MAP}"
            --commands "${CLI_CMDS_DIR}/${BAD_SCENARIO}.txt"
        )
        set_tests_properties("cli_${BAD_SCENARIO}" PROPERTIES
            LABELS "integration"
            WILL_FAIL TRUE
        )
    endforeach()

    add_test(
        NAME cli_determinism_check
        COMMAND ${CMAKE_COMMAND}
        -DCLI_BIN=$<TARGET_FILE:colony_cli>
        -DMAP_FILE=${CLI_VALID_MAP}
        -DCMD_FILE=${CLI_CMDS_DIR}/valid_basic.txt
        -P "${CMAKE_CURRENT_BINARY_DIR}/check_cli_determinism.cmake"
    )
    set_tests_properties(cli_determinism_check PROPERTIES LABELS "integration")

    file(GENERATE
        OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/check_cli_determinism.cmake"
        CONTENT [[
            execute_process(
                COMMAND "${CLI_BIN}" --map "${MAP_FILE}" --commands "${CMD_FILE}" --seed 777 --ticks 30
                OUTPUT_VARIABLE OUT_1
                RESULT_VARIABLE RES_1
            )
            execute_process(
                COMMAND "${CLI_BIN}" --map "${MAP_FILE}" --commands "${CMD_FILE}" --seed 777 --ticks 30
                OUTPUT_VARIABLE OUT_2
                RESULT_VARIABLE RES_2
            )
            if(NOT RES_1 EQUAL 0 OR NOT RES_2 EQUAL 0)
                message(FATAL_ERROR "CLI failed during determinism check")
            endif()
            if(NOT OUT_1 STREQUAL OUT_2)
                message(FATAL_ERROR "Non-deterministic CLI output:\nRun 1:\n${OUT_1}\nRun 2:\n${OUT_2}")
            endif()
        ]]
    )
endif()
