#ifndef simql_returncodes_header_h
#define simql_returncodes_header_h

#include <cstdint>
#include <string_view>
#include <unordered_map>

namespace simql_returncodes {
    
    enum class code : std::uint8_t {

        /* generic */
        success,
        success_info,
        null_response,

        /* ENVIRONMENT HANDLE */
        error_alloc_handle,
        error_set_odbc_version3,
        error_set_pooling_type,
        error_set_pool_match_type,

        /* CONNECTION HANDLE */
        error_set_access_mode,
        error_set_connection_timeout,
        error_set_login_timeout,
        error_set_packet_size,
        error_set_async,
        error_set_autocommit,
        error_set_tracing,
        error_set_tracefile,
        error_open_connection,
        error_unknown_connection_state,

        /* STATEMENT HANDLE */
        error_set_cursor_type,
        error_set_query_timeout,
        error_set_max_rows,
        error_set_param_binding,
        error_set_param_duplicate,
        error_set_param_invalid_dtype,
        error_stmt_prepare,
        error_stmt_execute,
        error_stmt_column_calc,
        error_stmt_no_columns,
        error_col_binding,
        error_col_invalid_dtype

    };
    static constexpr code is_nullptr = code::null_response;
}

namespace {
    const std::unordered_map<simql_returncodes::code, std::string_view> descriptions {
        {simql_returncodes::code::success,                          std::string_view("success")},
        {simql_returncodes::code::success_info,                     std::string_view("success but ODBC diagnostics were generated")},
        {simql_returncodes::code::null_response,                          std::string_view("the underlying pointer is null")},
        {simql_returncodes::code::error_alloc_handle,               std::string_view("could not allocate the ODBC handle")},
        {simql_returncodes::code::error_set_odbc_version3,          std::string_view("could not set to ODBC version 3")},
        {simql_returncodes::code::error_set_pooling_type,           std::string_view("could not set the pooling type")},
        {simql_returncodes::code::error_set_pool_match_type,        std::string_view("could not set the pool match type")},
        {simql_returncodes::code::error_set_access_mode,            std::string_view("could not set the access mode")},
        {simql_returncodes::code::error_set_connection_timeout,     std::string_view("could not set the connection timeout")},
        {simql_returncodes::code::error_set_login_timeout,          std::string_view("could not set the login timeout")},
        {simql_returncodes::code::error_set_packet_size,            std::string_view("could not set the packet size")},
        {simql_returncodes::code::error_set_async,                  std::string_view("could not set to asynchronous")},
        {simql_returncodes::code::error_set_autocommit,             std::string_view("could not set the autocommit state")},
        {simql_returncodes::code::error_set_tracing,                std::string_view("could not enable/disable tracing")},
        {simql_returncodes::code::error_set_tracefile,              std::string_view("could not set the tracefile path")},
        {simql_returncodes::code::error_open_connection,            std::string_view("could not open a connection to the database")},
        {simql_returncodes::code::error_unknown_connection_state,   std::string_view("could not determine if the connection is open")},
        {simql_returncodes::code::error_set_cursor_type,            std::string_view("could not set the cursor type")},
        {simql_returncodes::code::error_set_query_timeout,          std::string_view("could not set the query timeout")},
        {simql_returncodes::code::error_set_max_rows,               std::string_view("could not set the max rows")},
        {simql_returncodes::code::error_set_param_binding,          std::string_view("could not set the parameter binding")},
        {simql_returncodes::code::error_set_param_duplicate,        std::string_view("duplicate parameters not permitted")},
        {simql_returncodes::code::error_set_param_invalid_dtype,    std::string_view("could not bind the parameter to the returned data type")},
        {simql_returncodes::code::error_stmt_prepare,               std::string_view("could not prepare the sql statement")},
        {simql_returncodes::code::error_stmt_execute,               std::string_view("could not execute the sql statement")},
        {simql_returncodes::code::error_stmt_column_calc,           std::string_view("could not calculate the result set's column count")},
        {simql_returncodes::code::error_stmt_no_columns,            std::string_view("there are no columns in the result set")},
        {simql_returncodes::code::error_col_binding,                std::string_view("could not bind the current column")},
        {simql_returncodes::code::error_col_invalid_dtype,          std::string_view("coudl not bind the column to the returned data type")}
    };
}

namespace simql_returncodes {
    static inline std::string_view description(const code& code) {
        auto it = descriptions.find(code);
        if (it == descriptions.end())
            return std::string_view("could not find the corresponding description");

        return it->second;
    }
}

#endif