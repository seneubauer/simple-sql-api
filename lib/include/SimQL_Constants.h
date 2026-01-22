#ifndef SimQL_Constants_header_h
#define SimQL_Constants_header_h

#include <unordered_map>
#include <cstdint>
#include <string>

namespace SimpleSqlConstants {

    // SimDatabase return codes values
    namespace ReturnCodes {

        // standard return codes
        static constexpr uint8_t SUCCESS                    = 0; // operation successful
        static constexpr uint8_t UNKNOWN                    = 255; // for return code definition fallback

        // query return codes
        static constexpr uint8_t Q_UNDEFINED_COLUMNS        = 0; // could not retrieve column metadata from the database
        static constexpr uint8_t Q_CALC_COLUMNS             = 0; // could not calculate the number of columns in the result set
        static constexpr uint8_t Q_DUPLICATE_COLUMNS        = 0; // potential duplicate columns found in result set
        static constexpr uint8_t Q_EMPTY_SQL                = 0; // provided SQL is empty
        static constexpr uint8_t Q_PREPARE                  = 0; // could not prepare the provided SQL statement
        static constexpr uint8_t Q_PARAMETER_CALC           = 0; // could not calculate the number of parameters
        static constexpr uint8_t Q_NO_PARAMETERS            = 0; // no parameters found in the prepared SQL statement
        static constexpr uint8_t Q_UNKNOWN_IO_TYPE          = 0; // could not determine the ODBC Input-Output type
        static constexpr uint8_t Q_UNKNOWN_BINDING_FAMILY   = 0; // could not determine the proper binding family
        static constexpr uint8_t Q_UNKNOWN_SQL_C_TYPE       = 0; // could not determine the ODBC C/SQL data type
        static constexpr uint8_t Q_NUMERIC_BIND             = 0; // cannot bind an unknown numeric type
        static constexpr uint8_t Q_BOOL_INT_BIND            = 0; // cannot bind an unknown boolean/integer type
        static constexpr uint8_t Q_GUID_BIND                = 0; // cannot bind an unknown GUID type
        static constexpr uint8_t Q_DATETIME_BIND            = 0; // cannot bind an unknown date/time type
        static constexpr uint8_t Q_BINDING                  = 0; // could not bind the provided parameter

        // database return codes
        static constexpr uint8_t D_STMT_HANDLE_ASSIGNMENT   = 0; // could not assign the statement handle
        static constexpr uint8_t D_ENV_HANDLE_ALLOC         = 0; // could not allocate the environment handle
        static constexpr uint8_t D_ODBC_VERSION3            = 0; // could not set ODBC Version 3
        static constexpr uint8_t D_DBC_HANDLE_ALLOC         = 0; // could not allocate the connection handle
        static constexpr uint8_t D_CONNECTION               = 0; // could not open a connection to the database
    }
    static std::unordered_map<uint8_t, std::string_view> return_code_definitions {
        {ReturnCodes::SUCCESS,                      string_view("process was successful")},
        {ReturnCodes::UNKNOWN,                      string_view("fallback return code for undefined unsigned 8bit integers")},
        {ReturnCodes::Q_UNDEFINED_COLUMNS,          string_view("could not retrieve the column metadata from the database")},
        {ReturnCodes::Q_CALC_COLUMNS,               string_view("could not calculate the number of columns found in the result set")},
        {ReturnCodes::Q_DUPLICATE_COLUMNS,          string_view("potential duplicate columns found in the result set")},
        {ReturnCodes::Q_EMPTY_SQL,                  string_view("the provided SQL statement is empty")},
        {ReturnCodes::Q_PREPARE,                    string_view("could not prepare the provided SQL statement")},
        {ReturnCodes::Q_PARAMETER_CALC,             string_view("could not calculate the number of parameters")},
        {ReturnCodes::Q_NO_PARAMETERS,              string_view("no parameters found in the prepared SQL statement")},
        {ReturnCodes::Q_UNKNOWN_IO_TYPE,            string_view("could not determine the ODBC Input/Output type")},
        {ReturnCodes::Q_UNKNOWN_BINDING_FAMILY,     string_view("could not determine the proper binding family")},
        {ReturnCodes::Q_UNKNOWN_SQL_C_TYPE,         string_view("could not determine the ODBC C/SQL data type")},
        {ReturnCodes::Q_NUMERIC_BIND,               string_view("cannot bind an unknown floating point type")},
        {ReturnCodes::Q_BOOL_INT_BIND,              string_view("cannot bind an unknown boolean or integer type")},
        {ReturnCodes::Q_GUID_BIND,                  string_view("cannot bind an unknown GUID type")},
        {ReturnCodes::Q_DATETIME_BIND,              string_view("cannot bind an unknown date/time type")},
        {ReturnCodes::Q_BINDING,                    string_view("could not bind the provided parameter")},
        {ReturnCodes::D_STMT_HANDLE_ASSIGNMENT,     string_view("could not assign the statement handle")},
        {ReturnCodes::D_ENV_HANDLE_ALLOC,           string_view("could not allocate the environment handle")},
        {ReturnCodes::D_ODBC_VERSION3,              string_view("could not set ODBC to version 3")},
        {ReturnCodes::D_DBC_HANDLE_ALLOC,           string_view("could not allocate the connection handle")},
        {ReturnCodes::D_CONNECTION,                 string_view("could not open a connection to the database")}
    };
    inline std::string_view& return_code_def(const uint8_t &return_code) {
        auto it = return_code_definitions.find(return_code);
        if (it != return_code_definitions.end())
            return it->second;
        return return_code_definitions[ReturnCodes::UNKNOWN];
    }

    // sets the upper limit for user defined statement pool size
    static constexpr uint8_t max_parallel_query_count = 8;
    static constexpr uint8_t max_statement_handle_pool_size = 16;

}

#endif