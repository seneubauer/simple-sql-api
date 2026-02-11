#ifndef SimQL_ReturnCodes_header_h
#define SimQL_ReturnCodes_header_h

#include <cstdint>
#include <string_view>
#include <unordered_map>

namespace SimQL_ReturnCodes {
    
    enum class Code : std::uint8_t {

        /* generic */
        SUCCESS,
        SUCCESS_INFO,
        NULLPTR,

        /* ENVIRONMENT HANDLE */
        ERROR_ALLOC_HANDLE,
        ERROR_SET_ODBC_VERSION3,
        ERROR_SET_POOLING_TYPE,
        ERROR_SET_POOL_MATCH_TYPE,

        /* CONNECTION HANDLE */
        ERROR_SET_ACCESS_MODE,
        ERROR_SET_CONNECTION_TIMEOUT,
        ERROR_SET_LOGIN_TIMEOUT,
        ERROR_SET_PACKET_SIZE,
        ERROR_SET_ASYNC,
        ERROR_SET_AUTOCOMMIT,
        ERROR_SET_TRACING,
        ERROR_SET_TRACEFILE,
        ERROR_OPEN_CONNECTION,
        ERROR_UNKNOWN_CONNECTION_STATE,

        /* STATEMENT HANDLE */
        ERROR_SET_CURSOR_TYPE,
        ERROR_SET_QUERY_TIMEOUT,
        ERROR_SET_MAX_ROWS,
        ERROR_SET_PARAM_BINDING,
        ERROR_SET_PARAM_DUPLICATE,
        ERROR_SET_PARAM_INVALID_DTYPE,
        ERROR_STMT_PREPARE,
        ERROR_STMT_EXECUTE,
        ERROR_STMT_COLUMN_CALC,
        INFO_STMT_NO_COLUMNS,
        ERROR_COL_BINDING,
        ERROR_COL_INVALID_DTYPE

    };
    static constexpr Code IS_NULLPTR = Code::NULLPTR;
}

namespace {
    const std::unordered_map<SimQL_ReturnCodes::Code, std::string_view> descriptions {
        {SimQL_ReturnCodes::Code::SUCCESS,                          std::string_view("success")},
        {SimQL_ReturnCodes::Code::SUCCESS_INFO,                     std::string_view("success but ODBC diagnostics were generated")},
        {SimQL_ReturnCodes::Code::NULLPTR,                          std::string_view("the underlying pointer is null")},
        {SimQL_ReturnCodes::Code::ERROR_ALLOC_HANDLE,               std::string_view("could not allocate the ODBC handle")},
        {SimQL_ReturnCodes::Code::ERROR_SET_ODBC_VERSION3,          std::string_view("could not set to ODBC version 3")},
        {SimQL_ReturnCodes::Code::ERROR_SET_POOLING_TYPE,           std::string_view("could not set the pooling type")},
        {SimQL_ReturnCodes::Code::ERROR_SET_POOL_MATCH_TYPE,        std::string_view("could not set the pool match type")},
        {SimQL_ReturnCodes::Code::ERROR_SET_ACCESS_MODE,            std::string_view("could not set the access mode")},
        {SimQL_ReturnCodes::Code::ERROR_SET_CONNECTION_TIMEOUT,     std::string_view("could not set the connection timeout")},
        {SimQL_ReturnCodes::Code::ERROR_SET_LOGIN_TIMEOUT,          std::string_view("could not set the login timeout")},
        {SimQL_ReturnCodes::Code::ERROR_SET_PACKET_SIZE,            std::string_view("could not set the packet size")},
        {SimQL_ReturnCodes::Code::ERROR_SET_ASYNC,                  std::string_view("could not set to asynchronous")},
        {SimQL_ReturnCodes::Code::ERROR_SET_AUTOCOMMIT,             std::string_view("could not set the autocommit state")},
        {SimQL_ReturnCodes::Code::ERROR_SET_TRACING,                std::string_view("could not enable/disable tracing")},
        {SimQL_ReturnCodes::Code::ERROR_SET_TRACEFILE,              std::string_view("could not set the tracefile path")},
        {SimQL_ReturnCodes::Code::ERROR_OPEN_CONNECTION,            std::string_view("could not open a connection to the database")},
        {SimQL_ReturnCodes::Code::ERROR_UNKNOWN_CONNECTION_STATE,   std::string_view("could not determine if the connection is open")},
        {SimQL_ReturnCodes::Code::ERROR_SET_CURSOR_TYPE,            std::string_view("could not set the cursor type")},
        {SimQL_ReturnCodes::Code::ERROR_SET_QUERY_TIMEOUT,          std::string_view("could not set the query timeout")},
        {SimQL_ReturnCodes::Code::ERROR_SET_MAX_ROWS,               std::string_view("could not set the max rows")},
        {SimQL_ReturnCodes::Code::ERROR_SET_PARAM_BINDING,          std::string_view("could not set the parameter binding")},
        {SimQL_ReturnCodes::Code::ERROR_SET_PARAM_DUPLICATE,        std::string_view("duplicate parameters not permitted")},
        {SimQL_ReturnCodes::Code::ERROR_SET_PARAM_INVALID_DTYPE,    std::string_view("could not bind the parameter to the returned data type")},
        {SimQL_ReturnCodes::Code::ERROR_STMT_PREPARE,               std::string_view("could not prepare the sql statement")},
        {SimQL_ReturnCodes::Code::ERROR_STMT_EXECUTE,               std::string_view("could not execute the sql statement")},
        {SimQL_ReturnCodes::Code::ERROR_STMT_COLUMN_CALC,           std::string_view("could not calculate the result set's column count")},
        {SimQL_ReturnCodes::Code::INFO_STMT_NO_COLUMNS,             std::string_view("there are no columns in the result set")},
        {SimQL_ReturnCodes::Code::ERROR_COL_BINDING,                std::string_view("could not bind the current column")},
        {SimQL_ReturnCodes::Code::ERROR_COL_INVALID_DTYPE,          std::string_view("coudl not bind the column to the returned data type")}
    };
}

namespace SimQL_ReturnCodes {
    static inline std::string_view description(const Code& code) {
        auto it = descriptions.find(code);
        if (it == descriptions.end())
            return std::string_view("could not find the corresponding description");

        return it->second;
    }
}

#endif