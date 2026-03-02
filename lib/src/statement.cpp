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
#include <concepts>

// OS stuff
#include "os_inclusions.hpp"

// ODBC stuff
#include <sqltypes.h>
#include <sqlext.h>
#include <sql.h>

template<typename T>
concept column_binding_type =
    std::is_same_v<T, std::vector<SQLCHAR>> ||
    std::is_same_v<T, std::vector<SQLWCHAR>> ||
    std::is_same_v<T, std::vector<SQLDOUBLE>> ||
    std::is_same_v<T, std::vector<SQLREAL>> ||
    std::is_same_v<T, std::vector<SQLSMALLINT>> ||
    std::is_same_v<T, std::vector<SQLINTEGER>> ||
    std::is_same_v<T, std::vector<SQLLEN>> ||
    std::is_same_v<T, std::vector<simql_types::guid_struct>> ||
    std::is_same_v<T, std::vector<simql_types::datetime_struct>> ||
    std::is_same_v<T, std::vector<simql_types::date_struct>> ||
    std::is_same_v<T, std::vector<simql_types::time_struct>> ||
    std::is_same_v<T, std::vector<std::uint8_t>>;

template<typename T>
concept parameter_binding_type =
    std::is_same_v<T, std::basic_string<SQLCHAR>> ||
    std::is_same_v<T, std::basic_string<SQLWCHAR>> ||
    std::is_same_v<T, SQLCHAR> ||
    std::is_same_v<T, SQLWCHAR> ||
    std::is_same_v<T, SQLDOUBLE> ||
    std::is_same_v<T, SQLREAL> ||
    std::is_same_v<T, SQLSMALLINT> ||
    std::is_same_v<T, SQLINTEGER> ||
    std::is_same_v<T, SQLLEN> ||
    std::is_same_v<T, simql_types::guid_struct> ||
    std::is_same_v<T, simql_types::datetime_struct> ||
    std::is_same_v<T, simql_types::date_struct> ||
    std::is_same_v<T, simql_types::time_struct> ||
    std::is_same_v<T, std::vector<std::uint8_t>>;

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

        // binding for columns
        struct column_binding_struct {
        private:

            using buffer_variant = std::variant<
                std::vector<std::monostate>,                // null
                std::vector<SQLCHAR>,                       // SQL_C_CHAR, SQL_C_BINARY, SQL_C_STINYINT, SQL_C_BIT
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
            buffer_variant buffer;

        public:

            SQLSMALLINT             c_type;
            SQLLEN                  buffer_length;
            std::vector<SQLLEN>     indicators;
            statement::sql_column&  column;

            column_binding_struct(SQLUINTEGER row_count, statement::sql_column_string& col) : column(col) {
                if (col.is_wide) {
                    c_type              = SQL_C_WCHAR;
                    buffer_length       = (col.max_character_count + 1) * sizeof(SQLWCHAR);
                    buffer              = std::vector<SQLWCHAR>(row_count * (col.max_character_count + 1));
                    indicators.resize(row_count);
                } else {
                    c_type              = SQL_C_CHAR;
                    buffer_length       = (col.max_character_count + 1) * sizeof(SQLCHAR);
                    buffer              = std::vector<SQLCHAR>(row_count * (col.max_character_count + 1));
                    indicators.resize(row_count);
                }
            }

            column_binding_struct(SQLUINTEGER row_count, statement::sql_column_character& col) : column(col) {
                if (col.is_wide) {
                    c_type              = SQL_C_WCHAR;
                    buffer_length       = sizeof(SQLWCHAR);
                    buffer              = std::vector<SQLWCHAR>(row_count);
                    indicators.resize(row_count);
                } else {
                    c_type              = SQL_C_CHAR;
                    buffer_length       = sizeof(SQLCHAR);
                    buffer              = std::vector<SQLCHAR>(row_count);
                    indicators.resize(row_count);
                }
            }

            column_binding_struct(SQLUINTEGER row_count, statement::sql_column_boolean& col) : column(col) {
                c_type                  = SQL_C_BIT;
                buffer_length           = sizeof(SQLCHAR);
                buffer                  = std::vector<SQLCHAR>(row_count);
                indicators.resize(row_count);
            }

            column_binding_struct(SQLUINTEGER row_count, statement::sql_column_double& col) : column(col) {
                c_type                  = SQL_C_DOUBLE;
                buffer_length           = sizeof(SQLDOUBLE);
                buffer                  = std::vector<SQLDOUBLE>(row_count);
                indicators.resize(row_count);
            }

            column_binding_struct(SQLUINTEGER row_count, statement::sql_column_float& col) : column(col) {
                c_type                  = SQL_C_FLOAT;
                buffer_length           = sizeof(SQLREAL);
                buffer                  = std::vector<SQLREAL>(row_count);
                indicators.resize(row_count);
            }

            column_binding_struct(SQLUINTEGER row_count, statement::sql_column_int8& col) : column(col) {
                c_type                  = SQL_C_STINYINT;
                buffer_length           = sizeof(SQLCHAR);
                buffer                  = std::vector<SQLCHAR>(row_count);
                indicators.resize(row_count);
            }

            column_binding_struct(SQLUINTEGER row_count, statement::sql_column_int16& col) : column(col) {
                c_type                  = SQL_C_SSHORT;
                buffer_length           = sizeof(SQLSMALLINT);
                buffer                  = std::vector<SQLSMALLINT>(row_count);
                indicators.resize(row_count);
            }

            column_binding_struct(SQLUINTEGER row_count, statement::sql_column_int32& col) : column(col) {
                c_type                  = SQL_C_SLONG;
                buffer_length           = sizeof(SQLINTEGER);
                buffer                  = std::vector<SQLINTEGER>(row_count);
                indicators.resize(row_count);
            }

            column_binding_struct(SQLUINTEGER row_count, statement::sql_column_int64& col) : column(col) {
                c_type                  = SQL_C_SBIGINT;
                buffer_length           = sizeof(SQLLEN);
                buffer                  = std::vector<SQLLEN>(row_count);
                indicators.resize(row_count);
            }

            column_binding_struct(SQLUINTEGER row_count, statement::sql_column_guid& col) : column(col) {
                c_type                  = SQL_C_GUID;
                buffer_length           = sizeof(simql_types::guid_struct);
                buffer                  = std::vector<simql_types::guid_struct>(row_count);
                indicators.resize(row_count);
            }

            column_binding_struct(SQLUINTEGER row_count, statement::sql_column_datetime& col) : column(col) {
                c_type                  = SQL_C_TYPE_TIMESTAMP;
                buffer_length           = sizeof(simql_types::datetime_struct);
                buffer                  = std::vector<simql_types::datetime_struct>(row_count);
                indicators.resize(row_count);
            }

            column_binding_struct(SQLUINTEGER row_count, statement::sql_column_date& col) : column(col) {
                c_type                  = SQL_C_TYPE_DATE;
                buffer_length           = sizeof(simql_types::date_struct);
                buffer                  = std::vector<simql_types::date_struct>(row_count);
                indicators.resize(row_count);
            }

            column_binding_struct(SQLUINTEGER row_count, statement::sql_column_time& col) : column(col) {
                c_type                  = SQL_C_TYPE_TIME;
                buffer_length           = sizeof(simql_types::time_struct);
                buffer                  = std::vector<simql_types::time_struct>(row_count);
                indicators.resize(row_count);
            }

            column_binding_struct(SQLUINTEGER row_count, statement::sql_column_blob& col) : column(col) {
                c_type                  = SQL_C_BINARY;
                buffer_length           = col.max_byte_count;
                buffer                  = std::vector<SQLCHAR>(row_count * col.max_byte_count);
                indicators.reserve(row_count);
            }

            SQLPOINTER ptr() {
                auto visitor = [&](buffer_variant& bfr) -> SQLPOINTER {
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
                    } else if constexpr (std::is_same_v<T, std::vector<simql_types::guid_struct>>) {
                        return static_cast<SQLPOINTER>(std::get<std::vector<simql_types::guid_struct>>(bfr).data());
                    } else if constexpr (std::is_same_v<T, std::vector<simql_types::datetime_struct>>) {
                        return static_cast<SQLPOINTER>(std::get<std::vector<simql_types::datetime_struct>>(bfr).data());
                    } else if constexpr (std::is_same_v<T, std::vector<simql_types::date_struct>>) {
                        return static_cast<SQLPOINTER>(std::get<std::vector<simql_types::date_struct>>(bfr).data());
                    } else if constexpr (std::is_same_v<T, std::vector<simql_types::time_struct>>) {
                        return static_cast<SQLPOINTER>(std::get<std::vector<simql_types::time_struct>>(bfr).data());
                    } else {
                        return nullptr;
                    }
                };
                return std::visit<SQLPOINTER>(visitor, buffer);
            }

            void update(SQLUINTEGER row_index) {
                auto visitor = [&](buffer_variant& bfr) -> simql_types::sql_val {
                    using T = std::decay_t<decltype(bfr)>;

                    if constexpr (std::is_same_v<T, std::vector<SQLCHAR>>) {
                        auto& b = std::get<std::vector<SQLCHAR>>(bfr);

                        if (b.size() == indicators.size()) {
                            return simql_strings::from_odbc_char(b[row_index]);

                        switch (c_type) {
                        case SQL_C_CHAR:
                            if (indicators[row_index] == SQL_NO_TOTAL) {
                                return simql_strings::from_odbc(std::basic_string_view<SQLCHAR>(std::basic_string<SQLCHAR>(b.data() + row_index * (buffer_length + 1))));
                            } else if (indicators[row_index] >= 0) {
                                return simql_strings::from_odbc(std::basic_string_view<SQLCHAR>(std::basic_string<SQLCHAR>(b.data() + row_index * (buffer_length + 1), indicators[row_index] / sizeof(SQLCHAR))));
                            } else {
                                return std::monostate{};
                            }
                        case SQL_C_STINYINT:
                            return static_cast<std::int8_t>(b[row_index]);
                        case SQL_C_BINARY:
                            if (indicators[row_index] == SQL_NO_TOTAL) {
                                SQLCHAR* p_row = b.data() + row_index * buffer_length;
                                auto bytes = std::vector<SQLCHAR>(p_row, p_row + buffer_length);
                                std::vector<std::uint8_t> blob;
                                blob.resize(bytes.size());
                                std::transform(bytes.begin(), bytes.end(), blob.begin(), [&](SQLCHAR c) { return static_cast<std::uint8_t>(c); });
                                return blob;
                            } else if (indicators[row_index] >= 0) {
                                SQLCHAR* p_row = b.data() + row_index * buffer_length;
                                auto byte_count = (static_cast<SQLLEN>(indicators[row_index]) < buffer_length) ? static_cast<SQLLEN>(indicators[row_index]) : buffer_length;
                                auto bytes = std::vector<SQLCHAR>(p_row, p_row + byte_count);
                                std::vector<std::uint8_t> blob;
                                blob.resize(bytes.size());
                                std::transform(bytes.begin(), bytes.end(), blob.begin(), [&](SQLCHAR c) { return static_cast<std::uint8_t>(c); });
                                return blob;
                            } else {
                                return std::monostate{};
                            }
                            break;
                        case SQL_C_BIT:
                            return static_cast<bool>(b[row_index]);
                        default:
                            return std::monostate{};
                        }

                        // could be a narrow string, tiny integer, or blob
                    } else if constexpr (std::is_same_v<T, std::vector<SQLWCHAR>>) {

                        auto& b = std::get<std::vector<SQLWCHAR>>(bfr);
                        if (indicators[row_index] == SQL_NO_TOTAL) {
                            return simql_strings::from_odbc(std::basic_string_view<SQLWCHAR>(std::basic_string<SQLWCHAR>(b.data() + row_index * (buffer_length + 1))));
                        } else if (indicators[row_index] >= 0) {
                            return simql_strings::from_odbc(std::basic_string_view<SQLWCHAR>(std::basic_string<SQLWCHAR>(b.data() + row_index * (buffer_length + 1), indicators[row_index] / sizeof(SQLWCHAR))));
                        } else {
                            return std::monostate{};
                        }

                        // is a wide string
                    } else if constexpr (std::is_same_v<T, std::vector<SQLDOUBLE>>) {

                        return static_cast<double>(std::get<std::vector<SQLDOUBLE>>(bfr)[row_index]);
                    } else if constexpr (std::is_same_v<T, std::vector<SQLREAL>>) {

                        return static_cast<float>(std::get<std::vector<SQLREAL>>(bfr)[row_index]);
                    } else if constexpr (std::is_same_v<T, std::vector<SQLSMALLINT>>) {

                        return static_cast<std::int16_t>(std::get<std::vector<SQLSMALLINT>>(bfr)[row_index]);
                    } else if constexpr (std::is_same_v<T, std::vector<SQLINTEGER>>) {

                        return static_cast<std::int32_t>(std::get<std::vector<SQLINTEGER>>(bfr)[row_index]);
                    } else if constexpr (std::is_same_v<T, std::vector<SQLLEN>>) {

                        return static_cast<std::int64_t>(std::get<std::vector<SQLLEN>>(bfr)[row_index]);
                    } else if constexpr (std::is_same_v<T, std::vector<simql_types::guid_struct>>) {

                        return static_cast<simql_types::guid_struct>(std::get<std::vector<simql_types::guid_struct>>(bfr)[row_index]);
                    } else if constexpr (std::is_same_v<T, std::vector<simql_types::datetime_struct>>) {

                        return static_cast<simql_types::datetime_struct>(std::get<std::vector<simql_types::datetime_struct>>(bfr)[row_index]);
                    } else if constexpr (std::is_same_v<T, std::vector<simql_types::date_struct>>) {

                        return static_cast<simql_types::date_struct>(std::get<std::vector<simql_types::date_struct>>(bfr)[row_index]);
                    } else if constexpr (std::is_same_v<T, std::vector<simql_types::time_struct>>) {

                        return static_cast<simql_types::date_struct>(std::get<std::vector<simql_types::date_struct>>(bfr)[row_index]);
                    } else {
                        return simql_types::sql_val();
                    }
                };

                column.value = std::visit<simql_types::sql_val>(visitor, buffer);
            }

        };
        std::deque<column_binding_struct> column_bindings;

        // binding for parameters
        struct parameter_binding_struct {
        private:

            using buffer_variant = std::variant<
                std::monostate,                             // null
                std::basic_string<SQLCHAR>,                 // SQL_C_CHAR
                std::basic_string<SQLWCHAR>,                // SQL_C_WCHAR
                SQLCHAR,                                    // SQL_C_CHAR, SQL_C_STINYINT, SQL_C_BIT
                SQLWCHAR,                                   // SQL_C_WCHAR
                SQLDOUBLE,                                  // SQL_C_DOUBLE
                SQLREAL,                                    // SQL_C_FLOAT
                SQLSMALLINT,                                // SQL_C_SSHORT
                SQLINTEGER,                                 // SQL_C_SLONG
                SQLLEN,                                     // SQL_C_SBIGINT
                simql_types::guid_struct,                   // SQL_C_GUID
                simql_types::datetime_struct,               // SQL_C_TYPE_TIMESTAMP
                simql_types::date_struct,                   // SQL_C_TYPE_DATE
                simql_types::time_struct,                   // SQL_C_TYPE_TIME
                std::vector<std::uint8_t>                   // SQL_C_BINARY
            >;
            buffer_variant buffer;

        public:

            SQLSMALLINT                 binding_type;
            SQLSMALLINT                 c_data_type;
            SQLSMALLINT                 sql_data_type;
            SQLULEN                     column_size;
            SQLSMALLINT                 decimal_digits;
            SQLLEN                      buffer_length;
            SQLLEN                      indicator;
            statement::sql_parameter&   parameter;

            parameter_binding_struct(statement::sql_parameter_string& param) : parameter(param) {

                if (param.is_wide) {
                    c_data_type = SQL_C_WCHAR;
                    sql_data_type = param.variadic_characters ? SQL_WVARCHAR : SQL_WCHAR;
                    buffer = simql_strings::to_odbc_w(param.data());
                    column_size = param.data().size() + 1;
                    buffer_length = column_size * sizeof(SQLWCHAR);
                } else {
                    c_data_type = SQL_C_CHAR;
                    sql_data_type = param.variadic_characters ? SQL_VARCHAR : SQL_CHAR;
                    buffer = simql_strings::to_odbc_n(param.data());
                    column_size = param.data().size() + 1;
                    buffer_length = column_size * sizeof(SQLCHAR);
                }

                switch (param.binding_type) {
                case simql_types::parameter_binding_type::input_output:
                    binding_type = SQL_PARAM_INPUT_OUTPUT;
                    indicator = param.value.is_null() ? SQL_NULL_DATA : SQL_NTS;
                    break;
                case simql_types::parameter_binding_type::input:
                    binding_type = SQL_PARAM_INPUT;
                    indicator = param.value.is_null() ? SQL_NULL_DATA : SQL_NTS;
                    break;
                case simql_types::parameter_binding_type::output:
                    binding_type = SQL_PARAM_OUTPUT;
                    indicator = 0;
                    break;
                }

            }

            parameter_binding_struct(statement::sql_parameter_character& param) : parameter(param) {

                if (param.is_wide) {
                    c_data_type = SQL_C_WCHAR;
                    sql_data_type = SQL_WCHAR;
                    buffer = simql_strings::to_odbc_char_w(param.data());
                    column_size = 1;
                    buffer_length = sizeof(SQLWCHAR);
                } else {
                    c_data_type = SQL_C_CHAR;
                    sql_data_type = SQL_CHAR;
                    buffer = simql_strings::to_odbc_char_n(param.data());
                    column_size = 1;
                    buffer_length = sizeof(SQLCHAR);
                }

                switch (param.binding_type) {
                case simql_types::parameter_binding_type::input_output:
                    binding_type = SQL_PARAM_INPUT_OUTPUT;
                    indicator = param.value.is_null() ? SQL_NULL_DATA : SQL_NTS;
                    break;
                case simql_types::parameter_binding_type::input:
                    binding_type = SQL_PARAM_INPUT;
                    indicator = param.value.is_null() ? SQL_NULL_DATA : SQL_NTS;
                    break;
                case simql_types::parameter_binding_type::output:
                    binding_type = SQL_PARAM_OUTPUT;
                    indicator = 0;
                    break;
                }

            }

            parameter_binding_struct(statement::sql_parameter_boolean& param) : parameter(param) {

                c_data_type = SQL_C_BIT;
                sql_data_type = SQL_BIT;
                buffer = static_cast<SQLCHAR>(param.data());
                buffer_length = sizeof(SQLCHAR);

                switch (param.binding_type) {
                case simql_types::parameter_binding_type::input_output:
                    binding_type = SQL_PARAM_INPUT_OUTPUT;
                    indicator = param.value.is_null() ? SQL_NULL_DATA : 0;
                    break;
                case simql_types::parameter_binding_type::input:
                    binding_type = SQL_PARAM_INPUT;
                    indicator = param.value.is_null() ? SQL_NULL_DATA : 0;
                    break;
                case simql_types::parameter_binding_type::output:
                    binding_type = SQL_PARAM_OUTPUT;
                    indicator = 0;
                    break;
                }

            }

            parameter_binding_struct(statement::sql_parameter_double& param) : parameter(param) {

                c_data_type = SQL_C_DOUBLE;
                sql_data_type = SQL_DOUBLE;
                buffer = static_cast<SQLDOUBLE>(param.data());
                buffer_length = sizeof(SQLDOUBLE);

                switch (param.binding_type) {
                case simql_types::parameter_binding_type::input_output:
                    binding_type = SQL_PARAM_INPUT_OUTPUT;
                    indicator = param.value.is_null() ? SQL_NULL_DATA : 0;
                    break;
                case simql_types::parameter_binding_type::input:
                    binding_type = SQL_PARAM_INPUT;
                    indicator = param.value.is_null() ? SQL_NULL_DATA : 0;
                    break;
                case simql_types::parameter_binding_type::output:
                    binding_type = SQL_PARAM_OUTPUT;
                    indicator = 0;
                    break;
                }

            }

            parameter_binding_struct(statement::sql_parameter_float& param) : parameter(param) {

                c_data_type = SQL_C_FLOAT;
                sql_data_type = SQL_FLOAT;
                buffer = static_cast<SQLREAL>(param.data());
                buffer_length = sizeof(SQLREAL);

                switch (param.binding_type) {
                case simql_types::parameter_binding_type::input_output:
                    binding_type = SQL_PARAM_INPUT_OUTPUT;
                    indicator = param.value.is_null() ? SQL_NULL_DATA : 0;
                    break;
                case simql_types::parameter_binding_type::input:
                    binding_type = SQL_PARAM_INPUT;
                    indicator = param.value.is_null() ? SQL_NULL_DATA : 0;
                    break;
                case simql_types::parameter_binding_type::output:
                    binding_type = SQL_PARAM_OUTPUT;
                    indicator = 0;
                    break;
                }

            }

            parameter_binding_struct(statement::sql_parameter_int8& param) : parameter(param) {

                c_data_type = SQL_C_STINYINT;
                sql_data_type = SQL_TINYINT;
                buffer = static_cast<SQLCHAR>(param.data());
                buffer_length = sizeof(SQLCHAR);

                switch (param.binding_type) {
                case simql_types::parameter_binding_type::input_output:
                    binding_type = SQL_PARAM_INPUT_OUTPUT;
                    indicator = param.value.is_null() ? SQL_NULL_DATA : 0;
                    break;
                case simql_types::parameter_binding_type::input:
                    binding_type = SQL_PARAM_INPUT;
                    indicator = param.value.is_null() ? SQL_NULL_DATA : 0;
                    break;
                case simql_types::parameter_binding_type::output:
                    binding_type = SQL_PARAM_OUTPUT;
                    indicator = 0;
                    break;
                }

            }

            parameter_binding_struct(statement::sql_parameter_int16& param) : parameter(param) {

                c_data_type = SQL_C_SSHORT;
                sql_data_type = SQL_SMALLINT;
                buffer = static_cast<SQLSMALLINT>(param.data());
                buffer_length = sizeof(SQLSMALLINT);

                switch (param.binding_type) {
                case simql_types::parameter_binding_type::input_output:
                    binding_type = SQL_PARAM_INPUT_OUTPUT;
                    indicator = param.value.is_null() ? SQL_NULL_DATA : 0;
                    break;
                case simql_types::parameter_binding_type::input:
                    binding_type = SQL_PARAM_INPUT;
                    indicator = param.value.is_null() ? SQL_NULL_DATA : 0;
                    break;
                case simql_types::parameter_binding_type::output:
                    binding_type = SQL_PARAM_OUTPUT;
                    indicator = 0;
                    break;
                }

            }

            parameter_binding_struct(statement::sql_parameter_int32& param) : parameter(param) {

                c_data_type = SQL_C_SLONG;
                sql_data_type = SQL_INTEGER;
                buffer = static_cast<SQLINTEGER>(param.data());
                buffer_length = sizeof(SQLINTEGER);

                switch (param.binding_type) {
                case simql_types::parameter_binding_type::input_output:
                    binding_type = SQL_PARAM_INPUT_OUTPUT;
                    indicator = param.value.is_null() ? SQL_NULL_DATA : 0;
                    break;
                case simql_types::parameter_binding_type::input:
                    binding_type = SQL_PARAM_INPUT;
                    indicator = param.value.is_null() ? SQL_NULL_DATA : 0;
                    break;
                case simql_types::parameter_binding_type::output:
                    binding_type = SQL_PARAM_OUTPUT;
                    indicator = 0;
                    break;
                }

            }

            parameter_binding_struct(statement::sql_parameter_int64& param) : parameter(param) {

                c_data_type = SQL_C_SBIGINT;
                sql_data_type = SQL_BIGINT;
                buffer = static_cast<SQLLEN>(param.data());
                buffer_length = sizeof(SQLLEN);

                switch (param.binding_type) {
                case simql_types::parameter_binding_type::input_output:
                    binding_type = SQL_PARAM_INPUT_OUTPUT;
                    indicator = param.value.is_null() ? SQL_NULL_DATA : 0;
                    break;
                case simql_types::parameter_binding_type::input:
                    binding_type = SQL_PARAM_INPUT;
                    indicator = param.value.is_null() ? SQL_NULL_DATA : 0;
                    break;
                case simql_types::parameter_binding_type::output:
                    binding_type = SQL_PARAM_OUTPUT;
                    indicator = 0;
                    break;
                }

            }

            parameter_binding_struct(statement::sql_parameter_guid& param) : parameter(param) {

                c_data_type = SQL_C_GUID;
                sql_data_type = SQL_GUID;
                buffer = param.data();
                buffer_length = sizeof(simql_types::guid_struct);

                switch (param.binding_type) {
                case simql_types::parameter_binding_type::input_output:
                    binding_type = SQL_PARAM_INPUT_OUTPUT;
                    indicator = param.value.is_null() ? SQL_NULL_DATA : 0;
                    break;
                case simql_types::parameter_binding_type::input:
                    binding_type = SQL_PARAM_INPUT;
                    indicator = param.value.is_null() ? SQL_NULL_DATA : 0;
                    break;
                case simql_types::parameter_binding_type::output:
                    binding_type = SQL_PARAM_OUTPUT;
                    indicator = 0;
                    break;
                }

            }

            parameter_binding_struct(statement::sql_parameter_datetime& param) : parameter(param) {

                c_data_type = SQL_C_TYPE_TIMESTAMP;
                sql_data_type = SQL_TIMESTAMP;
                buffer = param.data();
                buffer_length = 29;
                decimal_digits = 9;

                switch (param.binding_type) {
                case simql_types::parameter_binding_type::input_output:
                    binding_type = SQL_PARAM_INPUT_OUTPUT;
                    indicator = param.value.is_null() ? SQL_NULL_DATA : 0;
                    break;
                case simql_types::parameter_binding_type::input:
                    binding_type = SQL_PARAM_INPUT;
                    indicator = param.value.is_null() ? SQL_NULL_DATA : 0;
                    break;
                case simql_types::parameter_binding_type::output:
                    binding_type = SQL_PARAM_OUTPUT;
                    indicator = 0;
                    break;
                }

            }

            parameter_binding_struct(statement::sql_parameter_date& param) : parameter(param) {

                c_data_type = SQL_C_TYPE_DATE;
                sql_data_type = SQL_DATE;
                buffer = param.data();
                buffer_length = 10;

                switch (param.binding_type) {
                case simql_types::parameter_binding_type::input_output:
                    binding_type = SQL_PARAM_INPUT_OUTPUT;
                    indicator = param.value.is_null() ? SQL_NULL_DATA : 0;
                    break;
                case simql_types::parameter_binding_type::input:
                    binding_type = SQL_PARAM_INPUT;
                    indicator = param.value.is_null() ? SQL_NULL_DATA : 0;
                    break;
                case simql_types::parameter_binding_type::output:
                    binding_type = SQL_PARAM_OUTPUT;
                    indicator = 0;
                    break;
                }

            }

            parameter_binding_struct(statement::sql_parameter_time& param) : parameter(param) {

                c_data_type = SQL_C_TYPE_TIME;
                sql_data_type = SQL_TIME;
                buffer = param.data();
                buffer_length = 10;

                switch (param.binding_type) {
                case simql_types::parameter_binding_type::input_output:
                    binding_type = SQL_PARAM_INPUT_OUTPUT;
                    indicator = param.value.is_null() ? SQL_NULL_DATA : 0;
                    break;
                case simql_types::parameter_binding_type::input:
                    binding_type = SQL_PARAM_INPUT;
                    indicator = param.value.is_null() ? SQL_NULL_DATA : 0;
                    break;
                case simql_types::parameter_binding_type::output:
                    binding_type = SQL_PARAM_OUTPUT;
                    indicator = 0;
                    break;
                }

            }

            parameter_binding_struct(statement::sql_parameter_blob& param) : parameter(param) {

                c_data_type = SQL_C_CHAR;
                sql_data_type = SQL_VARCHAR;
                buffer = param.data();
                column_size = param.data().size();
                buffer_length = column_size * sizeof(SQLCHAR);

                switch (param.binding_type) {
                case simql_types::parameter_binding_type::input_output:
                    binding_type = SQL_PARAM_INPUT_OUTPUT;
                    indicator = param.value.is_null() ? SQL_NULL_DATA : buffer_length;
                    break;
                case simql_types::parameter_binding_type::input:
                    binding_type = SQL_PARAM_INPUT;
                    indicator = param.value.is_null() ? SQL_NULL_DATA : buffer_length;
                    break;
                case simql_types::parameter_binding_type::output:
                    binding_type = SQL_PARAM_OUTPUT;
                    indicator = 0;
                    break;
                }

            }

            SQLPOINTER ptr() {
                auto visitor = [&](buffer_variant& bfr) -> SQLPOINTER {
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
                    } else if constexpr (std::is_same_v<T, std::vector<simql_types::guid_struct>>) {
                        return static_cast<SQLPOINTER>(std::get<std::vector<simql_types::guid_struct>>(bfr).data());
                    } else if constexpr (std::is_same_v<T, std::vector<simql_types::datetime_struct>>) {
                        return static_cast<SQLPOINTER>(std::get<std::vector<simql_types::datetime_struct>>(bfr).data());
                    } else if constexpr (std::is_same_v<T, std::vector<simql_types::date_struct>>) {
                        return static_cast<SQLPOINTER>(std::get<std::vector<simql_types::date_struct>>(bfr).data());
                    } else if constexpr (std::is_same_v<T, std::vector<simql_types::time_struct>>) {
                        return static_cast<SQLPOINTER>(std::get<std::vector<simql_types::time_struct>>(bfr).data());
                    } else {
                        return nullptr;
                    }
                };
                return std::visit<SQLPOINTER>(visitor, buffer);
            }

            void update(SQLUSMALLINT parameter_index) {

            }
        };
        std::map<std::string, parameter_binding_struct> parameter_bindings;

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
                break;
            case SQL_SUCCESS_WITH_INFO:
                diag.update(h_stmt, diagnostic_set::handle_type::stmt, std::string{"SQLExecute() -> SUCCESS_WITH_INFO"});
                break;
            case SQL_INVALID_HANDLE:
                last_error = std::string{"could not execute the prepared SQL: invalid handle"};
                diag.update(h_dbc, diagnostic_set::handle_type::dbc, std::string{"SQLExecute() -> INVALID_HANDLE"});
                return false;
            default:
                last_error = std::string{"could not execute the prepared SQL: generic error"};
                diag.update(h_stmt, diagnostic_set::handle_type::stmt, std::string{"SQLExecute() -> ERROR"});
                return false;
            }

            SQLSMALLINT column_count;
            if (!SQL_SUCCEEDED(SQLNumResultCols(h_stmt, &column_count))) {
                last_error = std::string{"could not determine if the SQL query returned one or more result sets"};
                return false;
            }

            if (column_count >= 1) {

                if (!bind_columns())
                    return false;

                if (cursor_is_scrollable) {
                    if (!fetch_first())
                        return false;

                } else {
                    if (!fetch_next())
                        return false;

                }
            }

            return true;
        }

        bool execute_direct(std::string_view sql) {
            std::basic_string<SQLWCHAR> w_sql = simql_strings::to_odbc_w(sql);
            switch (SQLExecDirectW(h_stmt, w_sql.data(), SQL_NTS)) {
            case SQL_SUCCESS:
                break;
            case SQL_SUCCESS_WITH_INFO:
                diag.update(h_stmt, diagnostic_set::handle_type::stmt, std::string{"SQLExecuteDirect() -> SUCCESS_WITH_INFO"});
                break;
            case SQL_INVALID_HANDLE:
                last_error = std::string{"could not execute the provided SQL: invalid handle"};
                diag.update(h_dbc, diagnostic_set::handle_type::dbc, std::string{"SQLExecuteDirect() -> INVALID_HANDLE"});
                return false;
            default:
                last_error = std::string{"could not execute the provided SQL: generic error"};
                diag.update(h_stmt, diagnostic_set::handle_type::stmt, std::string{"SQLExecuteDirect() -> ERROR"});
                return false;
            }

            SQLSMALLINT column_count;
            if (!SQL_SUCCEEDED(SQLNumResultCols(h_stmt, &column_count))) {
                last_error = std::string{"could not determine if the SQL query returned one or more result sets"};
                return false;
            }

            if (column_count >= 1) {

                if (!bind_columns())
                    return false;

                if (cursor_is_scrollable) {
                    if (!fetch_first())
                        return false;

                } else {
                    if (!fetch_next())
                        return false;

                }
            }

            return true;
        }

        // --------------------------------------------------
        // FILL DATA BUFFERS
        // --------------------------------------------------

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
            for (auto& binding : column_bindings) {
                binding.update(current_row_index);
            }

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
            for (auto& binding : column_bindings) {
                binding.update(current_row_index);
            }

            return true;
        }

        bool prev_record() {

            if (current_row_index - 1 > 0) {
                current_row_index--;
            } else {

                if (!cursor_is_scrollable) {
                    last_error = std::string{"cursor scrolling is disabled"};
                    return false;
                }

                if (!fetch_prev())
                    return false;

                current_row_index = rows_fetched - 1;
            }

            for (auto& binding : column_bindings) {
                binding.update(current_row_index);
            }

            return true;
        }

        bool next_record() {

            if (current_row_index + 1 < rows_fetched) {
                current_row_index++;
            } else {

                if (!fetch_next())
                    return false;

                current_row_index = 0;
            }

            for (auto& binding : column_bindings) {
                binding.update(current_row_index);
            }

            return true;
        }

        bool next_result_set() {
            column_bindings.clear();
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

        template<parameter_binding_type T>
        bool bind_parameter(T& param) {
            auto it = parameter_bindings.find(param.name);
            if (it != parameter_bindings.end()) {
                last_error = std::string{"cannot bind duplicate parameters"};
                return false;
            }

            parameter_binding_struct pb(param);
            switch (SQLBindParameter(h_stmt, 0, pb.binding_type, pb.c_data_type, pb.sql_data_type, pb.column_size, pb.decimal_digits, pb.ptr(), pb.buffer_length, &pb.indicator)) {
            case SQL_SUCCESS:
                break;
            case SQL_SUCCESS_WITH_INFO:
                diag.update(h_stmt, diagnostic_set::handle_type::stmt, std::format("SQLBindParameter()::{} -> SUCCESS_WITH_INFO", param.name));
                break;
            case SQL_INVALID_HANDLE:
                last_error = std::format("could not bind parameter::{} -> invalid handle", param.name);
                diag.update(h_dbc, diagnostic_set::handle_type::dbc, std::format("SQLBindParameter()::{} -> INVALID_HANDLE", param.name));
                return false;
            default:
                last_error = std::format("could not bind parameter::{} -> generic error", param.name);
                diag.update(h_stmt, diagnostic_set::handle_type::stmt, std::format("SQLBindParameter()::{} -> ERROR", param.name));
                return false;
            }
        }

        // --------------------------------------------------
        // COLUMN BINDING
        // --------------------------------------------------

        template<column_binding_type T>
        bool add_column(T& col) {

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

            if (rowset_size <= 1) {
                last_error = std::string{"invalid rowset size"};
                return false;
            }

            column_bindings.emplace_back(column_binding_struct(rowset_size, col));
        }

        bool bind_columns() {
            for (column_binding_struct& binding : column_bindings) {
                switch (SQLBindCol(h_stmt, binding.column.position + 1, binding.c_type, binding.ptr(), binding.buffer_length, binding.indicators.data())) {
                case SQL_SUCCESS:
                    break;
                case SQL_SUCCESS_WITH_INFO:
                    diag.update(h_stmt, diagnostic_set::handle_type::stmt, std::format("SQLBindCol()::{} -> SUCCESS_WITH_INFO", binding.column.name));
                    break;
                case SQL_INVALID_HANDLE:
                    last_error = std::format("could not bind column::{} -> invalid handle", binding.column.name);
                    diag.update(h_dbc, diagnostic_set::handle_type::dbc, std::format("SQLBindCol()::{} -> INVALID_HANDLE", binding.column.name));
                    return false;
                default:
                    last_error = std::format("could not bind column::{} -> generic error", binding.column.name);
                    diag.update(h_stmt, diagnostic_set::handle_type::stmt, std::format("SQLBindCol()::{} -> INVALID_HANDLE", binding.column.name));
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

    bool statement::prepare(std::string_view sql) {
        return !p_handle ? false : p_handle->prepare(sql);
    }

    bool statement::execute() {
        return p_handle ? p_handle->execute() : false;
    }

    bool statement::execute_direct(std::string_view sql) {
        return p_handle ? p_handle->execute_direct(sql) : false;
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

    std::int64_t statement::rows_affected() {
        return !p_handle ? -1 : p_handle->rows_affected();
    }

    // --------------------------------------------------
    // COLUMN BINDING
    // --------------------------------------------------

    template<typename T> requires std::derived_from<std::remove_cvref_t<T>, statement::sql_column>
    bool statement::bind_columns(T& column) {
        if (!p_handle)
            return false;

        return p_handle->add_column(column);
    }

    template<typename T, typename... args> requires std::derived_from<std::remove_cvref_t<T>, statement::sql_column> && (std::derived_from<std::remove_cvref_t<args>, statement::sql_column> && ...)
    bool statement::bind_columns(T& first_column, args&... other_columns) {
        if (!p_handle)
            return false;

        return p_handle->add_column(first_column) && bind_columns(other_columns);
    }

    // --------------------------------------------------
    // PARAMETER BINDING
    // --------------------------------------------------

    template<typename T> requires std::derived_from<std::remove_cvref_t<T>, statement::sql_parameter>
    bool statement::bind_parameters(T&& parameter) {
        if (!p_handle)
            return false;

        return p_handle->bind_parameter(parameter);
    }

    template<typename T, typename... args> requires std::derived_from<std::remove_cvref_t<T>, statement::sql_parameter> && (std::derived_from<std::remove_cvref_t<args>, statement::sql_parameter> && ...)
    bool statement::bind_parameters(T&& first_parameter, args&&... other_parameters) {
        if (!p_handle)
            return false;

        return p_handle->bind_parameter(std::move(first_parameter)) && bind_parameters(std::move(other_parameters));
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