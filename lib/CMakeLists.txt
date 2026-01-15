add_library(
    SimpleSql STATIC
    src/simql.cpp
    src/query.cpp
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