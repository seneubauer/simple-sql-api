#ifndef simql_constants_header_h
#define simql_constants_header_h

#include <unordered_map>
#include <cstdint>
#include <string_view>

namespace simql_constants {

    namespace database_drivers {
        static constexpr std::string_view odbc_17_sql_server    = std::string_view("ODBC Driver 17 for SQL Server");
        static constexpr std::string_view psql_odbc_ansi        = std::string_view("PostgreSQL ANSI");
        static constexpr std::string_view psql_odbc_uni         = std::string_view("PostgreSQL UNICODE");
        static constexpr std::string_view psql_odbc             = std::string_view("PostgreSQL");
    }

    namespace limits {
        static constexpr std::uint8_t max_sql_column_name_size = 255;
        static constexpr std::uint8_t max_parallel_query_count = 8;
        static constexpr std::uint8_t max_statement_handle_pool_size = 16;
        static constexpr std::uint8_t min_statement_handle_pool_size = 4;
    }

}

#endif
