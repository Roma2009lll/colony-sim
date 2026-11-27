add_executable(colony_cli
    main.cpp
)

target_link_libraries(colony_cli
    PRIVATE
    colony_core
)

if(COMMAND colony_set_warnings)
    colony_set_warnings(colony_cli)
endif()
