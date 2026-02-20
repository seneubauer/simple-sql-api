// SimQL stuff
#include "statement.hpp"
#include "database_connection.hpp"
#include "simql_strings.hpp"
#include "simql_constants.hpp"
#include "diagnostic_set.hpp"
#include "result_set.hpp"
#include "value_set.hpp"

// STL stuff
#include <cstdint>
#include <memory>
#include <vector>
#include <map>
#include <deque>
#include <variant>
#include <algorithm>
#include <concepts>

// OS stuff
#include "os_inclusions.hpp"

// ODBC stuff
#include <sqltypes.h>
#include <sqlext.h>
#include <sql.h>

namespace simql {

    extern void* get_dbc_handle(database_connection& dbc) noexcept;

    enum class handle_ownership : std::uint8_t {
        owns,
        borrows
    };

    struct statement::handle {

        // handles
        SQLHDBC h_dbc{SQL_NULL_HDBC};
        SQLHSTMT h_stmt{SQL_NULL_HSTMT};
        void* p_pool;

        // diagnostics
        std::string last_error{};
        diagnostic_set diag{};

        // trackers
        bool is_valid{true};
        handle_ownership ownership{handle_ownership::owns};
        SQLUSMALLINT pbind_index{1};
        SQLUINTEGER rows_fetched{0};

        // data members
        result_set results{};
        value_set values{};

        // odbc binding data type
        using BoundValue = std::variant<
            std::basic_string<SQLCHAR>,
            std::basic_string<SQLWCHAR>,
            std::vector<SQLCHAR>,
            SQLDOUBLE,
            SQLREAL,
            SQLCHAR,
            SQLWCHAR,
            SQLSMALLINT,
            SQLINTEGER,
            SQLLEN,
            SQLGUID,
            SQL_TIMESTAMP_STRUCT,
            SQL_DATE_STRUCT,
            SQL_TIME_STRUCT,
            std::vector<std::basic_string<SQLCHAR>>,
            std::vector<std::basic_string<SQLWCHAR>>,
            std::vector<std::vector<SQLCHAR>>,
            std::vector<SQLDOUBLE>,
            std::vector<SQLREAL>,
            std::vector<SQLCHAR>,
            std::vector<SQLWCHAR>,
            std::vector<SQLSMALLINT>,
            std::vector<SQLINTEGER>,
            std::vector<SQLLEN>,
            std::vector<SQLGUID>,
            std::vector<SQL_TIMESTAMP_STRUCT>,
            std::vector<SQL_DATE_STRUCT>,
            std::vector<SQL_TIME_STRUCT>
        >;
        struct Binding {
            BoundValue value;
            SQLLEN indicator;
            SQLSMALLINT c_type{0};
        };
        std::map<std::string, Binding> bound_parameters;

        using bound_array_variant = std::variant<
            std::vector<std::basic_string<SQLCHAR>>,
            std::vector<std::basic_string<SQLWCHAR>>,
            std::vector<std::vector<SQLCHAR>>,
            std::vector<SQLDOUBLE>,
            std::vector<SQLREAL>,
            std::vector<SQLCHAR>,
            std::vector<SQLWCHAR>,
            std::vector<SQLSMALLINT>,
            std::vector<SQLINTEGER>,
            std::vector<SQLLEN>,
            std::vector<SQLGUID>,
            std::vector<SQL_TIMESTAMP_STRUCT>,
            std::vector<SQL_DATE_STRUCT>,
            std::vector<SQL_TIME_STRUCT>
        >;
        struct array_binding {
            bound_array_variant value{};
            std::vector<SQLLEN> indicators{};
        };

        handle() = default;

        explicit handle(database_connection& dbc, const statement::alloc_options& options) {
            ownership = handle_ownership::owns;

            // allocate the handle
            h_dbc = static_cast<SQLHDBC>(get_dbc_handle(dbc));
            switch (SQLAllocHandle(SQL_HANDLE_STMT, h_dbc, &h_stmt)) {
            case SQL_SUCCESS:
                break;
            case SQL_SUCCESS_WITH_INFO:
                diag.update(h_dbc, diagnostic_set::handle_type::dbc, std::string{"SQLAllocHandle(SQL_HANDLE_STMT) -> SUCCESS_WITH_INFO"});
                break;
            case SQL_INVALID_HANDLE:
                last_error = std::string{"could not allocate the statement handle: invalid handle"};
                diag.update(h_dbc, diagnostic_set::handle_type::dbc, std::string{"SQLAllocHandle(SQL_HANDLE_STMT) -> INVALID_HANDLE"});
                is_valid = false;
                return;
            default:
                last_error = std::string{"could not allocate the statement handle: generic error"};
                diag.update(h_dbc, diagnostic_set::handle_type::dbc, std::string{"SQLAllocHandle(SQL_HANDLE_STMT) -> ERROR"});
                is_valid = false;
                return;
            }

            // set cursor type
            is_valid = set_cursor_type(options.cursor);
            if (!is_valid)
                return;

            // set query timeout
            is_valid = set_query_timeout(options.query_timeout);
            if (!is_valid)
                return;

            // set max rows
            is_valid = set_max_rows(options.max_rows);
            if (!is_valid)
                return;

            // set rowset size
            is_valid = set_rowset_size(options.rowset_size);
            if (!is_valid)
                return;

        }

        handle(void* stmt_handle, database_connection& conn, void* pool) noexcept {
            h_dbc = static_cast<SQLHDBC>(get_dbc_handle(conn));
            h_stmt = static_cast<SQLHSTMT>(stmt_handle);
            ownership = handle_ownership::borrows;
            p_pool = pool;
        }

        ~handle() {
            if (h_stmt) {
                switch (ownership) {
                case handle_ownership::owns:
                    SQLFreeHandle(SQL_HANDLE_STMT, h_stmt);
                    break;
                case handle_ownership::borrows:
                    SQLFreeStmt(h_stmt, SQL_CLOSE);
                    SQLFreeStmt(h_stmt, SQL_RESET_PARAMS);
                    SQLFreeStmt(h_stmt, SQL_UNBIND);
                    break;
                }
                h_stmt = SQL_NULL_HSTMT;
            }
        }

        bool set_cursor_type(statement::cursor_type cursor) {
            SQLPOINTER p_cursor_type;
            switch (cursor) {
            case cursor_type::forward_only:
                p_cursor_type = reinterpret_cast<SQLPOINTER>(SQL_CURSOR_FORWARD_ONLY);
                break;
            case cursor_type::static_cursor:
                p_cursor_type = reinterpret_cast<SQLPOINTER>(SQL_CURSOR_STATIC);
                break;
            case cursor_type::dyanmic_cursor:
                p_cursor_type = reinterpret_cast<SQLPOINTER>(SQL_CURSOR_DYNAMIC);
                break;
            case cursor_type::keyset_driven:
                p_cursor_type = reinterpret_cast<SQLPOINTER>(SQL_CURSOR_KEYSET_DRIVEN);
                break;
            }

            switch (SQLSetStmtAttrW(h_stmt, SQL_ATTR_CURSOR_TYPE, p_cursor_type, 0)) {
            case SQL_SUCCESS:
                return true;
            case SQL_SUCCESS_WITH_INFO:
                diag.update(h_stmt, diagnostic_set::handle_type::stmt, std::string{"SQLSetStmtAttr(SQL_ATTR_CURSOR_TYPE) -> SUCCESS_WITH_INFO"});
                return true;
            case SQL_INVALID_HANDLE:
                last_error = std::string{"could not set the cursor type: invalid handle"};
                diag.update(h_stmt, diagnostic_set::handle_type::stmt, std::string{"SQLSetStmtAttr(SQL_ATTR_CURSOR_TYPE) -> INVALID_HANDLE"});
                return false;
            default:
                last_error = std::string{"could not set the cursor type: generic error"};
                diag.update(h_stmt, diagnostic_set::handle_type::stmt, std::string{"SQLSetStmtAttr(SQL_ATTR_CURSOR_TYPE) -> ERROR"});
                return false;
            }
        }

        bool set_query_timeout(std::uint32_t timeout) {
            SQLPOINTER p_query_timeout = reinterpret_cast<SQLPOINTER>(static_cast<SQLULEN>(timeout));
            switch (SQLSetStmtAttrW(h_stmt, SQL_ATTR_QUERY_TIMEOUT, p_query_timeout, 0)) {
            case SQL_SUCCESS:
                return true;
            case SQL_SUCCESS_WITH_INFO:
                diag.update(h_stmt, diagnostic_set::handle_type::stmt, std::string{"SQLSetStmtAttr(SQL_ATTR_QUERY_TIMEOUT) -> SUCCESS_WITH_INFO"});
                return true;
            case SQL_INVALID_HANDLE:
                last_error = std::string{"could not set the query timeout: invalid handle"};
                diag.update(h_stmt, diagnostic_set::handle_type::stmt, std::string{"SQLSetStmtAttr(SQL_ATTR_QUERY_TIMEOUT) -> INVALID_HANDLE"});
                return false;
            default:
                last_error = std::string{"could not set the query timeout: generic error"};
                diag.update(h_stmt, diagnostic_set::handle_type::stmt, std::string{"SQLSetStmtAttr(SQL_ATTR_QUERY_TIMEOUT) -> ERROR"});
                return false;
            }
        }

