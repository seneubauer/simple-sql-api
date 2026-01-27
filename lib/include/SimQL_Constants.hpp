#ifndef SimQL_Constants_header_h
#define SimQL_Constants_header_h

#include <unordered_map>
#include <cstdint>
#include <string_view>

namespace SimpleSqlConstants {

    namespace DatabaseDrivers {
        static constexpr std::string_view ODBC_17_SQL_SERVER    = std::string_view("ODBC Driver 17 for SQL Server");
        static constexpr std::string_view PSQL_ODBC_ANSI        = std::string_view("PostgreSQL ANSI");
        static constexpr std::string_view PSQL_ODBC_UNI         = std::string_view("PostgreSQL UNICODE");
    }

    namespace Limits {
        static constexpr std::uint8_t max_sql_column_name_size = 255;
        static constexpr std::uint8_t max_parallel_query_count = 8;
        static constexpr std::uint8_t max_statement_handle_pool_size = 16;
    }

}

#endif
