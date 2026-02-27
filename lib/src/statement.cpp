// SimQL stuff
#include "statement.hpp"
#include "database_connection.hpp"
#include "simql_strings.hpp"
#include "simql_constants.hpp"
#include "diagnostic_set.hpp"

// STL stuff
#include <cstdint>
#include <memory>
#include <vector>
#include <map>
#include <deque>
#include <variant>
#include <algorithm>
#include <type_traits>
#include <format>

// OS stuff
#include "os_inclusions.hpp"

// ODBC stuff
#include <sqltypes.h>
#include <sqlext.h>
#include <sql.h>

template <typename T>
concept variable_size =
    std::is_same_v<T, SQLCHAR> ||
    std::is_same_v<T, SQLWCHAR>;

template <typename T>
concept fixed_size =
    std::is_same_v<T, SQLDOUBLE> ||
    std::is_same_v<T, SQLREAL> ||
    std::is_same_v<T, SQLCHAR> ||
    std::is_same_v<T, SQLSMALLINT> ||
    std::is_same_v<T, SQLINTEGER> ||
    std::is_same_v<T, SQLLEN> ||
    std::is_same_v<T, SQLGUID> ||
    std::is_same_v<T, SQL_TIMESTAMP_STRUCT> ||
    std::is_same_v<T, SQL_DATE_STRUCT> ||
    std::is_same_v<T, SQL_TIME_STRUCT>;

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
        bool cursor_is_scrollable{false};
        bool is_valid{true};
        handle_ownership ownership{handle_ownership::owns};
        SQLUSMALLINT bound_parameter_index{1};
        SQLUINTEGER rows_fetched{0};
        SQLUINTEGER current_row_index{0};

        // odbc binding data type for parameters
        using bound_variant = std::variant<
            std::basic_string<SQLCHAR>,
            std::basic_string<SQLWCHAR>,
            std::vector<SQLCHAR>,
            SQLDOUBLE,
            SQLREAL,
            SQLCHAR,
            SQLSMALLINT,
            SQLINTEGER,
            SQLLEN,
            SQLGUID,
            SQL_TIMESTAMP_STRUCT,
            SQL_DATE_STRUCT,
            SQL_TIME_STRUCT
        >;
        struct value_binding {
            bound_variant value{};
            SQLLEN indicator{};
            SQLSMALLINT c_type{};
        };
        std::map<std::string, value_binding> bound_parameters;

        // odbc binding data type for columns
        using bound_array_variant = std::variant<
            std::vector<SQLCHAR>,
            std::vector<SQLWCHAR>,
            std::vector<SQLDOUBLE>,
            std::vector<SQLREAL>,
            std::vector<SQLSMALLINT>,
            std::vector<SQLINTEGER>,
            std::vector<SQLLEN>,
            std::vector<SQLGUID>,
            std::vector<SQL_TIMESTAMP_STRUCT>,
            std::vector<SQL_DATE_STRUCT>,
            std::vector<SQL_TIME_STRUCT>
        >;
        struct array_binding {
            bound_array_variant values{};
            std::vector<SQLLEN> indicators{};
            SQLSMALLINT c_type{};
        };
        std::deque<array_binding> bound_columns;
        std::vector<simql_types::sql_column> columns;

        struct column_binding {
        private:

            using buffer_variant_vector = std::variant<
                std::vector<std::monostate>,                // null
                std::vector<SQLCHAR>,                       // SQL_C_CHAR, SQL_C_BINARY, SQL_C_STINYINT
                std::vector<SQLWCHAR>,                      // SQL_C_WCHAR
                std::vector<SQLDOUBLE>,                     // SQL_C_DOUBLE
                std::vector<SQLREAL>,                       // SQL_C_FLOAT
                std::vector<SQLSMALLINT>,                   // SQL_C_SSHORT
                std::vector<SQLINTEGER>,                    // SQL_C_SLONG
                std::vector<SQLLEN>,                        // SQL_C_SBIGINT
                std::vector<simql_types::guid_struct>,      // SQL_C_GUID
                std::vector<simql_types::datetime_struct>,  // SQL_C_TYPE_TIMESTAMP
                std::vector<simql_types::date_struct>,      // SQL_C_TYPE_DATE
                std::vector<simql_types::time_struct>       // SQL_C_TYPE_TIME
            >;
            buffer_variant_vector buffer;

        public:

            SQLUSMALLINT        column_number;
            SQLSMALLINT         c_type;
            SQLLEN              bytes_per_value;
            std::vector<SQLLEN> indicators;

            void init_string(SQLUSMALLINT col_num, SQLLEN max_characters, SQLUINTEGER row_count) {
                column_number       = col_num;
                c_type              = SQL_C_CHAR;
                bytes_per_value     = (max_characters + 1) * sizeof(SQLCHAR);
                buffer              = std::vector<SQLCHAR>(row_count * (max_characters + 1));
                indicators.resize(row_count);
            }

            void init_wstring(SQLUSMALLINT col_num, SQLLEN max_characters, SQLUINTEGER row_count) {
                column_number       = col_num;
                c_type              = SQL_C_WCHAR;
                bytes_per_value     = (max_characters + 1) * sizeof(SQLWCHAR);
                buffer              = std::vector<SQLWCHAR>(row_count * (max_characters + 1));
                indicators.resize(row_count);
            }

            void init_double(SQLUSMALLINT col_num, SQLUINTEGER row_count) {
                column_number       = col_num;
                c_type              = SQL_C_DOUBLE;
                bytes_per_value     = sizeof(SQLDOUBLE);
                buffer              = std::vector<SQLDOUBLE>(row_count);
                indicators.resize(row_count);
            }

            void init_float(SQLUSMALLINT col_num, SQLUINTEGER row_count) {
                column_number       = col_num;
                c_type              = SQL_C_FLOAT;
                bytes_per_value     = sizeof(SQLREAL);
                buffer              = std::vector<SQLREAL>(row_count);
                indicators.resize(row_count);
            }

            void init_int8(SQLUSMALLINT col_num, SQLUINTEGER row_count) {
                column_number       = col_num;
                c_type              = SQL_C_STINYINT;
                bytes_per_value     = sizeof(SQLCHAR);
                buffer              = std::vector<SQLCHAR>(row_count);
                indicators.resize(row_count);
            }

            void init_int16(SQLUSMALLINT col_num, SQLUINTEGER row_count) {
                column_number       = col_num;
                c_type              = SQL_C_SSHORT;
                bytes_per_value     = sizeof(SQLSMALLINT);
                buffer              = std::vector<SQLSMALLINT>(row_count);
                indicators.resize(row_count);
            }

            void init_int32(SQLUSMALLINT col_num, SQLUINTEGER row_count) {
                column_number       = col_num;
                c_type              = SQL_C_SLONG;
                bytes_per_value     = sizeof(SQLINTEGER);
                buffer              = std::vector<SQLINTEGER>(row_count);
                indicators.resize(row_count);
            }

            void init_int64(SQLUSMALLINT col_num, SQLUINTEGER row_count) {
                column_number       = col_num;
                c_type              = SQL_C_SBIGINT;
                bytes_per_value     = sizeof(SQLLEN);
                buffer              = std::vector<SQLLEN>(row_count);
                indicators.resize(row_count);
            }

            void init_guid(SQLUSMALLINT col_num, SQLUINTEGER row_count) {
                column_number       = col_num;
                c_type              = SQL_C_GUID;
                bytes_per_value     = sizeof(simql_types::guid_struct);
                buffer              = std::vector<simql_types::guid_struct>(row_count);
                indicators.resize(row_count);
            }

            void init_datetime(SQLUSMALLINT col_num, SQLUINTEGER row_count) {
                column_number       = col_num;
                c_type              = SQL_C_TYPE_TIMESTAMP;
                bytes_per_value     = sizeof(simql_types::datetime_struct);
                buffer              = std::vector<simql_types::datetime_struct>(row_count);
                indicators.resize(row_count);
            }

            void init_date(SQLUSMALLINT col_num, SQLUINTEGER row_count) {
                column_number       = col_num;
                c_type              = SQL_C_TYPE_DATE;
                bytes_per_value     = sizeof(simql_types::date_struct);
                buffer              = std::vector<simql_types::date_struct>(row_count);
                indicators.resize(row_count);
            }

            void init_time(SQLUSMALLINT col_num, SQLUINTEGER row_count) {
                column_number       = col_num;
                c_type              = SQL_C_TYPE_TIME;
                bytes_per_value     = sizeof(simql_types::time_struct);
                buffer              = std::vector<simql_types::time_struct>(row_count);
                indicators.resize(row_count);
            }

            void init_blob(SQLUSMALLINT col_num, SQLLEN max_bytes, SQLUINTEGER row_count) {
                column_number       = col_num;
                c_type              = SQL_C_BINARY;
                bytes_per_value     = max_bytes;
                buffer              = std::vector<SQLCHAR>(row_count * max_bytes);
                indicators.resize(row_count);
            }

            SQLPOINTER ptr() {
                auto visitor = [&](buffer_variant_vector& bfr) -> SQLPOINTER {
                    using T = std::decay_t<decltype(bfr)>;
                    if constexpr (std::is_same_v<T, std::vector<SQLCHAR>>) {
                        return static_cast<SQLPOINTER>(std::get<std::vector<SQLCHAR>>(bfr).data());
                    } else if constexpr (std::is_same_v<T, std::vector<SQLWCHAR>>) {
                        return static_cast<SQLPOINTER>(std::get<std::vector<SQLWCHAR>>(bfr).data());
                    } else if constexpr (std::is_same_v<T, std::vector<SQLDOUBLE>>) {
                        return static_cast<SQLPOINTER>(std::get<std::vector<SQLDOUBLE>>(bfr).data());
                    } else if constexpr (std::is_same_v<T, std::vector<SQLREAL>>) {
                        return static_cast<SQLPOINTER>(std::get<std::vector<SQLREAL>>(bfr).data());
                    } else if constexpr (std::is_same_v<T, std::vector<SQLSMALLINT>>) {
                        return static_cast<SQLPOINTER>(std::get<std::vector<SQLSMALLINT>>(bfr).data());
                    } else if constexpr (std::is_same_v<T, std::vector<SQLINTEGER>>) {
                        return static_cast<SQLPOINTER>(std::get<std::vector<SQLINTEGER>>(bfr).data());
                    } else if constexpr (std::is_same_v<T, std::vector<SQLLEN>>) {
                        return static_cast<SQLPOINTER>(std::get<std::vector<SQLLEN>>(bfr).data());
                    } else if constexpr (std::is_same_v<T, std::vector<SQLGUID>>) {
                        return static_cast<SQLPOINTER>(std::get<std::vector<SQLGUID>>(bfr).data());
                    } else if constexpr (std::is_same_v<T, std::vector<SQL_TIMESTAMP_STRUCT>>) {
                        return static_cast<SQLPOINTER>(std::get<std::vector<SQL_TIMESTAMP_STRUCT>>(bfr).data());
                    } else if constexpr (std::is_same_v<T, std::vector<SQL_DATE_STRUCT>>) {
                        return static_cast<SQLPOINTER>(std::get<std::vector<SQL_DATE_STRUCT>>(bfr).data());
                    } else if constexpr (std::is_same_v<T, std::vector<SQL_TIME_STRUCT>>) {
                        return static_cast<SQLPOINTER>(std::get<std::vector<SQL_TIME_STRUCT>>(bfr).data());
                    } else {
                        return nullptr;
                    }
                };
                return std::visit<SQLPOINTER>(visitor, buffer);
            }
        };
        std::map<statement::column_value&, column_binding> column_bindings; 

        // --------------------------------------------------
        // LIFECYCLE
        // --------------------------------------------------

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

            // set the cursor scrollability
            is_valid = set_scrollable(options.is_scrollable);
            if (!is_valid)
                return;

            // set cursor sensitivity
            is_valid = set_cursor_sensitivity(options.sensitivity);
            if (!is_valid)
                return;

        }

        handle(void* stmt_handle, database_connection& conn, void* pool) noexcept {
            h_dbc = static_cast<SQLHDBC>(get_dbc_handle(conn));
            h_stmt = static_cast<SQLHSTMT>(stmt_handle);
            ownership = handle_ownership::borrows;
            p_pool = pool;

            SQLULEN is_scrollable;
            switch (SQLGetStmtAttrW(h_stmt, SQL_ATTR_CURSOR_SCROLLABLE, &is_scrollable, SQL_IS_INTEGER, nullptr)) {
            case SQL_SUCCESS:
                cursor_is_scrollable = is_scrollable == SQL_SCROLLABLE;
                break;
            case SQL_SUCCESS_WITH_INFO:
                cursor_is_scrollable = is_scrollable == SQL_SCROLLABLE;
                diag.update(h_stmt, diagnostic_set::handle_type::stmt, std::string{"SQLGetStmtAttr(SQL_ATTR_CURSOR_SCROLLABLE) -> SUCCESS_WITH_INFO"});
                break;
            case SQL_INVALID_HANDLE:
                last_error = std::string{"could not determine the cursor scrolling status: invalid handle"};
                diag.update(h_dbc, diagnostic_set::handle_type::dbc, std::string{"SQLGetStmtAttr(SQL_ATTR_CURSOR_SCROLLABLE) -> INVALID_HANDLE"});
                is_valid = false;
                break;
            default:
                last_error = std::string{"could not determine the cursor scrolling status: generic error"};
                diag.update(h_stmt, diagnostic_set::handle_type::stmt, std::string{"SQLGetStmtAttr(SQL_ATTR_CURSOR_SCROLLABLE) -> ERROR"});
                is_valid = false;
                break;
            }
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

        void reset() {
            SQLCloseCursor(h_stmt);
            SQLFreeStmt(h_stmt, SQL_RESET_PARAMS);
            SQLFreeStmt(h_stmt, SQL_UNBIND);
        }

        // --------------------------------------------------
        // CONFIGURATION
        // --------------------------------------------------

        bool set_query_timeout(std::uint32_t timeout) {
            SQLPOINTER p_query_timeout = reinterpret_cast<SQLPOINTER>(static_cast<SQLULEN>(timeout));
            switch (SQLSetStmtAttrW(h_stmt, SQL_ATTR_QUERY_TIMEOUT, p_query_timeout, SQL_IS_INTEGER)) {
            case SQL_SUCCESS:
                return true;
            case SQL_SUCCESS_WITH_INFO:
                diag.update(h_stmt, diagnostic_set::handle_type::stmt, std::string{"SQLSetStmtAttr(SQL_ATTR_QUERY_TIMEOUT) -> SUCCESS_WITH_INFO"});
                return true;
            case SQL_INVALID_HANDLE:
                last_error = std::string{"could not set the query timeout: invalid handle"};
                diag.update(h_dbc, diagnostic_set::handle_type::dbc, std::string{"SQLSetStmtAttr(SQL_ATTR_QUERY_TIMEOUT) -> INVALID_HANDLE"});
                return false;
            default:
                last_error = std::string{"could not set the query timeout: generic error"};
                diag.update(h_stmt, diagnostic_set::handle_type::stmt, std::string{"SQLSetStmtAttr(SQL_ATTR_QUERY_TIMEOUT) -> ERROR"});
                return false;
            }
        }

        bool set_max_rows(std::uint64_t max_rows) {
            SQLPOINTER p_max_rows = reinterpret_cast<SQLPOINTER>(static_cast<SQLULEN>(max_rows));
            switch (SQLSetStmtAttrW(h_stmt, SQL_ATTR_MAX_ROWS, p_max_rows, SQL_IS_INTEGER)) {
            case SQL_SUCCESS:
                return true;
            case SQL_SUCCESS_WITH_INFO:
                diag.update(h_stmt, diagnostic_set::handle_type::stmt, std::string{"SQLSetStmtAttr(SQL_ATTR_MAX_ROWS) -> SUCCESS_WITH_INFO"});
                return true;
            case SQL_INVALID_HANDLE:
                last_error = std::string{"could not set the max rows: invalid handle"};
                diag.update(h_dbc, diagnostic_set::handle_type::dbc, std::string{"SQLSetStmtAttr(SQL_ATTR_MAX_ROWS) -> INVALID_HANDLE"});
                return false;
            default:
                last_error = std::string{"could not set the max rows: generic error"};
                diag.update(h_stmt, diagnostic_set::handle_type::stmt, std::string{"SQLSetStmtAttr(SQL_ATTR_MAX_ROWS) -> ERROR"});
                return false;
            }
        }

        bool set_rowset_size(std::uint32_t rowset_size) {

            if (rowset_size <= 1) {
                last_error = std::string{"rowset size must be larger than 1"};
                return false;
            }

            SQLPOINTER p_rowset_size = reinterpret_cast<SQLPOINTER>(static_cast<SQLUINTEGER>(rowset_size));
            switch (SQLSetStmtAttrW(h_stmt, SQL_ATTR_ROW_ARRAY_SIZE, p_rowset_size, SQL_IS_INTEGER)) {
            case SQL_SUCCESS:
                return true;
            case SQL_SUCCESS_WITH_INFO:
                diag.update(h_stmt, diagnostic_set::handle_type::stmt, std::string{"SQLSetStmtAttr(SQL_ATTR_ROW_ARRAY_SIZE) -> SUCCESS_WITH_INFO"});
                return true;
            case SQL_INVALID_HANDLE:
                last_error = std::string{"could not set the rowset size: invalid handle"};
                diag.update(h_dbc, diagnostic_set::handle_type::dbc, std::string{"SQLSetStmtAttr(SQL_ATTR_ROW_ARRAY_SIZE) -> INVALID_HANDLE"});
                return false;
            default:
                last_error = std::string{"could not set the rowset size: generic error"};
                diag.update(h_stmt, diagnostic_set::handle_type::stmt, std::string{"SQLSetStmtAttr(SQL_ATTR_ROW_ARRAY_SIZE) -> ERROR"});
                return false;
            }
        }

        bool set_scrollable(bool is_scrollable) {
            SQLPOINTER p_is_scrollable = is_scrollable ? reinterpret_cast<SQLPOINTER>(SQL_SCROLLABLE) : reinterpret_cast<SQLPOINTER>(SQL_NONSCROLLABLE);
            switch (SQLSetStmtAttrW(h_stmt, SQL_ATTR_CURSOR_SCROLLABLE, p_is_scrollable, SQL_IS_INTEGER)) {
            case SQL_SUCCESS:
                cursor_is_scrollable = is_scrollable;
                return true;
            case SQL_SUCCESS_WITH_INFO:
                cursor_is_scrollable = is_scrollable;
                diag.update(h_stmt, diagnostic_set::handle_type::stmt, std::string{"SQLSetStmtAttr(SQL_ATTR_CURSOR_SCROLLABLE) -> SUCCESS_WITH_INFO"});
                return true;
            case SQL_INVALID_HANDLE:
                last_error = std::string{"could not set the rowset size: invalid handle"};
                diag.update(h_dbc, diagnostic_set::handle_type::dbc, std::string{"SQLSetStmtAttr(SQL_ATTR_CURSOR_SCROLLABLE) -> INVALID_HANDLE"});
                return false;
            default:
                last_error = std::string{"could not set the rowset size: generic error"};
                diag.update(h_stmt, diagnostic_set::handle_type::stmt, std::string{"SQLSetStmtAttr(SQL_ATTR_CURSOR_SCROLLABLE) -> ERROR"});
                return false;
            }
        }

        bool set_cursor_sensitivity(statement::cursor_sensitivity cursor) {
            SQLPOINTER p_cursor_sensitivity;
            switch (cursor) {
            case statement::cursor_sensitivity::unspecified:
                p_cursor_sensitivity = reinterpret_cast<SQLPOINTER>(SQL_UNSPECIFIED);
                break;
            case statement::cursor_sensitivity::insensitive:
                p_cursor_sensitivity = reinterpret_cast<SQLPOINTER>(SQL_INSENSITIVE);
                break;
            case statement::cursor_sensitivity::sensitive:
                p_cursor_sensitivity = reinterpret_cast<SQLPOINTER>(SQL_SENSITIVE);
                break;
            }

            switch (SQLSetStmtAttrW(h_stmt, SQL_ATTR_CURSOR_SENSITIVITY, p_cursor_sensitivity, SQL_IS_INTEGER)) {
            case SQL_SUCCESS:
                return true;
            case SQL_SUCCESS_WITH_INFO:
                diag.update(h_stmt, diagnostic_set::handle_type::stmt, std::string{"SQLSetStmtAttr(SQL_ATTR_CURSOR_SENSITIVITY) -> SUCCESS_WITH_INFO"});
                return true;
            case SQL_INVALID_HANDLE:
                last_error = std::string{"could not set the cursor sensitivity: invalid handle"};
                diag.update(h_dbc, diagnostic_set::handle_type::dbc, std::string{"SQLSetStmtAttr(SQL_ATTR_CURSOR_SENSITIVITY) -> INVALID_HANDLE"});
                return false;
            default:
                last_error = std::string{"could not set the cursor sensitivity: generic error"};
                diag.update(h_stmt, diagnostic_set::handle_type::stmt, std::string{"SQLSetStmtAttr(SQL_ATTR_CURSOR_SENSITIVITY) -> ERROR"});
                return false;
            }
        }

        bool update_fetched_row_count() {
            switch (SQLSetStmtAttrW(h_stmt, SQL_ATTR_ROWS_FETCHED_PTR, &rows_fetched, SQL_IS_INTEGER)) {
            case SQL_SUCCESS:
                return true;
            case SQL_SUCCESS_WITH_INFO:
                diag.update(h_stmt, diagnostic_set::handle_type::stmt, std::string{"SQLSetStmtAttr(SQL_ATTR_FETCHED_PTR) -> SUCCESS_WITH_INFO"});
                return true;
            case SQL_INVALID_HANDLE:
                last_error = std::string{"could not bind the fetched array size pointer: invalid handle"};
                diag.update(h_dbc, diagnostic_set::handle_type::dbc, std::string{"SQLSetStmtAttr(SQL_ATTR_FETCHED_PTR) -> INVALID_HANDLE"});
                return false;
            default:
                last_error = std::string{"could not bind the fetched array size pointer: generic error"};
                diag.update(h_stmt, diagnostic_set::handle_type::stmt, std::string{"SQLSetStmtAttr(SQL_ATTR_FETCHED_PTR) -> ERROR"});
                return false;
            }
        }

        // --------------------------------------------------
        // INTERNAL UTILITY
        // --------------------------------------------------

        SQLUINTEGER get_rowset_size() {
            SQLUINTEGER rowset_size{};
            switch (SQLGetStmtAttrW(h_stmt, SQL_ATTR_ROW_ARRAY_SIZE, &rowset_size, SQL_IS_INTEGER, nullptr)) {
            case SQL_SUCCESS:
                return rowset_size;
            case SQL_SUCCESS_WITH_INFO:
                diag.update(h_stmt, diagnostic_set::handle_type::stmt, std::string{"SQLGetStmtAttr(SQL_ATTR_ROW_ARRAY_SIZE) -> SUCCESS_WITH_INFO"});
                return rowset_size;
            case SQL_INVALID_HANDLE:
                last_error = std::string{"could not retrieve the SQL_ATTR_ROW_ARRAY_SIZE attribute: invalid handle"};
                diag.update(h_dbc, diagnostic_set::handle_type::dbc, std::string{"SQLGetStmtAttr(SQL_ATTR_ROW_ARRAY_SIZE) -> INVALID_HANDLE"});
                return 0;
            default:
                last_error = std::string{"could not retrieve the SQL_ATTR_ROW_ARRAY_SIZE attribute: generic error"};
                diag.update(h_stmt, diagnostic_set::handle_type::stmt, std::string{"SQLGetStmtAttr(SQL_ATTR_ROW_ARRAY_SIZE) -> ERROR"});
                return 0;
            }
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

        simql_types::null_rule_type null_rule(const SQLSMALLINT& null_id) {
            switch (null_id) {
            case SQL_NO_NULLS:
                return simql_types::null_rule_type::not_allowed;
            case SQL_NULLABLE:
                return simql_types::null_rule_type::allowed;
            case SQL_NULLABLE_UNKNOWN:
                return simql_types::null_rule_type::unknown;
            }
            return simql_types::null_rule_type::unknown;
        }

        SQLSMALLINT get_column_count() {
            SQLSMALLINT column_count{};
            if (SQL_SUCCEEDED(SQLNumResultCols(h_stmt, &column_count)))
                return column_count;

            return -1;
        }

        // --------------------------------------------------
        // EXECUTION
        // --------------------------------------------------

        bool prepare(std::string_view sql) {
            std::basic_string<SQLWCHAR> w_sql = simql_strings::to_odbc_w(sql);
            switch (SQLPrepareW(h_stmt, w_sql.data(), SQL_NTS)) {
            case SQL_SUCCESS:
                return true;
            case SQL_SUCCESS_WITH_INFO:
                diag.update(h_stmt, diagnostic_set::handle_type::stmt, std::string{"SQLPrepare() -> SUCCESS_WITH_INFO"});
                return true;
            case SQL_INVALID_HANDLE:
                last_error = std::string{"could not prepare the provided SQL: invalid handle"};
                diag.update(h_dbc, diagnostic_set::handle_type::dbc, std::string{"SQLPrepare() -> INVALID_HANDLE"});
                return false;
            default:
                last_error = std::string{"could not prepare the provided SQL: generic error"};
                diag.update(h_stmt, diagnostic_set::handle_type::stmt, std::string{"SQLPrepare() -> ERROR"});
                return false;
            }
        }

        bool execute() {
            switch (SQLExecute(h_stmt)) {
            case SQL_SUCCESS:
                return true;
            case SQL_SUCCESS_WITH_INFO:
                diag.update(h_stmt, diagnostic_set::handle_type::stmt, std::string{"SQLExecute() -> SUCCESS_WITH_INFO"});
                return true;
            case SQL_INVALID_HANDLE:
                last_error = std::string{"could not execute the prepared SQL: invalid handle"};
                diag.update(h_dbc, diagnostic_set::handle_type::dbc, std::string{"SQLExecute() -> INVALID_HANDLE"});
                return false;
            default:
                last_error = std::string{"could not execute the prepared SQL: generic error"};
                diag.update(h_stmt, diagnostic_set::handle_type::stmt, std::string{"SQLExecute() -> ERROR"});
                return false;
            }
        }

        bool execute_direct(std::string_view sql) {
            std::basic_string<SQLWCHAR> w_sql = simql_strings::to_odbc_w(sql);
            switch (SQLExecDirectW(h_stmt, w_sql.data(), SQL_NTS)) {
            case SQL_SUCCESS:
                return true;
            case SQL_SUCCESS_WITH_INFO:
                diag.update(h_stmt, diagnostic_set::handle_type::stmt, std::string{"SQLExecuteDirect() -> SUCCESS_WITH_INFO"});
                return true;
            case SQL_INVALID_HANDLE:
                last_error = std::string{"could not execute the provided SQL: invalid handle"};
                diag.update(h_dbc, diagnostic_set::handle_type::dbc, std::string{"SQLExecuteDirect() -> INVALID_HANDLE"});
                return false;
            default:
                last_error = std::string{"could not execute the provided SQL: generic error"};
                diag.update(h_stmt, diagnostic_set::handle_type::stmt, std::string{"SQLExecuteDirect() -> ERROR"});
                return false;
            }
        }

        // --------------------------------------------------
        // FILL DATA BUFFERS
        // --------------------------------------------------

        bool update_buffers() {

            SQLSMALLINT column_count = get_column_count();
            if (column_count < 1) {
                last_error = std::string{"could not determine the result set column count"};
                return false;
            }

            SQLUINTEGER rowset_size = get_rowset_size();
            if (rowset_size <= 1) {
                last_error = std::string{"invalid rowset size"};
                return false;
            }

            if (!bind_columns(column_count, rowset_size)) {
                last_error = std::string{"could not bind arrays to the result set columns"};
                return false;
            }

            return true;
        }

        bool fetch_first() {
            switch (SQLFetchScroll(h_stmt, SQL_FETCH_FIRST, 0)) {
            case SQL_SUCCESS:
                return update_fetched_row_count();
            case SQL_SUCCESS_WITH_INFO:
                diag.update(h_stmt, diagnostic_set::handle_type::stmt, std::string{"SQLFetchScroll(SQL_FETCH_FIRST) -> SUCCESS_WITH_INFO"});
                return update_fetched_row_count();
            case SQL_INVALID_HANDLE:
                last_error = std::string{"could not fetch-first from the result set: invalid handle"};
                diag.update(h_dbc, diagnostic_set::handle_type::dbc, std::string{"SQLFetchScroll(SQL_FETCH_FIRST) -> INVALID_HANDLE"});
                return false;
            default:
                last_error = std::string{"could not fetch-first from the result set: generic error"};
                diag.update(h_stmt, diagnostic_set::handle_type::stmt, std::string{"SQLFetchScroll(SQL_FETCH_FIRST) -> ERROR"});
                return false;
            }
        }

        bool fetch_last() {
            switch (SQLFetchScroll(h_stmt, SQL_FETCH_LAST, 0)) {
            case SQL_SUCCESS:
                return update_fetched_row_count();
            case SQL_SUCCESS_WITH_INFO:
                diag.update(h_stmt, diagnostic_set::handle_type::stmt, std::string{"SQLFetchScroll(SQL_FETCH_LAST) -> SUCCESS_WITH_INFO"});
                return update_fetched_row_count();
            case SQL_INVALID_HANDLE:
                last_error = std::string{"could not fetch-last from the result set: invalid handle"};
                diag.update(h_dbc, diagnostic_set::handle_type::dbc, std::string{"SQLFetchScroll(SQL_FETCH_LAST) -> INVALID_HANDLE"});
                return false;
            default:
                last_error = std::string{"could not fetch-last from the result set: generic error"};
                diag.update(h_stmt, diagnostic_set::handle_type::stmt, std::string{"SQLFetchScroll(SQL_FETCH_LAST) -> ERROR"});
                return false;
            }
        }

        bool fetch_prev() {
            switch (SQLFetchScroll(h_stmt, SQL_FETCH_PREV, 0)) {
            case SQL_SUCCESS:
                return update_fetched_row_count();
            case SQL_SUCCESS_WITH_INFO:
                diag.update(h_stmt, diagnostic_set::handle_type::stmt, std::string{"SQLFetchScroll(SQL_FETCH_PREV) -> SUCCESS_WITH_INFO"});
                return update_fetched_row_count();
            case SQL_INVALID_HANDLE:
                last_error = std::string{"could not fetch-prev from the result set: invalid handle"};
                diag.update(h_dbc, diagnostic_set::handle_type::dbc, std::string{"SQLFetchScroll(SQL_FETCH_PREV) -> INVALID_HANDLE"});
                return false;
            default:
                last_error = std::string{"could not fetch-prev from the result set: generic error"};
                diag.update(h_stmt, diagnostic_set::handle_type::stmt, std::string{"SQLFetchScroll(SQL_FETCH_PREV) -> ERROR"});
                return false;
            }
        }

        bool fetch_next() {
            switch (SQLFetchScroll(h_stmt, SQL_FETCH_NEXT, 0)) {
            case SQL_SUCCESS:
                return update_fetched_row_count();
            case SQL_SUCCESS_WITH_INFO:
                diag.update(h_stmt, diagnostic_set::handle_type::stmt, std::string{"SQLFetchScroll(SQL_FETCH_NEXT) -> SUCCESS_WITH_INFO"});
                return update_fetched_row_count();
            case SQL_INVALID_HANDLE:
                last_error = std::string{"could not fetch-next from the result set: invalid handle"};
                diag.update(h_dbc, diagnostic_set::handle_type::dbc, std::string{"SQLFetchScroll(SQL_FETCH_NEXT) -> INVALID_HANDLE"});
                return false;
            default:
                last_error = std::string{"could not fetch-next from the result set: generic error"};
                diag.update(h_stmt, diagnostic_set::handle_type::stmt, std::string{"SQLFetchScroll(SQL_FETCH_NEXT) -> ERROR"});
                return false;
            }
        }

        // --------------------------------------------------
        // RESULT NAVIGATION
        // --------------------------------------------------

        bool first_record() {

            if (!cursor_is_scrollable) {
                last_error = std::string{"cursor scrolling is disabled"};
                return false;
            }

            if (!fetch_first())
                return false;

            current_row_index = 0;
            return true;
        }

        bool last_record() {

            if (!cursor_is_scrollable) {
                last_error = std::string{"cursor scrolling is disabled"};
                return false;
            }

            if (!fetch_last())
                return false;

            current_row_index = rows_fetched - 1;
            return true;
        }

        bool prev_record() {

            if (current_row_index - 1 > 0) {
                current_row_index--;
                return true;
            } else {

                if (!cursor_is_scrollable) {
                    last_error = std::string{"cursor scrolling is disabled"};
                    return false;
                }

                if (!fetch_prev())
                    return false;

                current_row_index = rows_fetched - 1;
                return true;
            }

        }

        bool next_record() {

            if (current_row_index + 1 < rows_fetched) {
                current_row_index++;
                return true;
            } else {

                if (!fetch_next())
                    return false;

                current_row_index = 0;
                return true;
            }

        }

        bool next_result_set() {
            columns.clear();
            bound_columns.clear();
            switch (SQLMoreResults(h_stmt)) {
            case SQL_SUCCESS:
                return true;
            case SQL_SUCCESS_WITH_INFO:
                diag.update(h_stmt, diagnostic_set::handle_type::stmt, std::string{"SQLMoreResults() -> SUCCESS_WITH_INFO"});
                return true;
            case SQL_NO_DATA:
                return false;
            case SQL_INVALID_HANDLE:
                last_error = std::string{"could not request the next result set: invalid handle"};
                diag.update(h_dbc, diagnostic_set::handle_type::dbc, std::string{"SQLMoreResults() -> INVALID_HANDLE"});
                return false;
            default:
                last_error = std::string{"could not request the next result set: generic error"};
                diag.update(h_stmt, diagnostic_set::handle_type::stmt, std::string{"SQLMoreResults() -> ERROR"});
                return false;
            }
        }

        bool goto_bound_parameters() {
            bool exit_condition{false};
            while (true) {
                switch (SQLMoreResults(h_stmt)) {
                case SQL_SUCCESS:
                    break;
                case SQL_SUCCESS_WITH_INFO:
                    diag.update(h_stmt, diagnostic_set::handle_type::stmt, std::string{"SQLMoreResults() -> SUCCESS_WITH_INFO"});
                    break;
                case SQL_INVALID_HANDLE:
                    last_error = std::string{"could not advance to the next result set: invalid handle"};
                    diag.update(h_dbc, diagnostic_set::handle_type::dbc, std::string{"SQLMoreResults() -> INVALID_HANDLE"});
                    return false;
                case SQL_NO_DATA:
                    exit_condition = true;
                    break;
                default:
                    last_error = std::string{"could not advance to the next result set: generic error"};
                    diag.update(h_stmt, diagnostic_set::handle_type::stmt, std::string{"SQLMoreResults() -> ERROR"});
                    return false;
                }
                if (exit_condition)
                    break;
            }
            return true;
        }

        // --------------------------------------------------
        // DATA RETRIEVAL
        // --------------------------------------------------

        bool parameter_value(const std::string& name, simql_types::sql_value& value) {
            auto it = bound_parameters.find(name);
            if (it == bound_parameters.end())
                return false;

            switch (it->second.c_type) {
            case SQL_C_CHAR:
                {
                    auto str = std::get<std::basic_string<SQLCHAR>>(it->second.value);
                    str.resize(it->second.indicator / sizeof(SQLCHAR));
                    value.data = simql_strings::from_odbc(std::basic_string_view<SQLCHAR>(str.data(), str.size()));
                    value.data_type = simql_types::sql_dtype::string;
                }
                break;
            case SQL_C_WCHAR:
                {
                    auto str = std::get<std::basic_string<SQLWCHAR>>(it->second.value);
                    str.resize(it->second.indicator / sizeof(SQLWCHAR));
                    value.data = simql_strings::from_odbc(std::basic_string_view<SQLWCHAR>(str.data(), str.size()));
                    value.data_type = simql_types::sql_dtype::string;
                }
                break;
            case SQL_C_DOUBLE:
                value.data = static_cast<double>(std::get<SQLDOUBLE>(it->second.value));
                value.data_type = simql_types::sql_dtype::floating_point;
                break;
            case SQL_C_FLOAT:
                value.data = static_cast<double>(std::get<SQLREAL>(it->second.value));
                value.data_type = simql_types::sql_dtype::floating_point;
                break;
            case SQL_C_BIT:
                value.data = static_cast<bool>(std::get<SQLCHAR>(it->second.value));
                value.data_type = simql_types::sql_dtype::boolean;
                break;
            case SQL_C_STINYINT:
                value.data = static_cast<int>(std::get<SQLCHAR>(it->second.value));
                value.data_type = simql_types::sql_dtype::integer;
                break;
            case SQL_C_SSHORT:
                value.data = static_cast<int>(std::get<SQLSMALLINT>(it->second.value));
                value.data_type = simql_types::sql_dtype::integer;
                break;
            case SQL_C_SLONG:
                value.data = static_cast<int>(std::get<SQLINTEGER>(it->second.value));
                value.data_type = simql_types::sql_dtype::integer;
                break;
            case SQL_C_SBIGINT:
                value.data = static_cast<int>(std::get<SQLLEN>(it->second.value));
                value.data_type = simql_types::sql_dtype::integer;
                break;
            case SQL_C_GUID:
                {
                    auto guid = std::get<SQLGUID>(it->second.value);
                    simql_types::guid_struct x;
                    x.time_low = guid.Data1;
                    x.time_mid = guid.Data2;
                    x.time_high = guid.Data3;
                    std::copy(std::begin(guid.Data4), std::end(guid.Data4), std::begin(x.clock_seq_node));
                    value.data = x;
                    value.data_type = simql_types::sql_dtype::guid;
                }
                break;
            case SQL_C_TYPE_TIMESTAMP:
                {
                    auto dt = std::get<SQL_TIMESTAMP_STRUCT>(it->second.value);
                    simql_types::datetime_struct x;
                    x.year = dt.year;
                    x.month = dt.month;
                    x.day = dt.day;
                    x.hour = dt.hour;
                    x.minute = dt.minute;
                    x.second = dt.second;
                    x.fraction = dt.fraction;
                    value.data = x;
                    value.data_type = simql_types::sql_dtype::datetime;
                }
                break;
            case SQL_C_TYPE_DATE:
                {
                    auto d = std::get<SQL_DATE_STRUCT>(it->second.value);
                    simql_types::date_struct x;
                    x.year = d.year;
                    x.month = d.month;
                    x.day = d.day;
                    value.data = x;
                    value.data_type = simql_types::sql_dtype::date;
                }
                break;
            case SQL_C_TYPE_TIME:
                {
                    auto t = std::get<SQL_TIME_STRUCT>(it->second.value);
                    simql_types::time_struct x;
                    x.hour = t.hour;
                    x.minute = t.minute;
                    x.second = t.second;
                    value.data = x;
                    value.data_type = simql_types::sql_dtype::time;
                }
                break;
            case SQL_C_BINARY:
                {
                    auto bin = std::get<std::vector<SQLCHAR>>(it->second.value);
                    bin.resize(it->second.indicator / sizeof(SQLCHAR));
                    std::vector<std::uint8_t> blob;
                    blob.resize(bin.size());
                    std::transform(bin.begin(), bin.end(), blob.begin(), [&](SQLCHAR c) { return static_cast<std::uint8_t>(c); });
                    value.data = blob;
                    value.data_type = simql_types::sql_dtype::blob;
                }
                break;
            }
            return true;
        }

        bool current_record(std::vector<simql_types::sql_value>& values) {
            values.clear();

            if (columns.empty() || bound_columns.empty()) {
                last_error = std::string{"column metadata and/or column bindings are empty"};
                return false;
            }

            if (columns.size() != bound_columns.size()) {
                last_error = std::string{"column metadata and column bindings are not in sync"};
                return false;
            }

            int column_index{0};
            for (array_binding& binding : bound_columns) {

                if (binding.indicators[current_row_index] == SQL_NULL_DATA) {
                    values.emplace_back(
                        simql_types::sql_variant{},
                        columns[column_index].data_type,
                        true
                    );
                    continue;
                }

                switch (binding.c_type) {
                case SQL_C_CHAR:
                    {
                        auto& ind = binding.indicators[current_row_index];
                        SQLCHAR* p_buffer = std::get<std::vector<SQLCHAR>>(binding.values).data();
                        SQLCHAR* p_row = p_buffer + current_row_index * (columns[column_index].size + 1);

                        bool is_null{false};
                        std::basic_string<SQLCHAR> odbc_str;
                        if (ind == SQL_NO_TOTAL) {
                            odbc_str = std::basic_string<SQLCHAR>(p_row);
                        } else if (ind >= 0) {
                            auto character_count = ind / sizeof(SQLCHAR);
                            odbc_str = std::basic_string<SQLCHAR>(p_row, character_count);
                        } else {
                            odbc_str = std::basic_string<SQLCHAR>{};
                            is_null = true;
                        }
                        values.emplace_back(
                            simql_strings::from_odbc(std::basic_string_view<SQLCHAR>(odbc_str.data(), odbc_str.size())),
                            simql_types::sql_dtype::string,
                            is_null
                        );
                    }
                    break;
                case SQL_C_WCHAR:
                    {
                        auto& ind = binding.indicators[current_row_index];
                        SQLWCHAR* p_buffer = std::get<std::vector<SQLWCHAR>>(binding.values).data();
                        SQLWCHAR* p_row = p_buffer + current_row_index * (columns[column_index].size + 1);

                        bool is_null{false};
                        std::basic_string<SQLWCHAR> odbc_str;
                        if (ind == SQL_NO_TOTAL) {
                            odbc_str = std::basic_string<SQLWCHAR>(p_row);
                        } else if (ind >= 0) {
                            auto character_count = ind / sizeof(SQLWCHAR);
                            odbc_str = std::basic_string<SQLWCHAR>(p_row, character_count);
                        } else {
                            odbc_str = std::basic_string<SQLWCHAR>{};
                            is_null = true;
                        }
                        values.emplace_back(
                            simql_strings::from_odbc(std::basic_string_view<SQLWCHAR>(odbc_str.data(), odbc_str.size())),
                            simql_types::sql_dtype::string,
                            is_null
                        );
                    }
                    break;
                case SQL_C_DOUBLE:
                    values.emplace_back(
                        static_cast<double>(std::get<std::vector<SQLDOUBLE>>(binding.values)[current_row_index]),
                        simql_types::sql_dtype::floating_point,
                        false
                    );
                    break;
                case SQL_C_FLOAT:
                    values.emplace_back(
                        static_cast<double>(std::get<std::vector<SQLREAL>>(binding.values)[current_row_index]),
                        simql_types::sql_dtype::floating_point,
                        false
                    );
                    break;
                case SQL_C_BIT:
                    values.emplace_back(
                        static_cast<bool>(std::get<std::vector<SQLCHAR>>(binding.values)[current_row_index]),
                        simql_types::sql_dtype::boolean,
                        false
                    );
                    break;
                case SQL_C_STINYINT:
                    values.emplace_back(
                        static_cast<int>(std::get<std::vector<SQLCHAR>>(binding.values)[current_row_index]),
                        simql_types::sql_dtype::integer,
                        false
                    );
                    break;
                case SQL_C_SSHORT:
                    values.emplace_back(
                        static_cast<int>(std::get<std::vector<SQLSMALLINT>>(binding.values)[current_row_index]),
                        simql_types::sql_dtype::integer,
                        false
                    );
                    break;
                case SQL_C_SLONG:
                    values.emplace_back(
                        static_cast<int>(std::get<std::vector<SQLINTEGER>>(binding.values)[current_row_index]),
                        simql_types::sql_dtype::integer,
                        false
                    );
                    break;
                case SQL_C_SBIGINT:
                    values.emplace_back(
                        static_cast<int>(std::get<std::vector<SQLLEN>>(binding.values)[current_row_index]),
                        simql_types::sql_dtype::integer,
                        false
                    );
                    break;
                case SQL_C_GUID:
                    {
                        auto guid = std::get<std::vector<SQLGUID>>(binding.values)[current_row_index];
                        simql_types::guid_struct library_guid;
                        library_guid.time_low = guid.Data1;
                        library_guid.time_mid = guid.Data2;
                        library_guid.time_high = guid.Data3;
                        std::copy(std::begin(guid.Data4), std::end(guid.Data4), std::begin(library_guid.clock_seq_node));
                        values.emplace_back(
                            library_guid,
                            simql_types::sql_dtype::guid,
                            false
                        );
                    }
                    break;
                case SQL_C_TYPE_TIMESTAMP:
                    {
                        auto dt = std::get<std::vector<SQL_TIMESTAMP_STRUCT>>(binding.values)[current_row_index];
                        simql_types::datetime_struct library_datetime;
                        library_datetime.year = dt.year;
                        library_datetime.month = dt.month;
                        library_datetime.day = dt.day;
                        library_datetime.hour = dt.hour;
                        library_datetime.minute = dt.minute;
                        library_datetime.second = dt.second;
                        library_datetime.fraction = dt.fraction;
                        values.emplace_back(
                            library_datetime,
                            simql_types::sql_dtype::datetime,
                            false
                        );
                    }
                    break;
                case SQL_C_TYPE_DATE:
                    {
                        auto d = std::get<std::vector<SQL_DATE_STRUCT>>(binding.values)[current_row_index];
                        simql_types::date_struct library_date;
                        library_date.year = d.year;
                        library_date.month = d.month;
                        library_date.day = d.day;
                        values.emplace_back(
                            library_date,
                            simql_types::sql_dtype::date,
                            false
                        );
                    }
                    break;
                case SQL_C_TYPE_TIME:
                    {
                        auto t = std::get<std::vector<SQL_TIMESTAMP_STRUCT>>(binding.values)[current_row_index];
                        simql_types::time_struct library_time;
                        library_time.hour = t.hour;
                        library_time.minute = t.minute;
                        library_time.second = t.second;
                        values.emplace_back(
                            library_time,
                            simql_types::sql_dtype::time,
                            false
                        );
                    }
                    break;
                case SQL_C_BINARY:
                    {
                        auto& ind = binding.indicators[current_row_index];
                        SQLCHAR* p_buffer = std::get<std::vector<SQLCHAR>>(binding.values).data();

                        bool is_null{false};
                        std::vector<std::uint8_t> blob{};
                        if (ind == SQL_NO_TOTAL) {
                            SQLCHAR* p_row = p_buffer + current_row_index * columns[column_index].size;
                            auto bytes = std::vector<SQLCHAR>(p_row, p_row + columns[column_index].size);
                            blob.resize(bytes.size());
                            std::transform(bytes.begin(), bytes.end(), blob.begin(), [&](SQLCHAR c) { return static_cast<std::uint8_t>(c); });
                        } else if (ind >= 0) {
                            SQLCHAR* p_row = p_buffer + current_row_index * columns[column_index].size;
                            std::uint64_t byte_count = (static_cast<std::uint64_t>(ind) < columns[column_index].size) ? static_cast<std::uint64_t>(ind) : columns[column_index].size;
                            auto bytes = std::vector<SQLCHAR>(p_row, p_row + byte_count);
                            blob.resize(bytes.size());
                            std::transform(bytes.begin(), bytes.end(), blob.begin(), [&](SQLCHAR c) { return static_cast<std::uint8_t>(c); });
                        } else {
                            is_null = true;
                        }

                        values.emplace_back(
                            blob,
                            simql_types::sql_dtype::string,
                            is_null
                        );
                    }
                    break;
                }
            }
            return true;
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
                diag.update(h_dbc, diagnostic_set::handle_type::dbc, std::string{"SQLRowCount() -> INVALID_HANDLE"});
                return -1;
            default:
                last_error = std::string{"could not retreive the affected row count: generic error"};
                diag.update(h_stmt, diagnostic_set::handle_type::stmt, std::string{"SQLRowCount() -> ERROR"});
                return -1;
            }
        }

        // --------------------------------------------------
        // PARAMETER BINDING
        // --------------------------------------------------

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

            if (!SQL_SUCCEEDED(SQLDescribeParam(h_stmt, bound_parameter_index, &sql_type, &definition, &scale, &nullable))) {
                sql_type = SQL_WVARCHAR;
                definition = static_cast<SQLULEN>(value.size() + 1);
            }

            SQLPOINTER p_val;
            SQLSMALLINT c_type;
            if (sql_type == SQL_VARCHAR || sql_type == SQL_CHAR) {
                c_type = SQL_C_CHAR;
                buffer_length = definition * sizeof(SQLCHAR);
                bound_parameters.emplace(name, value_binding { bound_variant {simql_strings::to_odbc_n(value)}, 0, c_type });
                auto& param = bound_parameters.at(name);
                auto& val = std::get<std::basic_string<SQLCHAR>>(param.value);
                p_val = reinterpret_cast<SQLPOINTER>(val.data());
            } else if (sql_type == SQL_WVARCHAR || sql_type == SQL_WCHAR) {
                c_type = SQL_C_WCHAR;
                buffer_length = definition * sizeof(SQLWCHAR);
                bound_parameters.emplace(name, value_binding { bound_variant {simql_strings::to_odbc_w(value)}, 0, c_type });
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

            switch (SQLBindParameter(h_stmt, bound_parameter_index, param_type(binding_type), c_type, sql_type, definition, scale, p_val, buffer_length, &bound_parameters.at(name).indicator)) {
            case SQL_SUCCESS:
                bound_parameter_index++;
                return true;
            case SQL_SUCCESS_WITH_INFO:
                bound_parameter_index++;
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

            if (!SQL_SUCCEEDED(SQLDescribeParam(h_stmt, bound_parameter_index, &sql_type, &definition, &scale, &nullable))) {
                sql_type = SQL_DOUBLE;
            }

            SQLPOINTER p_val;
            SQLSMALLINT c_type;
            if (sql_type == SQL_DOUBLE || sql_type == SQL_FLOAT) {
                sql_type = SQL_DOUBLE;
                c_type = SQL_C_DOUBLE;
                buffer_length = sizeof(SQLDOUBLE);
                bound_parameters.emplace(name, value_binding { bound_variant {value}, 0, c_type });
                auto& param = bound_parameters.at(name);
                auto& val = std::get<SQLDOUBLE>(param.value);
                p_val = reinterpret_cast<SQLPOINTER>(&val);
            } else if (sql_type == SQL_REAL) {
                c_type = SQL_C_FLOAT;
                buffer_length = sizeof(SQLREAL);
                bound_parameters.emplace(name, value_binding { bound_variant {value}, 0, c_type });
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

            switch (SQLBindParameter(h_stmt, bound_parameter_index, param_type(binding_type), c_type, sql_type, definition, scale, p_val, buffer_length, &bound_parameters.at(name).indicator)) {
            case SQL_SUCCESS:
                bound_parameter_index++;
                return true;
            case SQL_SUCCESS_WITH_INFO:
                bound_parameter_index++;
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

            if (!SQL_SUCCEEDED(SQLDescribeParam(h_stmt, bound_parameter_index, &sql_type, &definition, &scale, &nullable))) {
                sql_type = SQL_BIT;
                definition = 1;
            }

            SQLPOINTER p_val;
            SQLSMALLINT c_type;
            if (sql_type == SQL_BIT) {
                c_type = SQL_C_BIT;
                bound_parameters.emplace(name, value_binding { bound_variant {static_cast<SQLCHAR>(value)}, 0, c_type });
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

            switch (SQLBindParameter(h_stmt, bound_parameter_index, param_type(binding_type), c_type, sql_type, definition, scale, p_val, buffer_length, &bound_parameters.at(name).indicator)) {
            case SQL_SUCCESS:
                bound_parameter_index++;
                return true;
            case SQL_SUCCESS_WITH_INFO:
                bound_parameter_index++;
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

            if (!SQL_SUCCEEDED(SQLDescribeParam(h_stmt, bound_parameter_index, &sql_type, &definition, &scale, &nullable))) {
                sql_type = SQL_INTEGER;
            }

            SQLPOINTER p_val;
            SQLSMALLINT c_type;
            if (sql_type == SQL_TINYINT) {
                c_type = SQL_C_STINYINT;
                bound_parameters.emplace(name, value_binding { bound_variant {static_cast<SQLCHAR>(value)}, 0, c_type });
                auto& param = bound_parameters.at(name);
                auto& val = std::get<SQLCHAR>(param.value);
                p_val = reinterpret_cast<SQLPOINTER>(&val);
            } else if (sql_type == SQL_SMALLINT) {
                c_type = SQL_C_SSHORT;
                bound_parameters.emplace(name, value_binding { bound_variant {static_cast<SQLSMALLINT>(value)}, 0, c_type });
                auto& param = bound_parameters.at(name);
                auto& val = std::get<SQLSMALLINT>(param.value);
                p_val = reinterpret_cast<SQLPOINTER>(&val);
            } else if (sql_type == SQL_INTEGER) {
                c_type = SQL_C_SLONG;
                bound_parameters.emplace(name, value_binding { bound_variant {static_cast<SQLINTEGER>(value)}, 0, c_type });
                auto& param = bound_parameters.at(name);
                auto& val = std::get<SQLINTEGER>(param.value);
                p_val = reinterpret_cast<SQLPOINTER>(&val);
            } else if (sql_type == SQL_BIGINT) {
                c_type = SQL_C_SBIGINT;
                bound_parameters.emplace(name, value_binding { bound_variant {static_cast<SQLBIGINT>(value)}, 0, c_type });
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

            switch (SQLBindParameter(h_stmt, bound_parameter_index, param_type(binding_type), c_type, sql_type, definition, scale, p_val, buffer_length, &bound_parameters.at(name).indicator)) {
            case SQL_SUCCESS:
                bound_parameter_index++;
                return true;
            case SQL_SUCCESS_WITH_INFO:
                bound_parameter_index++;
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

            if (!SQL_SUCCEEDED(SQLDescribeParam(h_stmt, bound_parameter_index, &sql_type, &definition, &scale, &nullable))) {
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
                bound_parameters.emplace(name, value_binding { bound_variant {guid}, 0, c_type });
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

            switch (SQLBindParameter(h_stmt, bound_parameter_index, param_type(binding_type), c_type, sql_type, definition, scale, p_val, buffer_length, &bound_parameters.at(name).indicator)) {
            case SQL_SUCCESS:
                bound_parameter_index++;
                return true;
            case SQL_SUCCESS_WITH_INFO:
                bound_parameter_index++;
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

            bound_parameters.emplace(name, value_binding { bound_variant {x}, 0, c_type });
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

            switch (SQLBindParameter(h_stmt, bound_parameter_index, param_type(binding_type), c_type, sql_type, definition, scale, p_val, buffer_length, &bound_parameters.at(name).indicator)) {
            case SQL_SUCCESS:
                bound_parameter_index++;
                return true;
            case SQL_SUCCESS_WITH_INFO:
                bound_parameter_index++;
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

            bound_parameters.emplace(name, value_binding { bound_variant {x}, 0, c_type });
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

            switch (SQLBindParameter(h_stmt, bound_parameter_index, param_type(binding_type), c_type, sql_type, definition, scale, p_val, buffer_length, &bound_parameters.at(name).indicator)) {
            case SQL_SUCCESS:
                bound_parameter_index++;
                return true;
            case SQL_SUCCESS_WITH_INFO:
                bound_parameter_index++;
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

            bound_parameters.emplace(name, value_binding { bound_variant {x}, 0, c_type });
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

            switch (SQLBindParameter(h_stmt, bound_parameter_index, param_type(binding_type), c_type, sql_type, definition, scale, p_val, buffer_length, &bound_parameters.at(name).indicator)) {
            case SQL_SUCCESS:
                bound_parameter_index++;
                return true;
            case SQL_SUCCESS_WITH_INFO:
                bound_parameter_index++;
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

            if (!SQL_SUCCEEDED(SQLDescribeParam(h_stmt, bound_parameter_index, &sql_type, &definition, &scale, &nullable))) {
                definition = static_cast<SQLULEN>(value.size());
                sql_type = SQL_VARBINARY;
            }

            if (sql_type != SQL_VARBINARY && sql_type != SQL_BINARY) {
                last_error = std::string{"invalid SQL data type"};
                return false;
            }

            bound_parameters.emplace(name, value_binding { bound_variant {value}, 0, c_type });
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

            switch (SQLBindParameter(h_stmt, bound_parameter_index, param_type(binding_type), c_type, sql_type, definition, scale, p_val, buffer_length, &bound_parameters.at(name).indicator)) {
            case SQL_SUCCESS:
                bound_parameter_index++;
                return true;
            case SQL_SUCCESS_WITH_INFO:
                bound_parameter_index++;
                diag.update(h_stmt, diagnostic_set::handle_type::stmt, std::string{"SQLBindParameter() -> BLOB -> SUCCESS_WITH_INFO"});
                return true;
            default:
                last_error = std::string{"could not bind the blob parameter"};
                diag.update(h_stmt, diagnostic_set::handle_type::stmt, std::string{"SQLBindParameter() -> BLOB -> ERROR"});
                return false;
            }
        }

        // --------------------------------------------------
        // COLUMN BINDING
        // --------------------------------------------------

        bool add_column(statement::column_value& col) {

            SQLUSMALLINT column_index = static_cast<SQLUSMALLINT>(col.column_number);
            SQLUINTEGER rowset_size = get_rowset_size();
            if (rowset_size <= 1) {
                last_error = std::string{"invalid rowset size"};
                return false;
            }

            // create a column_binding based on the underlying type of the column_value
            column_binding cb;
            if (col.data.is_string()) {
                col.is_wide_string ? cb.init_wstring(column_index, col.column_size, rowset_size) : cb.init_string(column_index, col.column_size, rowset_size);
            } else if (col.data.is_double()) {
                cb.init_double(column_index, rowset_size);
            } else if (col.data.is_float()) {
                cb.init_double(column_index, rowset_size);
            } else if (col.data.is_int8()) {
                cb.init_int8(column_index, rowset_size);
            } else if (col.data.is_int16()) {
                cb.init_int16(column_index, rowset_size);
            } else if (col.data.is_int32()) {
                cb.init_int32(column_index, rowset_size);
            } else if (col.data.is_int64()) {
                cb.init_int64(column_index, rowset_size);
            } else if (col.data.is_guid()) {
                cb.init_guid(column_index, rowset_size);
            } else if (col.data.is_datetime()) {
                cb.init_datetime(column_index, rowset_size);
            } else if (col.data.is_date()) {
                cb.init_date(column_index, rowset_size);
            } else if (col.data.is_time()) {
                cb.init_time(column_index, rowset_size);
            } else if (col.data.is_blob()) {
                cb.init_blob(column_index, col.column_size, rowset_size);
            } else {
                last_error = std::string{"could not determine the column underlying data type"};
                return false;
            }

            // add to the association map
            column_bindings.emplace(col, cb);
        }

        bool bind_columns(const SQLSMALLINT& column_count, const SQLUINTEGER& rowset_size) {
            for (auto& col : column_bindings) {
                switch (SQLBindCol(h_stmt, col.second.column_number, col.second.c_type, col.second.ptr(), col.second.bytes_per_value, col.second.indicators.data())) {
                case SQL_SUCCESS:
                    break;
                case SQL_SUCCESS_WITH_INFO:
                    diag.update(h_stmt, diagnostic_set::handle_type::stmt, std::format("SQLBindCol()::{} -> SUCCESS_WITH_INFO", col.first.name));
                    break;
                case SQL_INVALID_HANDLE:
                    last_error = std::format("could not bind column: '{}' -> invalid handle", col.first.name);
                    diag.update(h_dbc, diagnostic_set::handle_type::dbc, std::format("SQLBindCol()::{} -> INVALID_HANDLE", col.first.name));
                    return false;
                default:
                    last_error = std::format("could not bind column: '{}' -> generic error", col.first.name);
                    diag.update(h_stmt, diagnostic_set::handle_type::stmt, std::format("SQLBindCol()::{} -> INVALID_HANDLE", col.first.name));
                    return false;
                }
            }
            return true;
        }

    };

    // --------------------------------------------------
    // LIFECYCLE
    // --------------------------------------------------

    statement::statement(database_connection& dbc, const statement::alloc_options& options) : p_handle(std::make_unique<handle>(dbc, options)) {}
    statement::~statement() = default;
    statement::statement(statement&&) noexcept = default;
    statement& statement::operator=(statement&&) noexcept = default;

    // --------------------------------------------------
    // EXECUTION
    // --------------------------------------------------

    template<typename T, typename...args>
    bool statement::prepare(std::string_view sql, T first, args... rest) {
        if (!p_handle)
            return false;

        return prepare(rest);
    }

    bool statement::prepare(std::string_view sql) {
        return !p_handle ? false : p_handle->prepare(sql);
    }

    bool statement::execute() {
        return p_handle ? p_handle->execute() : false;
    }

    bool statement::execute_direct(std::string_view sql) {
        return p_handle ? p_handle->execute_direct(sql) : false;
    }

    bool statement::open_cursor() {
        if (!p_handle)
            return false;

        if (!p_handle->update_buffers())
            return false;

        if (p_handle->cursor_is_scrollable) {
            return p_handle->fetch_first();
        } else {
            return p_handle->fetch_next();
        }
    }

    // --------------------------------------------------
    // RESULT NAVIGATION
    // --------------------------------------------------

    bool statement::first_record() {
        return !p_handle ? false : p_handle->first_record();
    }

    bool statement::last_record() {
        return !p_handle ? false : p_handle->last_record();
    }

    bool statement::prev_record() {
        return !p_handle ? false : p_handle->prev_record();
    }

    bool statement::next_record() {
        return !p_handle ? false : p_handle->next_record();
    }

    bool statement::next_result_set() {
        return !p_handle ? false : p_handle->next_result_set();
    }

    bool statement::goto_bound_parameters() {
        return !p_handle ? false : p_handle->goto_bound_parameters();
    }

    // --------------------------------------------------
    // DATA RETRIEVAL
    // --------------------------------------------------

    bool statement::parameter_value(const std::string& name, simql_types::sql_value& value) {
        return !p_handle ? false : p_handle->parameter_value(name, value);
    }

    bool statement::current_record(std::vector<simql_types::sql_value>& values) {
        return !p_handle ? false : p_handle->current_record(values);
    }

    std::int64_t statement::rows_affected() {
        return !p_handle ? -1 : p_handle->rows_affected();
    }

    // --------------------------------------------------
    // PARAMETER BINDING
    // --------------------------------------------------

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

    // --------------------------------------------------
    // DIAGNOSTICS
    // --------------------------------------------------

    bool statement::is_valid() {
        return !p_handle ? false : p_handle->is_valid;
    }

    std::string_view statement::last_error() {
        return !p_handle ? std::string_view{} : p_handle->last_error;
    }

    diagnostic_set* statement::diagnostics() {
        return !p_handle ? nullptr : &p_handle->diag;
    }

    // --------------------------------------------------
    // PRIVATE
    // --------------------------------------------------

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

}