        bool set_max_rows(std::uint64_t max_rows) {
            SQLPOINTER p_max_rows = reinterpret_cast<SQLPOINTER>(static_cast<SQLULEN>(max_rows));
            switch (SQLSetStmtAttrW(h_stmt, SQL_ATTR_MAX_ROWS, p_max_rows, 0)) {
            case SQL_SUCCESS:
                return true;
            case SQL_SUCCESS_WITH_INFO:
                diag.update(h_stmt, diagnostic_set::handle_type::stmt, std::string{"SQLSetStmtAttr(SQL_ATTR_MAX_ROWS) -> SUCCESS_WITH_INFO"});
                return true;
            case SQL_INVALID_HANDLE:
                last_error = std::string{"could not set the max rows: invalid handle"};
                diag.update(h_stmt, diagnostic_set::handle_type::stmt, std::string{"SQLSetStmtAttr(SQL_ATTR_MAX_ROWS) -> INVALID_HANDLE"});
                return false;
            default:
                last_error = std::string{"could not set the max rows: generic error"};
                diag.update(h_stmt, diagnostic_set::handle_type::stmt, std::string{"SQLSetStmtAttr(SQL_ATTR_MAX_ROWS) -> ERROR"});
                return false;
            }
        }

        bool set_rowset_size(std::uint32_t rowset_size) {
            SQLPOINTER p_rowset_size = reinterpret_cast<SQLPOINTER>(static_cast<SQLUINTEGER>(rowset_size));
            switch (SQLSetStmtAttrW(h_stmt, SQL_ATTR_ROW_ARRAY_SIZE, p_rowset_size, 0)) {
            case SQL_SUCCESS:
                return true;
            case SQL_SUCCESS_WITH_INFO:
                return true;
            case SQL_INVALID_HANDLE:
                last_error = std::string{"could not set the rowset size: invalid handle"};
                diag.update(h_stmt, diagnostic_set::handle_type::stmt, std::string{"SQLSetStmtAttr(SQL_ATTR_ROW_ARRAY_SIZE) -> INVALID_HANDLE"});
                return false;
            default:
                last_error = std::string{"could not set the rowset size: generic error"};
                diag.update(h_stmt, diagnostic_set::handle_type::stmt, std::string{"SQLSetStmtAttr(SQL_ATTR_ROW_ARRAY_SIZE) -> ERROR"});
                return false;
            }
        }

        SQLUINTEGER get_rowset_size() {
            SQLUINTEGER rowset_size{};
            switch (SQLGetStmtAttrW(h_stmt, SQL_ATTR_ROW_ARRAY_SIZE, &rowset_size, 0, nullptr)) {
            case SQL_SUCCESS:
                return rowset_size;
            case SQL_SUCCESS_WITH_INFO:
                diag.update(h_stmt, diagnostic_set::handle_type::stmt, std::string{"SQLGetStmtAttr(SQL_ATTR_ROW_ARRAY_SIZE) -> SUCCESS_WITH_INFO"});
                return rowset_size;
            case SQL_INVALID_HANDLE:
                last_error = std::string{"could not retrieve the SQL_ATTR_ROW_ARRAY_SIZE attribute: invalid handle"};
                diag.update(h_stmt, diagnostic_set::handle_type::stmt, std::string{"SQLGetStmtAttr(SQL_ATTR_ROW_ARRAY_SIZE) -> INVALID_HANDLE"});
                return 0;
            default:
                last_error = std::string{"could not retrieve the SQL_ATTR_ROW_ARRAY_SIZE attribute: generic error"};
                diag.update(h_stmt, diagnostic_set::handle_type::stmt, std::string{"SQLGetStmtAttr(SQL_ATTR_ROW_ARRAY_SIZE) -> ERROR"});
                return 0;
            }
        }

        void reset() {
            SQLCloseCursor(h_stmt);
            SQLFreeStmt(h_stmt, SQL_RESET_PARAMS);
            SQLFreeStmt(h_stmt, SQL_UNBIND);
        }

        SQLSMALLINT param_type(const simql_types::parameter_binding_type& binding_type) {
            switch (binding_type) {
            case simql_types::parameter_binding_type::input_output:
                return SQL_PARAM_INPUT_OUTPUT;
            case simql_types::parameter_binding_type::input:
                return SQL_PARAM_INPUT;
            case simql_types::parameter_binding_type::output:
                return SQL_PARAM_OUTPUT;
            default:
                return SQL_PARAM_INPUT;
            }
        }

        bool bindparam_string(const std::string& name, const std::string& value, const simql_types::parameter_binding_type& binding_type, const bool& set_null) {
            auto it = bound_parameters.find(name);
            if (it != bound_parameters.end()) {
                last_error = std::string{"cannot bind duplicate parameters"};
                return false;
            }

            SQLSMALLINT sql_type{};
            SQLULEN definition{};
            SQLSMALLINT scale{0};
            SQLSMALLINT nullable{0};
            SQLLEN buffer_length{};

            if (!SQL_SUCCEEDED(SQLDescribeParam(h_stmt, pbind_index, &sql_type, &definition, &scale, &nullable))) {
                sql_type = SQL_WVARCHAR;
                definition = static_cast<SQLULEN>(value.size() + 1);
            }

            SQLPOINTER p_val;
            SQLSMALLINT c_type;
            if (sql_type == SQL_VARCHAR || sql_type == SQL_CHAR) {
                c_type = SQL_C_CHAR;
                buffer_length = definition * sizeof(SQLCHAR);
                bound_parameters.emplace(name, Binding { BoundValue(simql_strings::to_odbc_n(value)), 0 });
                auto& param = bound_parameters.at(name);
                auto& val = std::get<std::basic_string<SQLCHAR>>(param.value);
                p_val = reinterpret_cast<SQLPOINTER>(val.data());
            } else if (sql_type == SQL_WVARCHAR || sql_type == SQL_WCHAR) {
                c_type = SQL_C_WCHAR;
                buffer_length = definition * sizeof(SQLWCHAR);
                bound_parameters.emplace(name, Binding { BoundValue(simql_strings::to_odbc_w(value)), 0 });
                auto& param = bound_parameters.at(name);
                auto& val = std::get<std::basic_string<SQLWCHAR>>(param.value);
                p_val = reinterpret_cast<SQLPOINTER>(val.data());
            } else {
                last_error = std::string{"the SQL data type is not mapped"};
                return false;
            }

            switch (binding_type) {
            case simql_types::parameter_binding_type::input_output:
                bound_parameters.at(name).indicator = set_null ? SQL_NULL_DATA : SQL_NTS;
                break;
            case simql_types::parameter_binding_type::input:
                bound_parameters.at(name).indicator = set_null ? SQL_NULL_DATA : SQL_NTS;
                break;
            case simql_types::parameter_binding_type::output:
                bound_parameters.at(name).indicator = 0;
                break;
            }

            switch (SQLBindParameter(h_stmt, pbind_index, param_type(binding_type), c_type, sql_type, definition, scale, p_val, buffer_length, &bound_parameters.at(name).indicator)) {
            case SQL_SUCCESS:
                pbind_index++;
                return true;
            case SQL_SUCCESS_WITH_INFO:
                pbind_index++;
                diag.update(h_stmt, diagnostic_set::handle_type::stmt, std::string{"SQLBindParameter() -> STRING -> SUCCESS_WITH_INFO"});
                return true;
            default:
                last_error = std::string{"could not bind the string parameter"};
                diag.update(h_stmt, diagnostic_set::handle_type::stmt, std::string{"SQLBindParameter() -> STRING -> ERROR"});
                return false;
            }
        }

        bool bindparam_floating_point(const std::string& name, const double& value, const simql_types::parameter_binding_type& binding_type, const bool& set_null) {
            auto it = bound_parameters.find(name);
            if (it != bound_parameters.end()) {
                last_error = std::string{"cannot bind duplicate parameters"};
                return false;
            }

            SQLSMALLINT sql_type{};
            SQLULEN definition{};
            SQLSMALLINT scale{0};
            SQLSMALLINT nullable{0};
            SQLLEN buffer_length{};

            if (!SQL_SUCCEEDED(SQLDescribeParam(h_stmt, pbind_index, &sql_type, &definition, &scale, &nullable))) {
                sql_type = SQL_DOUBLE;
            }

            SQLPOINTER p_val;
            SQLSMALLINT c_type;
            if (sql_type == SQL_DOUBLE || sql_type == SQL_FLOAT) {
                sql_type = SQL_DOUBLE;
                c_type = SQL_C_DOUBLE;
                buffer_length = sizeof(SQLDOUBLE);
                bound_parameters.emplace(name, Binding { BoundValue(value), 0 });
                auto& param = bound_parameters.at(name);
                auto& val = std::get<SQLDOUBLE>(param.value);
                p_val = reinterpret_cast<SQLPOINTER>(&val);
            } else if (sql_type == SQL_REAL) {
                c_type = SQL_C_FLOAT;
                buffer_length = sizeof(SQLREAL);
                bound_parameters.emplace(name, Binding { BoundValue(value), 0 });
                auto& param = bound_parameters.at(name);
                auto& val = std::get<SQLREAL>(param.value);
                p_val = reinterpret_cast<SQLPOINTER>(&val);
            } else {
                last_error = std::string{"the SQL data type is not mapped"};
                return false;
            }
            definition = buffer_length * 2;

            switch (binding_type) {
            case simql_types::parameter_binding_type::input_output:
                bound_parameters.at(name).indicator = set_null ? SQL_NULL_DATA : 0;
                break;
            case simql_types::parameter_binding_type::input:
                bound_parameters.at(name).indicator = set_null ? SQL_NULL_DATA : 0;
                break;
            case simql_types::parameter_binding_type::output:
                bound_parameters.at(name).indicator = 0;
                break;
            }

            switch (SQLBindParameter(h_stmt, pbind_index, param_type(binding_type), c_type, sql_type, definition, scale, p_val, buffer_length, &bound_parameters.at(name).indicator)) {
            case SQL_SUCCESS:
                pbind_index++;
                return true;
            case SQL_SUCCESS_WITH_INFO:
                pbind_index++;
                diag.update(h_stmt, diagnostic_set::handle_type::stmt, std::string{"SQLBindParameter() -> FLOATING_POINT -> SUCCESS_WITH_INFO"});
                return true;
            default:
                last_error = std::string{"could not bind the floating point parameter"};
                diag.update(h_stmt, diagnostic_set::handle_type::stmt, std::string{"SQLBindParameter() -> FLOATING_POINT -> ERROR"});
                return false;
            }
        }

