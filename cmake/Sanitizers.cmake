function(enable_sanitizers target_name sanitizer_type)
    if(sanitizer_type STREQUAL "asan-ubsan")
        target_compile_options(${target_name} PRIVATE
            -fsanitize=address,undefined
            -fno-omit-frame-pointer
            -fno-sanitize-recover=undefined
        )
        target_link_options(${target_name} PRIVATE
            -fsanitize=address,undefined
            -fno-sanitize-recover=undefined
        )
    elseif(sanitizer_type STREQUAL "tsan")
        target_compile_options(${target_name} PRIVATE -fsanitize=thread)
        target_link_options(${target_name} PRIVATE -fsanitize=thread)
    endif()
endfunction()
