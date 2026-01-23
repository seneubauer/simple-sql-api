add_executable(test test.cpp)

target_include_directories(test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(test PRIVATE SimpleSql)

if (MSVC)
    target_compile_options(test PRIVATE /W4 /EHsc)
else()
    target_compile_options(test PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()