        bool bindparam_boolean(const std::string& name, const bool& value, const simql_types::parameter_binding_type& binding_type, const bool& set_null) {
            auto it = bound_parameters.find(name);
            if (it != bound_parameters.end()) {
                last_error = std::string{"cannot bind duplicate parameters"};
                return false;
            }

            SQLSMALLINT sql_type{};
            SQLULEN definition{};
            SQLSMALLINT scale{};
            SQLSMALLINT nullable{};
            SQLLEN buffer_length{0};

            if (!SQL_SUCCEEDED(SQLDescribeParam(h_stmt, pbind_index, &sql_type, &definition, &scale, &nullable))) {
                sql_type = SQL_BIT;
                definition = 1;
            }

            SQLPOINTER p_val;
            SQLSMALLINT c_type;
            if (sql_type == SQL_BIT) {
                c_type = SQL_C_BIT;
                bound_parameters.emplace(name, Binding { BoundValue(static_cast<SQLCHAR>(value)), 0 });
                auto& param = bound_parameters.at(name);
                auto& val = std::get<SQLCHAR>(param.value);
                p_val = reinterpret_cast<SQLPOINTER>(&val);
            } else {
                last_error = std::string{"the SQL data type is not mapped"};
                return false;
            }

            switch (binding_type) {
            case simql_types::parameter_binding_type::input_output:
                bound_parameters.at(name).indicator = set_null ? SQL_NULL_DATA : 0;
                break;
            case simql_types::parameter_binding_type::input:
                bound_parameters.at(name).indicator = set_null ? SQL_NULL_DATA : 0;
                break;
            case simql_types::parameter_binding_type::output:
                bound_parameters.at(name).indicator = 0;
                break;
            }

            switch (SQLBindParameter(h_stmt, pbind_index, param_type(binding_type), c_type, sql_type, definition, scale, p_val, buffer_length, &bound_parameters.at(name).indicator)) {
            case SQL_SUCCESS:
                pbind_index++;
                return true;
            case SQL_SUCCESS_WITH_INFO:
                pbind_index++;
                diag.update(h_stmt, diagnostic_set::handle_type::stmt, std::string{"SQLBindParameter() -> BOOLEAN -> SUCCESS_WITH_INFO"});
                return true;
            default:
                last_error = std::string{"could not bind the boolean parameter"};
                diag.update(h_stmt, diagnostic_set::handle_type::stmt, std::string{"SQLBindParameter() -> BOOLEAN -> ERROR"});
                return false;
            }
        }

        bool bindparam_integer(const std::string& name, const int& value, const simql_types::parameter_binding_type& binding_type, const bool& set_null) {
            auto it = bound_parameters.find(name);
            if (it != bound_parameters.end()) {
                last_error = std::string{"cannot bind duplicate parameters"};
                return false;
            }

            SQLSMALLINT sql_type{};
            SQLULEN definition{0};
            SQLSMALLINT scale{0};
            SQLSMALLINT nullable{0};
            SQLLEN buffer_length{0};

            if (!SQL_SUCCEEDED(SQLDescribeParam(h_stmt, pbind_index, &sql_type, &definition, &scale, &nullable))) {
                sql_type = SQL_INTEGER;
            }

            SQLPOINTER p_val;
            SQLSMALLINT c_type;
            if (sql_type == SQL_TINYINT) {
                c_type = SQL_C_STINYINT;
                bound_parameters.emplace(name, Binding { BoundValue(static_cast<SQLCHAR>(value)), 0 });
                auto& param = bound_parameters.at(name);
                auto& val = std::get<SQLCHAR>(param.value);
                p_val = reinterpret_cast<SQLPOINTER>(&val);
            } else if (sql_type == SQL_SMALLINT) {
                c_type = SQL_C_SSHORT;
                bound_parameters.emplace(name, Binding { BoundValue(static_cast<SQLSMALLINT>(value)), 0 });
                auto& param = bound_parameters.at(name);
                auto& val = std::get<SQLSMALLINT>(param.value);
                p_val = reinterpret_cast<SQLPOINTER>(&val);
            } else if (sql_type == SQL_INTEGER) {
                c_type = SQL_C_SLONG;
                bound_parameters.emplace(name, Binding { BoundValue(static_cast<SQLINTEGER>(value)), 0 });
                auto& param = bound_parameters.at(name);
                auto& val = std::get<SQLINTEGER>(param.value);
                p_val = reinterpret_cast<SQLPOINTER>(&val);
            } else if (sql_type == SQL_BIGINT) {
                c_type = SQL_C_SBIGINT;
                bound_parameters.emplace(name, Binding { BoundValue(static_cast<SQLBIGINT>(value)), 0 });
                auto& param = bound_parameters.at(name);
                auto& val = std::get<SQLBIGINT>(param.value);
                p_val = reinterpret_cast<SQLPOINTER>(&val);
            } else {
                last_error = std::string{"the SQL data type is not mapped"};
                return false;
            }

            switch (binding_type) {
            case simql_types::parameter_binding_type::input_output:
                bound_parameters.at(name).indicator = set_null ? SQL_NULL_DATA : 0;
                break;
            case simql_types::parameter_binding_type::input:
                bound_parameters.at(name).indicator = set_null ? SQL_NULL_DATA : 0;
                break;
            case simql_types::parameter_binding_type::output:
                bound_parameters.at(name).indicator = 0;
                break;
            }

            switch (SQLBindParameter(h_stmt, pbind_index, param_type(binding_type), c_type, sql_type, definition, scale, p_val, buffer_length, &bound_parameters.at(name).indicator)) {
            case SQL_SUCCESS:
                pbind_index++;
                return true;
            case SQL_SUCCESS_WITH_INFO:
                pbind_index++;
                diag.update(h_stmt, diagnostic_set::handle_type::stmt, std::string{"SQLBindParameter() -> INTEGER -> SUCCESS_WITH_INFO"});
                return true;
            default:
                last_error = std::string{"could not bind the integer parameter"};
                diag.update(h_stmt, diagnostic_set::handle_type::stmt, std::string{"SQLBindParameter() -> INTEGER -> ERROR"});
                return false;
            }
        }

        bool bindparam_guid(const std::string& name, const simql_types::guid_struct& value, const simql_types::parameter_binding_type& binding_type, const bool& set_null) {
            auto it = bound_parameters.find(name);
            if (it != bound_parameters.end()) {
                last_error = std::string{"cannot bind duplicate parameters"};
                return false;
            }

            SQLSMALLINT sql_type{};
            SQLULEN definition{};
            SQLSMALLINT scale{0};
            SQLSMALLINT nullable{0};
            SQLLEN buffer_length{};

            if (!SQL_SUCCEEDED(SQLDescribeParam(h_stmt, pbind_index, &sql_type, &definition, &scale, &nullable))) {
                sql_type = SQL_GUID;
            }

            SQLGUID guid;
            guid.Data1 = value.time_low;
            guid.Data2 = value.time_mid;
            guid.Data3 = value.time_high;
            std::copy(std::begin(value.clock_seq_node), std::end(value.clock_seq_node), std::begin(guid.Data4));

            SQLPOINTER p_val;
            SQLSMALLINT c_type;
            if (sql_type == SQL_GUID) {
                c_type = SQL_C_GUID;
                definition = sizeof(SQLGUID);
                buffer_length = sizeof(SQLGUID);
                bound_parameters.emplace(name, Binding { BoundValue(guid), 0 });
                auto& param = bound_parameters.at(name);
                auto& val = std::get<SQLGUID>(param.value);
                p_val = reinterpret_cast<SQLPOINTER>(&val);
            } else {
                last_error = std::string{"the SQL data type is not mapped"};
                return false;
            }

            switch (binding_type) {
            case simql_types::parameter_binding_type::input_output:
                bound_parameters.at(name).indicator = set_null ? SQL_NULL_DATA : 0;
                break;
            case simql_types::parameter_binding_type::input:
                bound_parameters.at(name).indicator = set_null ? SQL_NULL_DATA : 0;
                break;
            case simql_types::parameter_binding_type::output:
                bound_parameters.at(name).indicator = 0;
                break;
            }

            switch (SQLBindParameter(h_stmt, pbind_index, param_type(binding_type), c_type, sql_type, definition, scale, p_val, buffer_length, &bound_parameters.at(name).indicator)) {
            case SQL_SUCCESS:
                pbind_index++;
                return true;
            case SQL_SUCCESS_WITH_INFO:
                pbind_index++;
                diag.update(h_stmt, diagnostic_set::handle_type::stmt, std::string{"SQLBindParameter() -> GUID -> SUCCESS_WITH_INFO"});
                return true;
            default:
                last_error = std::string{"could not bind the GUID parameter"};
                diag.update(h_stmt, diagnostic_set::handle_type::stmt, std::string{"SQLBindParameter() -> GUID -> ERROR"});
                return false;
            }
        }

