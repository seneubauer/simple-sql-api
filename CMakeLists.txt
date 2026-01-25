cmake_minimum_required(VERSION 3.15)
project(SimpleSql LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(ODBC REQUIRED)

add_subdirectory(lib)
add_subdirectory(test)

target_link_libraries(SimpleSql PRIVATE ODBC::ODBC)

#target_link_libraries(SimpleSql PRIVATE ${ODBC_LIBRARIES})
#target_include_directories(SimpleSql PRIVATE ${ODBC_INCLUDE_DIRS})
