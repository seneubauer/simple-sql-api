#ifndef simql_strings_header_h
#define simql_strings_header_h

// STL stuff
#include <string>
#include <string_view>
#include <memory>
#include <cstdint>

// OS stuff
#include "os_inclusions.hpp"

// ODBC stuff
#include <sqltypes.h>
#include <sqlext.h>
#include <sql.h>

namespace simql_strings {
    std::basic_string<SQLWCHAR> to_odbc_w(std::basic_string_view<char> utf8);
    std::basic_string<SQLCHAR> to_odbc_n(std::basic_string_view<char> utf8);
    std::basic_string<char> from_odbc(std::basic_string_view<SQLWCHAR> odbc);
    std::basic_string<char> from_odbc(std::basic_string_view<SQLCHAR> odbc);
    SQLWCHAR to_odbc_char_w(char utf8);
    SQLCHAR to_odbc_char_n(char utf8);
    char from_odbc_char(SQLWCHAR odbc);
    char from_odbc_char(SQLCHAR odbc);
}

#endif