        bool bindparam_datetime(const std::string& name, const simql_types::datetime_struct& value, const simql_types::parameter_binding_type& binding_type, const bool& set_null) {
            auto it = bound_parameters.find(name);
            if (it != bound_parameters.end()) {
                last_error = std::string{"cannot bind duplicate parameters"};
                return false;
            }

            SQLSMALLINT sql_type{SQL_TYPE_TIMESTAMP};
            SQLSMALLINT c_type{SQL_C_TYPE_TIMESTAMP};
            SQLULEN definition{29};
            SQLSMALLINT scale{9};
            SQLLEN buffer_length{0};

            SQL_TIMESTAMP_STRUCT x;
            x.year = value.year;
            x.month = value.month;
            x.day = value.day;
            x.hour = value.hour;
            x.minute = value.minute;
            x.second = value.second;
            x.fraction = value.fraction;

            bound_parameters.emplace(name, Binding { BoundValue(x), 0 });
            auto& param = bound_parameters.at(name);
            auto& val = std::get<SQL_TIMESTAMP_STRUCT>(param.value);
            SQLPOINTER p_val = reinterpret_cast<SQLPOINTER>(&val);

            switch (binding_type) {
            case simql_types::parameter_binding_type::input_output:
                bound_parameters.at(name).indicator = set_null ? SQL_NULL_DATA : 0;
                break;
            case simql_types::parameter_binding_type::input:
                bound_parameters.at(name).indicator = set_null ? SQL_NULL_DATA : 0;
                break;
            case simql_types::parameter_binding_type::output:
                bound_parameters.at(name).indicator = 0;
                break;
            }

            switch (SQLBindParameter(h_stmt, pbind_index, param_type(binding_type), c_type, sql_type, definition, scale, p_val, buffer_length, &bound_parameters.at(name).indicator)) {
            case SQL_SUCCESS:
                pbind_index++;
                return true;
            case SQL_SUCCESS_WITH_INFO:
                pbind_index++;
                diag.update(h_stmt, diagnostic_set::handle_type::stmt, std::string{"SQLBindParameter() -> DATETIME -> SUCCESS_WITH_INFO"});
                return true;
            default:
                last_error = std::string{"could not bind the datetime parameter"};
                diag.update(h_stmt, diagnostic_set::handle_type::stmt, std::string{"SQLBindParameter() -> DATETIME -> ERROR"});
                return false;
            }
        }

        bool bindparam_date(const std::string& name, const simql_types::date_struct& value, const simql_types::parameter_binding_type& binding_type, const bool& set_null) {
            auto it = bound_parameters.find(name);
            if (it != bound_parameters.end()) {
                last_error = std::string{"cannot bind duplicate parameters"};
                return false;
            }

            SQLSMALLINT sql_type{SQL_TYPE_DATE};
            SQLSMALLINT c_type{SQL_C_TYPE_DATE};
            SQLULEN definition{10};
            SQLSMALLINT scale{0};
            SQLLEN buffer_length{0};

            SQL_DATE_STRUCT x;
            x.year = value.year;
            x.month = value.month;
            x.day = value.day;

            bound_parameters.emplace(name, Binding { BoundValue(x), 0 });
            auto& param = bound_parameters.at(name);
            auto& val = std::get<SQL_DATE_STRUCT>(param.value);
            SQLPOINTER p_val = reinterpret_cast<SQLPOINTER>(&val);

            switch (binding_type) {
            case simql_types::parameter_binding_type::input_output:
                bound_parameters.at(name).indicator = set_null ? SQL_NULL_DATA : 0;
                break;
            case simql_types::parameter_binding_type::input:
                bound_parameters.at(name).indicator = set_null ? SQL_NULL_DATA : 0;
                break;
            case simql_types::parameter_binding_type::output:
                bound_parameters.at(name).indicator = 0;
                break;
            }

            switch (SQLBindParameter(h_stmt, pbind_index, param_type(binding_type), c_type, sql_type, definition, scale, p_val, buffer_length, &bound_parameters.at(name).indicator)) {
            case SQL_SUCCESS:
                pbind_index++;
                return true;
            case SQL_SUCCESS_WITH_INFO:
                pbind_index++;
                diag.update(h_stmt, diagnostic_set::handle_type::stmt, std::string{"SQLBindParameter() -> DATE -> SUCCESS_WITH_INFO"});
                return true;
            default:
                last_error = std::string{"could not bind the date parameter"};
                diag.update(h_stmt, diagnostic_set::handle_type::stmt, std::string{"SQLBindParameter() -> DATE -> ERROR"});
                return false;
            }
        }

        bool bindparam_time(const std::string& name, const simql_types::time_struct& value, const simql_types::parameter_binding_type& binding_type, const bool& set_null) {
            auto it = bound_parameters.find(name);
            if (it != bound_parameters.end()) {
                last_error = std::string{"cannot bind duplicate parameters"};
                return false;
            }

            SQLSMALLINT sql_type{SQL_TYPE_TIME};
            SQLSMALLINT c_type{SQL_C_TYPE_TIME};
            SQLULEN definition{10};
            SQLSMALLINT scale{0};
            SQLLEN buffer_length{0};

            SQL_TIME_STRUCT x;
            x.hour = value.hour;
            x.minute = value.minute;
            x.second = value.second;

            bound_parameters.emplace(name, Binding { BoundValue(x), 0 });
            auto& param = bound_parameters.at(name);
            auto& val = std::get<SQL_TIME_STRUCT>(param.value);
            SQLPOINTER p_val = reinterpret_cast<SQLPOINTER>(&val);

            switch (binding_type) {
            case simql_types::parameter_binding_type::input_output:
                bound_parameters.at(name).indicator = set_null ? SQL_NULL_DATA : 0;
                break;
            case simql_types::parameter_binding_type::input:
                bound_parameters.at(name).indicator = set_null ? SQL_NULL_DATA : 0;
                break;
            case simql_types::parameter_binding_type::output:
                bound_parameters.at(name).indicator = 0;
                break;
            }

            switch (SQLBindParameter(h_stmt, pbind_index, param_type(binding_type), c_type, sql_type, definition, scale, p_val, buffer_length, &bound_parameters.at(name).indicator)) {
            case SQL_SUCCESS:
                pbind_index++;
                return true;
            case SQL_SUCCESS_WITH_INFO:
                pbind_index++;
                diag.update(h_stmt, diagnostic_set::handle_type::stmt, std::string{"SQLBindParameter() -> TIME -> SUCCESS_WITH_INFO"});
                return true;
            default:
                last_error = std::string{"could not bind the time parameter"};
                diag.update(h_stmt, diagnostic_set::handle_type::stmt, std::string{"SQLBindParameter() -> TIME -> ERROR"});
                return false;
            }
        }

        bool bindparam_blob(const std::string& name, const std::vector<std::uint8_t>& value, const simql_types::parameter_binding_type& binding_type, const bool& set_null) {
            auto it = bound_parameters.find(name);
            if (it != bound_parameters.end()) {
                last_error = std::string{"cannot bind duplicate parameters"};
                return false;
            }

            SQLSMALLINT sql_type{};
            SQLULEN definition{};
            SQLSMALLINT scale{0};
            SQLSMALLINT nullable{0};
            SQLLEN buffer_length{static_cast<SQLLEN>(value.size())};
            SQLSMALLINT c_type{SQL_C_BINARY};

            if (!SQL_SUCCEEDED(SQLDescribeParam(h_stmt, pbind_index, &sql_type, &definition, &scale, &nullable))) {
                definition = static_cast<SQLULEN>(value.size());
                sql_type = SQL_VARBINARY;
            }

            if (sql_type != SQL_VARBINARY && sql_type != SQL_BINARY) {
                last_error = std::string{"invalid SQL data type"};
                return false;
            }

            bound_parameters.emplace(name, Binding { BoundValue(value), 0 });
            auto& param = bound_parameters.at(name);
            auto& val = std::get<std::vector<SQLCHAR>>(param.value);
            SQLPOINTER p_val = reinterpret_cast<SQLPOINTER>(val.data());

            switch (binding_type) {
            case simql_types::parameter_binding_type::input_output:
                bound_parameters.at(name).indicator = set_null ? SQL_NULL_DATA : buffer_length;
                break;
            case simql_types::parameter_binding_type::input:
                bound_parameters.at(name).indicator = set_null ? SQL_NULL_DATA : buffer_length;
                break;
            case simql_types::parameter_binding_type::output:
                bound_parameters.at(name).indicator = 0;
                break;
            }

            switch (SQLBindParameter(h_stmt, pbind_index, param_type(binding_type), c_type, sql_type, definition, scale, p_val, buffer_length, &bound_parameters.at(name).indicator)) {
            case SQL_SUCCESS:
                pbind_index++;
                return true;
            case SQL_SUCCESS_WITH_INFO:
                pbind_index++;
                diag.update(h_stmt, diagnostic_set::handle_type::stmt, std::string{"SQLBindParameter() -> BLOB -> SUCCESS_WITH_INFO"});
                return true;
            default:
                last_error = std::string{"could not bind the blob parameter"};
                diag.update(h_stmt, diagnostic_set::handle_type::stmt, std::string{"SQLBindParameter() -> BLOB -> ERROR"});
                return false;
            }
        }

