add_library(
    SimpleSql STATIC
    src/connection_string_builder.cpp
    src/database_connection.cpp
    src/diagnostic_set.cpp
    src/environment.cpp
    src/result_set.cpp
    src/simql_strings.cpp
    src/statement_pool.cpp
    src/statement.cpp
    src/value_set.cpp
)

target_include_directories(
    SimpleSql
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
)

if (MSVC)
	target_compile_options(SimpleSql PRIVATE /W4 /EHsc)
else()
	target_compile_options(SimpleSql PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()