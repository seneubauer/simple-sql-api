#ifndef SimQL_Constants_header_h
#define SimQL_Constants_header_h

#include <unordered_map>
#include <cstdint>
#include <string>

namespace SimpleSqlConstants {

    // SimDatabase return codes values
    static constexpr std::uint8_t SUCCESS                       = 0;
    static constexpr std::uint8_t CONNECTION                    = 1;
    static constexpr std::uint8_t STMT_HANDLE_ASSIGNMENT        = 2;
    static constexpr std::uint8_t STMT_HANDLE_RECLAIM           = 3;
    static constexpr std::uint8_t ENV_HANDLE_ALLOC              = 4;
    static constexpr std::uint8_t DBC_HANDLE_ALLOC              = 5;
    static constexpr std::uint8_t STMT_HANDLE_ALLOC             = 6;
    static constexpr std::uint8_t ODBC_VERSION3                 = 7;
    static constexpr std::uint8_t INTERNAL                      = 8;
    static constexpr std::uint8_t UNKNOWN                       = 255;

    static std::unordered_map<std::uint8_t, std::string_view> return_code_definitions {
        {CONNECTION_ERROR,              std::string_view("could not create an ODBC connection to the database")},
        {STMT_HANDLE_ASSIGNMENT_ERROR,  std::string_view("could not assign an ODBC Statement handle to the query")},
        {STMT_HANDLE_RECLAIM_ERROR,     std::string_view("could not reclaim the ODBC Statement handle from the query")},
        {UNKNOWN_ERROR,                 std::string_view("unknown error code")}
    };
    inline std::string_view& return_code_def(const std::uint8_t &return_code) {
        auto it = return_code_definitions.find(return_code);
        if (it != return_code_definitions.end())
            return it->second;
        return return_code_definitions[UNKNOWN_ERROR];
    }

    // sets the upper limit for user defined statement pool size
    static constexpr std::uint8_t max_parallel_query_count = 8;
    static constexpr std::uint8_t max_statement_handle_pool_size = 16;

}

#endif