        simql_types::null_rule_type null_rule(const SQLSMALLINT& null_id) {
            switch (null_id) {
            case SQL_NO_NULLS:
                return simql_types::null_rule_type::not_allowed;
            case SQL_NULLABLE:
                return simql_types::null_rule_type::allowed;
            case SQL_NULLABLE_UNKNOWN:
                return simql_types::null_rule_type::unknown;
            }
        }

        SQLSMALLINT get_column_count() {
            SQLSMALLINT column_count{};
            if (SQL_SUCCEEDED(SQLNumResultCols(h_stmt, &column_count)))
                return column_count;

            return -1;
        }


        /*
        c_type: should be limited to SQL_C_CHAR, SQL_C_WCHAR, or SQL_C_BINARY
        T: should be SQLCHAR or SQLWCHAR
        */
        template<typename T>
        bool bindcol_variable_size(const SQLSMALLINT& index, const SQLUINTEGER& rowset_size, const SQLSMALLINT& c_type, const SQLULEN& column_size, const bool& null_terminated, std::deque<array_binding>& column_bindings) {

            SQLLEN buffer_length{(column_size + 1) * sizeof(T)};
            column_bindings.emplace_back(
                array_binding {
                    null_terminated ? std::vector<std::basic_string<T>>(std::basic_string<T>(buffer_length), rowset_size) : std::vector<std::vector<T>>(std::vector<T>(buffer_length), rowset_size),
                    std::vector<SQLLEN>(0, rowset_size)
                }
            );

            auto& val = null_terminated ? std::get<std::vector<std::basic_string<T>>>(column_bindings.back().value) : std::get<std::vector<std::vector<T>>>(column_bindings.back().value);
            auto& ind = column_bindings.back().indicators;
            switch (SQLBindCol(h_stmt, index, c_type, val.data()[0], buffer_length, ind.data()[0])) {
            case SQL_SUCCESS:
                return true;
            case SQL_SUCCESS_WITH_INFO:
                diag.update(h_stmt, diagnostic_set::handle_type::stmt, std::string{"SQLBindCol() -> VARIABLE_SIZE -> SUCCESS_WITH_INFO"});
                return true;
            case SQL_INVALID_HANDLE:
                last_error = std::string{"could not bind the a variable sized data rowset to the column: invalid handle"};
                diag.update(h_stmt, diagnostic_set::handle_type::stmt, std::string{"SQLBindCol() -> VARIABLE_SIZE -> INVALID_HANDLE"});
                return false;
            default:
                last_error = std::string{"could not bind the a variable sized data rowset to the column: generic error"};
                diag.update(h_stmt, diagnostic_set::handle_type::stmt, std::string{"SQLBindCol() -> VARIABLE_SIZE -> ERROR"});
                return false;
            }
        }

        bool bindcol_string(const SQLSMALLINT& index, const SQLULEN& column_size, Binding& binding) {
            SQLPOINTER p_val;
            SQLLEN buffer_length;
            if (binding.c_type == SQL_C_CHAR) {
                buffer_length = (column_size + 1) * sizeof(SQLCHAR);
                auto& val = std::get<std::basic_string<SQLCHAR>>(binding.value);
                val.resize(buffer_length);
                p_val = reinterpret_cast<SQLPOINTER>(val.data());
            } else if (binding.c_type == SQL_C_WCHAR) {
                buffer_length = (column_size + 1) * sizeof(SQLWCHAR);
                auto& val = std::get<std::basic_string<SQLWCHAR>>(binding.value);
                val.resize(buffer_length);
                p_val = reinterpret_cast<SQLPOINTER>(val.data());
            } else {
                last_error = std::string{"invalid C data type for the string column"};
                return false;
            }

            switch (SQLBindCol(h_stmt, index, binding.c_type, p_val, buffer_length, &binding.indicator)) {
            case SQL_SUCCESS:
                return true;
            case SQL_SUCCESS_WITH_INFO:
                diag.update(h_stmt, diagnostic_set::handle_type::stmt, std::string{"SQLBindCol() -> STRING -> SUCCESS_WITH_INFO"});
                return true;
            default:
                last_error = std::string{"could not bind a string value to the provided column"};
                diag.update(h_stmt, diagnostic_set::handle_type::stmt, std::string{"SQLBindCol() -> STRING -> ERROR"});
                return false;
            }
        }

        bool bindcol_floating_point(const SQLSMALLINT& index, Binding& binding) {
            SQLPOINTER p_val;
            SQLLEN buffer_length;
            if (binding.c_type == SQL_C_DOUBLE) {
                auto& val = std::get<SQLDOUBLE>(binding.value);
                p_val = reinterpret_cast<SQLPOINTER>(&val);
                buffer_length = sizeof(SQLDOUBLE);
            } else if (binding.c_type == SQL_C_FLOAT) {
                auto& val = std::get<SQLREAL>(binding.value);
                p_val = reinterpret_cast<SQLPOINTER>(&val);
                buffer_length = sizeof(SQLREAL);
            } else {
                last_error = std::string{"invalid C data type for the floating point column"};
                return false;
            }

            switch (SQLBindCol(h_stmt, index, binding.c_type, p_val, buffer_length, &binding.indicator)) {
            case SQL_SUCCESS:
                return true;
            case SQL_SUCCESS_WITH_INFO:
                diag.update(h_stmt, diagnostic_set::handle_type::stmt, std::string{"SQLBindCol() -> FLOATING_POINT -> SUCCESS_WITH_INFO"});
                return true;
            default:
                last_error = std::string{"could not bind a floating point value to the provided column"};
                diag.update(h_stmt, diagnostic_set::handle_type::stmt, std::string{"SQLBindCol() -> FLOATING_POINT -> ERROR"});
                return false;
            }
        }

        template<typename T>
        bool bindcol_floating_point(const SQLSMALLINT& index, const SQLUINTEGER& rowset_size, const SQLSMALLINT& c_type, std::deque<array_binding>& column_bindings) {

            SQLLEN buffer_length{sizeof(T)};
            column_bindings.emplace_back(
                array_binding {
                    std::vector<T>(T{}, rowset_size),
                    std::vector<SQLLEN>(0, rowset_size)
                }
            );

            auto& val = std::get<std::vector<T>>(column_bindings.back().value);
            auto& ind = column_bindings.back().indicators;
            switch (SQLBindCol(h_stmt, index, c_type, val.data(), buffer_length, ind.data())) {
            case SQL_SUCCESS:
                return true;
            case SQL_SUCCESS_WITH_INFO:
                diag.update(h_stmt, diagnostic_set::handle_type::stmt, std::string{"SQLBindCol() -> STRING -> SUCCESS_WITH_INFO"});
                return true;
            case SQL_INVALID_HANDLE:
                last_error = std::string{"could not bind the a string rowset to the column: invalid handle"};
                diag.update(h_stmt, diagnostic_set::handle_type::stmt, std::string{"SQLBindCol() -> STRING -> INVALID_HANDLE"});
                return false;
            default:
                last_error = std::string{"could not bind the a string rowset to the column: generic handle"};
                diag.update(h_stmt, diagnostic_set::handle_type::stmt, std::string{"SQLBindCol() -> STRING -> ERROR"});
                return false;
            }
        }

        bool bindcol_boolean(const SQLSMALLINT& index, Binding& binding) {
            SQLPOINTER p_val;
            SQLLEN buffer_length;
            if (binding.c_type == SQL_C_BIT) {
                auto& val = std::get<SQLCHAR>(binding.value);
                p_val = reinterpret_cast<SQLPOINTER>(&val);
                buffer_length = sizeof(SQLCHAR);
            } else {
                last_error = std::string{"invalid C data type for the boolean column"};
                return false;
            }

            switch (SQLBindCol(h_stmt, index, binding.c_type, p_val, buffer_length, &binding.indicator)) {
            case SQL_SUCCESS:
                return true;
            case SQL_SUCCESS_WITH_INFO:
                diag.update(h_stmt, diagnostic_set::handle_type::stmt, std::string{"SQLBindCol() -> BOOLEAN -> SUCCESS_WITH_INFO"});
                return true;
            default:
                last_error = std::string{"could not bind a boolean value to the provided column"};
                diag.update(h_stmt, diagnostic_set::handle_type::stmt, std::string{"SQLBindCol() -> BOOLEAN -> ERROR"});
                return false;
            }
        }

        bool bindcol_integer(const SQLSMALLINT& index, Binding& binding) {
            SQLPOINTER p_val;
            SQLLEN buffer_length;
            if (binding.c_type == SQL_C_TINYINT) {
                auto& val = std::get<SQLCHAR>(binding.value);
                p_val = reinterpret_cast<SQLPOINTER>(&val);
                buffer_length = sizeof(SQLCHAR);
            } else if (binding.c_type == SQL_C_SSHORT) {
                auto& val = std::get<SQLSMALLINT>(binding.value);
                p_val = reinterpret_cast<SQLPOINTER>(&val);
                buffer_length = sizeof(SQLSMALLINT);
            } else if (binding.c_type == SQL_C_SLONG) {
                auto& val = std::get<SQLINTEGER>(binding.value);
                p_val = reinterpret_cast<SQLPOINTER>(&val);
                buffer_length = sizeof(SQLINTEGER);
            } else if (binding.c_type == SQL_C_SBIGINT) {
                auto& val = std::get<SQLLEN>(binding.value);
                p_val = reinterpret_cast<SQLPOINTER>(&val);
                buffer_length = sizeof(SQLLEN);
            } else {
                last_error = std::string{"invalid C data type for the integer column"};
                return false;
            }

            switch (SQLBindCol(h_stmt, index, binding.c_type, p_val, buffer_length, &binding.indicator)) {
            case SQL_SUCCESS:
                return true;
            case SQL_SUCCESS_WITH_INFO:
                diag.update(h_stmt, diagnostic_set::handle_type::stmt, std::string{"SQLBindCol() -> INTEGER -> SUCCESS_WITH_INFO"});
                return true;
            default:
                last_error = std::string{"could not bind an integer value to the provided column"};
                diag.update(h_stmt, diagnostic_set::handle_type::stmt, std::string{"SQLBindCol() -> INTEGER -> ERROR"});
                return false;
            }
        }

