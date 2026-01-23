#ifndef SimQL_Constants_header_h
#define SimQL_Constants_header_h

#include <unordered_map>
#include <cstdint>
#include <string>

namespace SimpleSqlConstants {

    namespace DatabaseDrivers {
        static std::string ODBC_17_SQL_SERVER                   = "ODBC Driver 17 for SQL Server";
    }

    namespace ReturnCodes {

        // standard return codes
        static constexpr std::uint8_t SUCCESS                   = 0;
        static constexpr std::uint8_t UNKNOWN                   = 255;

        // query return codes
        static constexpr std::uint8_t Q_UNDEFINED_COLUMNS       = 1;
        static constexpr std::uint8_t Q_CALC_COLUMNS            = 2;
        static constexpr std::uint8_t Q_DUPLICATE_COLUMNS       = 3;
        static constexpr std::uint8_t Q_EMPTY_SQL               = 4;
        static constexpr std::uint8_t Q_PREPARE                 = 5;
        static constexpr std::uint8_t Q_PARAMETER_CALC          = 6;
        static constexpr std::uint8_t Q_NO_PARAMETERS           = 7;
        static constexpr std::uint8_t Q_UNKNOWN_IO_TYPE         = 8;
        static constexpr std::uint8_t Q_UNKNOWN_BINDING_FAMILY  = 9;
        static constexpr std::uint8_t Q_UNKNOWN_SQL_C_TYPE      = 10;
        static constexpr std::uint8_t Q_NUMERIC_BIND            = 11;
        static constexpr std::uint8_t Q_BOOL_INT_BIND           = 12;
        static constexpr std::uint8_t Q_GUID_BIND               = 13;
        static constexpr std::uint8_t Q_DATETIME_BIND           = 14;
        static constexpr std::uint8_t Q_BINDING_NOT_SET         = 15;
        static constexpr std::uint8_t Q_BINDING                 = 16;

        // database return codes
        static constexpr std::uint8_t D_STMT_HANDLE_ASSIGNMENT  = 17;
        static constexpr std::uint8_t D_ENV_HANDLE_ALLOC        = 18;
        static constexpr std::uint8_t D_ODBC_VERSION3           = 19;
        static constexpr std::uint8_t D_DBC_HANDLE_ALLOC        = 20;
        static constexpr std::uint8_t D_CONNECTION              = 21;
    }
    static std::unordered_map<std::uint8_t, std::string_view> return_code_definitions {
        {ReturnCodes::SUCCESS,                      std::string_view("process was successful")},
        {ReturnCodes::UNKNOWN,                      std::string_view("fallback return code for undefined unsigned 8bit integers")},
        {ReturnCodes::Q_UNDEFINED_COLUMNS,          std::string_view("could not retrieve the column metadata from the database")},
        {ReturnCodes::Q_CALC_COLUMNS,               std::string_view("could not calculate the number of columns found in the result set")},
        {ReturnCodes::Q_DUPLICATE_COLUMNS,          std::string_view("potential duplicate columns found in the result set")},
        {ReturnCodes::Q_EMPTY_SQL,                  std::string_view("the provided SQL statement is empty")},
        {ReturnCodes::Q_PREPARE,                    std::string_view("could not prepare the provided SQL statement")},
        {ReturnCodes::Q_PARAMETER_CALC,             std::string_view("could not calculate the number of parameters")},
        {ReturnCodes::Q_NO_PARAMETERS,              std::string_view("no parameters found in the prepared SQL statement")},
        {ReturnCodes::Q_UNKNOWN_IO_TYPE,            std::string_view("could not determine the ODBC Input/Output type")},
        {ReturnCodes::Q_UNKNOWN_BINDING_FAMILY,     std::string_view("could not determine the proper binding family")},
        {ReturnCodes::Q_UNKNOWN_SQL_C_TYPE,         std::string_view("could not determine the ODBC C/SQL data type")},
        {ReturnCodes::Q_NUMERIC_BIND,               std::string_view("cannot bind an unknown floating point type")},
        {ReturnCodes::Q_BOOL_INT_BIND,              std::string_view("cannot bind an unknown boolean or integer type")},
        {ReturnCodes::Q_GUID_BIND,                  std::string_view("cannot bind an unknown GUID type")},
        {ReturnCodes::Q_DATETIME_BIND,              std::string_view("cannot bind an unknown date/time type")},
        {ReturnCodes::Q_BINDING_NOT_SET,            std::string_view("the binding family is undefined")},
        {ReturnCodes::Q_BINDING,                    std::string_view("could not bind the provided parameter")},
        {ReturnCodes::D_STMT_HANDLE_ASSIGNMENT,     std::string_view("could not assign the statement handle")},
        {ReturnCodes::D_ENV_HANDLE_ALLOC,           std::string_view("could not allocate the environment handle")},
        {ReturnCodes::D_ODBC_VERSION3,              std::string_view("could not set ODBC to version 3")},
        {ReturnCodes::D_DBC_HANDLE_ALLOC,           std::string_view("could not allocate the connection handle")},
        {ReturnCodes::D_CONNECTION,                 std::string_view("could not open a connection to the database")}
    };
    inline std::string_view& return_code_def(const std::uint8_t &return_code) {
        auto it = return_code_definitions.find(return_code);
        if (it != return_code_definitions.end())
            return it->second;
        return return_code_definitions[ReturnCodes::UNKNOWN];
    }

    static constexpr std::uint8_t max_sql_column_name_size = 255;
    static constexpr std::uint8_t max_parallel_query_count = 8;
    static constexpr std::uint8_t max_statement_handle_pool_size = 16;

}

#endif