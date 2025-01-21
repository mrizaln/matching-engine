find_package(ut)

function(make_test NAME)
    add_executable(${NAME} ${CMAKE_CURRENT_SOURCE_DIR}/${NAME}.cpp)
    target_link_libraries(${NAME} PRIVATE matching-engine Boost::ut)

    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(${NAME} PRIVATE -Wall -Wextra -Wconversion)
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
        target_compile_options(${NAME} PRIVATE /W4)
    endif()

    # sanitizer (only on GCC or Clang)
    if(
        CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang"
        AND MATCHING_ENGINE_ENABLE_SANITIZERS
    )
        target_compile_options(
            ${NAME}
            PRIVATE -fsanitize=address,leak,undefined
        )
        target_link_options(
            ${NAME} #
            PRIVATE -fsanitize=address,leak,undefined
        )
    endif()

    add_test(NAME ${NAME} COMMAND $<TARGET_FILE:${NAME}>)

    if(MATCHING_ENGINE_RUN_TESTS_AFTER_BUILD)
        add_custom_command(
            TARGET ${NAME}
            POST_BUILD
            COMMAND $<TARGET_FILE:${NAME}>
        )
    endif()
endfunction()

make_test(match_engine_test)