        bool bindcol_guid(const SQLSMALLINT& index, Binding& binding) {
            SQLPOINTER p_val;
            SQLLEN buffer_length;
            if (binding.c_type == SQL_C_GUID) {
                auto& val = std::get<SQLGUID>(binding.value);
                p_val = reinterpret_cast<SQLPOINTER>(&val);
                buffer_length = sizeof(SQLGUID);
            } else {
                last_error = std::string{"invalid C data type for the GUID column"};
                return false;
            }

            switch (SQLBindCol(h_stmt, index, binding.c_type, p_val, buffer_length, &binding.indicator)) {
            case SQL_SUCCESS:
                return true;
            case SQL_SUCCESS_WITH_INFO:
                diag.update(h_stmt, diagnostic_set::handle_type::stmt, std::string{"SQLBindCol() -> GUID -> SUCCESS_WITH_INFO"});
                return true;
            default:
                last_error = std::string{"could not bind a GUID value to the provided column"};
                diag.update(h_stmt, diagnostic_set::handle_type::stmt, std::string{"SQLBindCol() -> GUID -> ERROR"});
                return false;
            }
        }

        bool bindcol_datetime(const SQLSMALLINT& index, Binding& binding) {
            SQLPOINTER p_val;
            SQLLEN buffer_length;
            if (binding.c_type == SQL_C_TYPE_TIMESTAMP) {
                auto& val = std::get<SQL_TIMESTAMP_STRUCT>(binding.value);
                p_val = reinterpret_cast<SQLPOINTER>(&val);
                buffer_length = sizeof(SQL_TIMESTAMP_STRUCT);
            } else {
                last_error = std::string{"invalid C data type for the datetime column"};
                return false;
            }

            switch (SQLBindCol(h_stmt, index, binding.c_type, p_val, buffer_length, &binding.indicator)) {
            case SQL_SUCCESS:
                return true;
            case SQL_SUCCESS_WITH_INFO:
                diag.update(h_stmt, diagnostic_set::handle_type::stmt, std::string{"SQLBindCol() -> DATETIME -> SUCCESS_WITH_INFO"});
                return true;
            default:
                last_error = std::string{"could not bind a datetime value to the provided column"};
                diag.update(h_stmt, diagnostic_set::handle_type::stmt, std::string{"SQLBindCol() -> DATETIME -> ERROR"});
                return false;
            }
        }

        bool bindcol_date(const SQLSMALLINT& index, Binding& binding) {
            SQLPOINTER p_val;
            SQLLEN buffer_length;
            if (binding.c_type == SQL_C_TYPE_DATE) {
                auto& val = std::get<SQL_DATE_STRUCT>(binding.value);
                p_val = reinterpret_cast<SQLPOINTER>(&val);
                buffer_length = sizeof(SQL_DATE_STRUCT);
            } else {
                last_error = std::string{"invalid C data type for the date column"};
                return false;
            }

            switch (SQLBindCol(h_stmt, index, binding.c_type, p_val, buffer_length, &binding.indicator)) {
            case SQL_SUCCESS:
                return true;
            case SQL_SUCCESS_WITH_INFO:
                diag.update(h_stmt, diagnostic_set::handle_type::stmt, std::string{"SQLBindCol() -> DATE -> SUCCESS_WITH_INFO"});
                return true;
            default:
                last_error = std::string{"could not bind a date value to the provided column"};
                diag.update(h_stmt, diagnostic_set::handle_type::stmt, std::string{"SQLBindCol() -> DATE -> ERROR"});
                return false;
            }
        }

        bool bindcol_time(const SQLSMALLINT& index, Binding& binding) {
            SQLPOINTER p_val;
            SQLLEN buffer_length;
            if (binding.c_type == SQL_C_TYPE_TIME) {
                auto& val = std::get<SQL_TIME_STRUCT>(binding.value);
                p_val = reinterpret_cast<SQLPOINTER>(&val);
                buffer_length = sizeof(SQL_TIME_STRUCT);
            } else {
                last_error = std::string{"invalid C data type for the time column"};
                return false;
            }

            switch (SQLBindCol(h_stmt, index, binding.c_type, p_val, buffer_length, &binding.indicator)) {
            case SQL_SUCCESS:
                return true;
            case SQL_SUCCESS_WITH_INFO:
                diag.update(h_stmt, diagnostic_set::handle_type::stmt, std::string{"SQLBindCol() -> TIME -> SUCCESS_WITH_INFO"});
                return true;
            default:
                last_error = std::string{"could not bind a time value to the provided column"};
                diag.update(h_stmt, diagnostic_set::handle_type::stmt, std::string{"SQLBindCol() -> TIME -> ERROR"});
                return false;
            }
        }

        bool bindcol_blob(const SQLSMALLINT& index, const SQLULEN& column_size, Binding& binding) {
            SQLPOINTER p_val;
            SQLLEN buffer_length;
            if (binding.c_type == SQL_C_BINARY) {
                buffer_length = column_size;
                auto& val = std::get<std::vector<SQLCHAR>>(binding.value);
                val.resize(buffer_length);
                p_val = reinterpret_cast<SQLPOINTER>(val.data());
            } else {
                last_error = std::string{"invalid C data type for the blob column"};
                return false;
            }

            switch (SQLBindCol(h_stmt, index, binding.c_type, p_val, buffer_length, &binding.indicator)) {
            case SQL_SUCCESS:
                return true;
            case SQL_SUCCESS_WITH_INFO:
                diag.update(h_stmt, diagnostic_set::handle_type::stmt, std::string{"SQLBindCol() -> BLOB -> SUCCESS_WITH_INFO"});
                return true;
            default:
                last_error = std::string{"could not bind a blob value to the provided column"};
                diag.update(h_stmt, diagnostic_set::handle_type::stmt, std::string{"SQLBindCol() -> BLOB -> ERROR"});
                return false;
            }
        }

