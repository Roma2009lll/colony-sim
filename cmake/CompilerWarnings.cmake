function(set_project_warnings target_name)
    set(GCC_CLANG_WARNINGS
        -Wall
        -Wextra
        -Wpedantic
        -Wshadow
        -Wconversion
        -Wnon-virtual-dtor
        -Wold-style-cast
        -Wcast-align
        -Wunused
        -Woverloaded-virtual
    )

    target_compile_options(${target_name} PRIVATE ${GCC_CLANG_WARNINGS})
endfunction()