        bool bind_columns_dynamic(const SQLSMALLINT& column_count, std::deque<Binding>& data_binding) {
            for (SQLSMALLINT i = 1; i <= column_count; ++i) {

                // get the column metadata
                std::basic_string<SQLWCHAR> column_name_buffer;
                column_name_buffer.resize(simql_constants::limits::max_sql_column_name_size);
                SQLSMALLINT column_name_length;
                SQLSMALLINT sql_type;
                SQLULEN definition;
                SQLSMALLINT scale;
                SQLSMALLINT null_id;

                switch (SQLDescribeColW(h_stmt, i, column_name_buffer.data(), sizeof(column_name_buffer), &column_name_length, &sql_type, &definition, &scale, &null_id)) {
                case SQL_SUCCESS:
                    break;
                case SQL_SUCCESS_WITH_INFO:
                    diag.update(h_stmt, diagnostic_set::handle_type::stmt, std::string{"SQLDescribeCol() -> SUCCESS_WITH_INFO"});
                    break;
                default:
                    last_error = std::string{"could not determine column metadata"};
                    diag.update(h_stmt, diagnostic_set::handle_type::stmt, std::string{"SQLDescribeCol() -> ERROR"});
                    return false;
                }

                // map library data type and bind column
                simql_types::sql_dtype data_type;
                bool bind_state;
                if (sql_type == SQL_VARCHAR || sql_type == SQL_CHAR) {
                    data_binding.emplace_back(Binding { std::basic_string<SQLCHAR>{}, 0 });
                    data_type = simql_types::sql_dtype::string;
                    data_binding.back().c_type = SQL_C_CHAR;
                    bind_state = bindcol_string(i, definition, data_binding.back());
                } else if (sql_type == SQL_WVARCHAR || sql_type == SQL_WCHAR) {
                    data_binding.emplace_back(Binding { std::basic_string<SQLWCHAR>{}, 0 });
                    data_type = simql_types::sql_dtype::string;
                    data_binding.back().c_type = SQL_C_WCHAR;
                    bind_state = bindcol_string(i, definition, data_binding.back());
                } else if (sql_type == SQL_DOUBLE || sql_type == SQL_FLOAT) {
                    data_binding.emplace_back(Binding { double{}, 0 });
                    data_type = simql_types::sql_dtype::floating_point;
                    data_binding.back().c_type = SQL_C_DOUBLE;
                    bind_state = bindcol_floating_point(i, data_binding.back());
                } else if (sql_type == SQL_REAL) {
                    data_binding.emplace_back(Binding { float{}, 0 });
                    data_type = simql_types::sql_dtype::floating_point;
                    data_binding.back().c_type = SQL_C_FLOAT;
                    bind_state = bindcol_floating_point(i, data_binding.back());
                } else if (sql_type == SQL_BIT) {
                    data_binding.emplace_back(Binding { SQLCHAR{}, 0 });
                    data_type = simql_types::sql_dtype::boolean;
                    data_binding.back().c_type = SQL_C_BIT;
                    bind_state = bindcol_boolean(i, data_binding.back());
                } else if (sql_type == SQL_TINYINT) {
                    data_binding.emplace_back(Binding { SQLCHAR{}, 0 });
                    data_type = simql_types::sql_dtype::integer;
                    data_binding.back().c_type = SQL_C_STINYINT;
                    bind_state = bindcol_integer(i, data_binding.back());
                } else if (sql_type == SQL_SMALLINT) {
                    data_binding.emplace_back(Binding { SQLSMALLINT{}, 0 });
                    data_type = simql_types::sql_dtype::integer;
                    data_binding.back().c_type = SQL_C_SSHORT;
                    bind_state = bindcol_integer(i, data_binding.back());
                } else if (sql_type == SQL_INTEGER) {
                    data_binding.emplace_back(Binding { SQLINTEGER{}, 0 });
                    data_type = simql_types::sql_dtype::integer;
                    data_binding.back().c_type = SQL_C_SLONG;
                    bind_state = bindcol_integer(i, data_binding.back());
                } else if (sql_type == SQL_BIGINT) {
                    data_binding.emplace_back(Binding { SQLLEN{}, 0 });
                    data_type = simql_types::sql_dtype::integer;
                    data_binding.back().c_type = SQL_C_SBIGINT;
                    bind_state = bindcol_integer(i, data_binding.back());
                } else if (sql_type == SQL_GUID) {
                    data_binding.emplace_back(Binding { SQLGUID{}, 0 });
                    data_type = simql_types::sql_dtype::guid;
                    data_binding.back().c_type = SQL_C_GUID;
                    bind_state = bindcol_guid(i, data_binding.back());
                } else if (sql_type == SQL_TYPE_TIMESTAMP) {
                    data_binding.emplace_back(Binding { SQL_TIMESTAMP_STRUCT{}, 0 });
                    data_type = simql_types::sql_dtype::datetime;
                    data_binding.back().c_type = SQL_C_TYPE_TIMESTAMP;
                    bind_state = bindcol_datetime(i, data_binding.back());
                } else if (sql_type == SQL_TYPE_DATE) {
                    data_binding.emplace_back(Binding { SQL_DATE_STRUCT{}, 0 });
                    data_type = simql_types::sql_dtype::date;
                    data_binding.back().c_type = SQL_C_TYPE_DATE;
                    bind_state = bindcol_date(i, data_binding.back());
                } else if (sql_type == SQL_TYPE_TIME) {
                    data_binding.emplace_back(Binding { SQL_TIME_STRUCT{}, 0 });
                    data_type = simql_types::sql_dtype::time;
                    data_binding.back().c_type = SQL_C_TYPE_TIME;
                    bind_state = bindcol_time(i, data_binding.back());
                } else if (sql_type == SQL_VARBINARY || sql_type == SQL_BINARY) {
                    data_binding.emplace_back(Binding { std::vector<SQLCHAR>{}, 0 });
                    data_type = simql_types::sql_dtype::blob;
                    data_binding.back().c_type = SQL_C_BINARY;
                    bind_state = bindcol_blob(i, definition, data_binding.back());
                } else {
                    last_error = std::string{"the column's SQL data type is not mapped"};
                    return false;
                }

                // add to the column vector
                if (!results.add_column(simql_types::sql_column {
                        simql_strings::from_odbc(std::basic_string_view<SQLWCHAR>(column_name_buffer.data(), column_name_length)),
                        data_type,
                        static_cast<std::uint64_t>(definition),
                        static_cast<std::int16_t>(scale),
                        null_rule(null_id)
                    })) {
                    last_error = std::string{"could not add the column metadata to the result set"};
                    return false;
                }
            }
            return true;
        }

        bool retrieve(std::deque<Binding>& data_binding) {
            std::vector<simql_types::sql_value> results_vector;
            std::uint16_t error_count{0};
            bool iterate = true;
            while (iterate) {
                switch (SQLFetch(h_stmt)) {
                case SQL_SUCCESS:
                    break;
                case SQL_SUCCESS_WITH_INFO:
                    diag.update(h_stmt, diagnostic_set::handle_type::stmt);
                    break;
                case SQL_NO_DATA:
                    iterate = false;
                    continue;
                default:
                    error_count++;
                    diag.update(h_stmt, diagnostic_set::handle_type::stmt);
                    if (error_count >= simql_constants::limits::max_error_fetches)
                        iterate = false;

                    continue;
                }

                for (size_t i = 0; i < results.columns().size(); ++i) {

                    if (data_binding[i].indicator == SQL_NULL_DATA) {
                        results_vector.push_back(simql_types::sql_value {
                            simql_types::sql_variant(),
                            results.columns()[i].data_type,
                            true
                        });
                        continue;
                    }

                    simql_types::sql_variant value;
                    switch (data_binding[i].c_type) {
                    case SQL_C_CHAR:
                        {
                            auto str = std::get<std::basic_string<SQLCHAR>>(data_binding[i].value);
                            str.resize(data_binding[i].indicator / sizeof(SQLCHAR));
                            value = simql_strings::from_odbc(std::basic_string_view<SQLCHAR>(str.data(), str.size()));
                        }
                        break;
                    case SQL_C_WCHAR:
                        {
                            auto str = std::get<std::basic_string<SQLWCHAR>>(data_binding[i].value);
                            str.resize(data_binding[i].indicator / sizeof(SQLWCHAR));
                            value = simql_strings::from_odbc(std::basic_string_view<SQLWCHAR>(str.data(), str.size()));
                        }
                        break;
                    case SQL_C_DOUBLE:
                        value = static_cast<double>(std::get<SQLDOUBLE>(data_binding[i].value));
                        break;
                    case SQL_C_FLOAT:
                        value = static_cast<double>(std::get<SQLREAL>(data_binding[i].value));
                        break;
                    case SQL_C_BIT:
                        value = static_cast<bool>(std::get<SQLCHAR>(data_binding[i].value));
                        break;
                    case SQL_C_STINYINT:
                        value = static_cast<int>(std::get<SQLCHAR>(data_binding[i].value));
                        break;
                    case SQL_C_SSHORT:
                        value = static_cast<int>(std::get<SQLSMALLINT>(data_binding[i].value));
                        break;
                    case SQL_C_SLONG:
                        value = static_cast<int>(std::get<SQLINTEGER>(data_binding[i].value));
                        break;
                    case SQL_C_SBIGINT:
                        value = static_cast<int>(std::get<SQLLEN>(data_binding[i].value));
                        break;
                    case SQL_C_GUID:
                        {
                            auto guid = std::get<SQLGUID>(data_binding[i].value);
                            simql_types::guid_struct x;
                            x.time_low = guid.Data1;
                            x.time_mid = guid.Data2;
                            x.time_high = guid.Data3;
                            std::copy(std::begin(guid.Data4), std::end(guid.Data4), std::begin(x.clock_seq_node));
                            value = x;
                        }
                        break;
                    case SQL_C_TYPE_TIMESTAMP:
                        {
                            auto dt = std::get<SQL_TIMESTAMP_STRUCT>(data_binding[i].value);
                            simql_types::datetime_struct x;
                            x.year = dt.year;
                            x.month = dt.month;
                            x.day = dt.day;
                            x.hour = dt.hour;
                            x.minute = dt.minute;
                            x.second = dt.second;
                            x.fraction = dt.fraction;
                            value = x;
                        }
                        break;
                    case SQL_C_TYPE_DATE:
                        {
                            auto d = std::get<SQL_DATE_STRUCT>(data_binding[i].value);
                            simql_types::date_struct x;
                            x.year = d.year;
                            x.month = d.month;
                            x.day = d.day;
                            value = x;
                        }
                        break;
                    case SQL_C_TYPE_TIME:
                        {
                            auto t = std::get<SQL_TIME_STRUCT>(data_binding[i].value);
                            simql_types::time_struct x;
                            x.hour = t.hour;
                            x.minute = t.minute;
                            x.second = t.second;
                            value = x;
                        }
                        break;
                    case SQL_C_BINARY:
                        {
                            auto bin = std::get<std::vector<SQLCHAR>>(data_binding[i].value);
                            bin.resize(data_binding[i].indicator / sizeof(SQLCHAR));
                            std::vector<std::uint8_t> blob;
                            blob.resize(bin.size());
                            std::transform(bin.begin(), bin.end(), blob.begin(), [&](SQLCHAR c) { return static_cast<std::uint8_t>(c); });
                            value = blob;
                        }
                        break;
                    }
                    results_vector.push_back(simql_types::sql_value {
                        value,
                        results.columns()[i].data_type,
                        false
                    });
                }
            }
            return results.set_data(std::move(results_vector));
        }

        bool prepare(std::string_view sql) {
            std::basic_string<SQLWCHAR> w_sql = simql_strings::to_odbc_w(sql);
            switch (SQLPrepareW(h_stmt, w_sql.data(), SQL_NTS)) {
            case SQL_SUCCESS:
                return true;
            case SQL_SUCCESS_WITH_INFO:
                diag.update(h_stmt, diagnostic_set::handle_type::stmt, std::string{"SQLPrepare(SQLHSTMT) -> SUCCESS_WITH_INFO"});
                return true;
            case SQL_INVALID_HANDLE:
                last_error = std::string{"could not prepare the provided SQL: invalid handle"};
                diag.update(h_stmt, diagnostic_set::handle_type::stmt, std::string{"SQLPrepare(SQLHSTMT) -> INVALID_HANDLE"});
                return false;
            default:
                last_error = std::string{"could not prepare the provided SQL: generic error"};
                diag.update(h_stmt, diagnostic_set::handle_type::stmt, std::string{"SQLPrepare(SQLHSTMT) -> ERROR"});
                return false;
            }
        }

        bool execute() {
            switch (SQLExecute(h_stmt)) {
            case SQL_SUCCESS:
                return true;
            case SQL_SUCCESS_WITH_INFO:
                diag.update(h_stmt, diagnostic_set::handle_type::stmt, std::string{"SQLExecute(SQLHSTMT) -> SUCCESS_WITH_INFO"});
                return true;
            case SQL_INVALID_HANDLE:
                last_error = std::string{"could not execute the prepared SQL: invalid handle"};
                diag.update(h_stmt, diagnostic_set::handle_type::stmt, std::string{"SQLExecute(SQLHSTMT) -> INVALID_HANDLE"});
                return false;
            default:
                last_error = std::string{"could not execute the prepared SQL: generic error"};
                diag.update(h_stmt, diagnostic_set::handle_type::stmt, std::string{"SQLExecute(SQLHSTMT) -> ERROR"});
                return false;
            }
        }

        bool execute_direct(std::string_view sql) {
            std::basic_string<SQLWCHAR> w_sql = simql_strings::to_odbc_w(sql);
            switch (SQLExecDirectW(h_stmt, w_sql.data(), SQL_NTS)) {
            case SQL_SUCCESS:
                return true;
            case SQL_SUCCESS_WITH_INFO:
                diag.update(h_stmt, diagnostic_set::handle_type::stmt, std::string{"SQLExecuteDirect(SQLHSTMT) -> SUCCESS_WITH_INFO"});
                return true;
            case SQL_INVALID_HANDLE:
                last_error = std::string{"could not execute the provided SQL: invalid handle"};
                diag.update(h_stmt, diagnostic_set::handle_type::stmt, std::string{"SQLExecuteDirect(SQLHSTMT) -> INVALID_HANDLE"});
                return false;
            default:
                last_error = std::string{"could not execute the provided SQL: generic error"};
                diag.update(h_stmt, diagnostic_set::handle_type::stmt, std::string{"SQLExecuteDirect(SQLHSTMT) -> ERROR"});
                return false;
            }
        }

        bool fetch_dynamic() {

            SQLSMALLINT column_count = get_column_count();
            if (column_count < 1)
                return false;

            std::deque<Binding> data_bindings;
            if (!bind_columns_dynamic(column_count, data_bindings))
                return false;

            if (!retrieve(data_bindings))
                return false;

            return true;
        }

        bool fetch_defined(std::vector<simql_types::sql_column>&& columns) {

        }

        bool fetch_values() {
            std::uint16_t iteration{0};
            SQLRETURN sr;
            do {
                iteration++;
                if (iteration >= simql_constants::limits::max_error_fetches)
                    break;

                sr = SQLMoreResults(h_stmt);
            } while (sr == SQL_SUCCESS || sr == SQL_SUCCESS_WITH_INFO);

            // bound parameters are now available for retrieval
        }

        bool next_result_set() {
            switch (SQLMoreResults(h_stmt)) {
            case SQL_SUCCESS:
                return true;
            case SQL_SUCCESS_WITH_INFO:
                diag.update(h_stmt, diagnostic_set::handle_type::stmt, std::string{"SQLMoreResults(SQLHSTMT) -> SUCCESS_WITH_INFO"});
                return true;
            case SQL_NO_DATA:
                return false;
            case SQL_INVALID_HANDLE:
                last_error = std::string{"could not request the next result set: invalid handle"};
                diag.update(h_stmt, diagnostic_set::handle_type::stmt, std::string{"SQLMoreResults(SQLHSTMT) -> INVALID_HANDLE"});
                return false;
            default:
                last_error = std::string{"could not request the next result set: generic error"};
                diag.update(h_stmt, diagnostic_set::handle_type::stmt, std::string{"SQLMoreResults(SQLHSTMT) -> ERROR"});
                return false;
            }
        }

        std::int64_t rows_affected() {
            SQLLEN rows;
            switch (SQLRowCount(h_stmt, &rows)) {
            case SQL_SUCCESS:
                return rows;
            case SQL_SUCCESS_WITH_INFO:
                diag.update(h_stmt, diagnostic_set::handle_type::stmt, std::string{"SQLRowCount() -> SUCCESS_WITH_INFO"});
                return rows;
            case SQL_INVALID_HANDLE:
                last_error = std::string{"could not retreive the affected row count: invalid handle"};
                diag.update(h_stmt, diagnostic_set::handle_type::stmt, std::string{"SQLRowCount() -> INVALID_HANDLE"});
                return -1;
            default:
                last_error = std::string{"could not retreive the affected row count: generic error"};
                diag.update(h_stmt, diagnostic_set::handle_type::stmt, std::string{"SQLRowCount() -> ERROR"});
                return -1;
            }
        }

    };

    statement::statement(database_connection& dbc, const statement::alloc_options& options) : p_handle(std::make_unique<handle>(dbc, options)) {}
    statement::~statement() = default;
    statement::statement(statement&&) noexcept = default;
    statement& statement::operator=(statement&&) noexcept = default;
    statement::statement(void* raw_stmt_handle, database_connection& conn, void* pool) : p_handle(std::make_unique<handle>(raw_stmt_handle, conn, pool)) {}

    void* statement::detach_handle() noexcept {
        if (!p_handle)
            return nullptr;

        if (!p_handle->h_stmt)
            return nullptr;

        if (p_handle->ownership == handle_ownership::owns)
            return nullptr;

        SQLHSTMT h = p_handle->h_stmt;
        p_handle->h_stmt = SQL_NULL_HSTMT;
        return reinterpret_cast<void*>(h);
    }

    bool statement::prepare(std::string_view sql) {
        return p_handle ? p_handle->prepare(sql) : false;
    }

    bool statement::execute() {
        return p_handle ? p_handle->execute() : false;
    }

    bool statement::execute_direct(std::string_view sql) {
        return p_handle ? p_handle->execute_direct(sql) : false;
    }

    bool statement::fetch_dynamic() {
        return !p_handle ? false : p_handle->fetch_dynamic();
    }

    bool statement::fetch_defined(std::vector<simql_types::sql_column>&& columns) {
        return !p_handle ? false : p_handle->fetch_defined(std::move(columns));
    }

    bool statement::next_result_set() {
        return p_handle ? p_handle->next_result_set() : false;
    }

    std::int64_t statement::rows_affected() {
        return !p_handle ? -1 : p_handle->rows_affected();
    }

    result_set* statement::results() {
        return !p_handle ? nullptr : &p_handle->results;
    }

    value_set* statement::values() {
        return !p_handle ? nullptr : &p_handle->values;
    }

    bool statement::bind_string(std::string name, std::string value, simql_types::parameter_binding_type binding_type, bool set_null) {
        return !p_handle ? false : p_handle->bindparam_string(name, value, binding_type, set_null);
    }

    bool statement::bind_floating_point(std::string name, double value, simql_types::parameter_binding_type binding_type, bool set_null) {
        return !p_handle ? false : p_handle->bindparam_floating_point(name, value, binding_type, set_null);
    }

    bool statement::bind_boolean(std::string name, bool value, simql_types::parameter_binding_type binding_type, bool set_null) {
        return !p_handle ? false : p_handle->bindparam_boolean(name, value, binding_type, set_null);
    }

    bool statement::bind_integer(std::string name, int value, simql_types::parameter_binding_type binding_type, bool set_null) {
        return !p_handle ? false : p_handle->bindparam_integer(name, value, binding_type, set_null);
    }

    bool statement::bind_guid(std::string name, simql_types::guid_struct value, simql_types::parameter_binding_type binding_type, bool set_null) {
        return !p_handle ? false : p_handle->bindparam_guid(name, value, binding_type, set_null);
    }

    bool statement::bind_datetime(std::string name, simql_types::datetime_struct value, simql_types::parameter_binding_type binding_type, bool set_null) {
        return !p_handle ? false : p_handle->bindparam_datetime(name, value, binding_type, set_null);
    }

    bool statement::bind_date(std::string name, simql_types::date_struct value, simql_types::parameter_binding_type binding_type, bool set_null) {
        return !p_handle ? false : p_handle->bindparam_date(name, value, binding_type, set_null);
    }

    bool statement::bind_time(std::string name, simql_types::time_struct value, simql_types::parameter_binding_type binding_type, bool set_null) {
        return !p_handle ? false : p_handle->bindparam_time(name, value, binding_type, set_null);
    }

    bool statement::bind_blob(std::string name, std::vector<std::uint8_t> value, simql_types::parameter_binding_type binding_type, bool set_null) {
        return !p_handle ? false : p_handle->bindparam_blob(name, value, binding_type, set_null);
    }

    bool statement::is_valid() {
        return !p_handle ? false : p_handle->is_valid;
    }

    std::string_view statement::last_error() {
        return !p_handle ? std::string_view{} : p_handle->last_error;
    }

    diagnostic_set* statement::diagnostics() {
        return !p_handle ? nullptr : &p_handle->diag;
    }

}