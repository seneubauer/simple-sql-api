// SimQL stuff
#include <SimQuery.h>

// STL stuff
#include <cstdint>
#include <memory>
#include <utility>

// ODBC stuff
#include <sql.h>
#include <sqltypes.h>
#include <sqlext.h>

// private enums

enum class BindingFamily {
    NOT_SET,
    NARROW_STRING,
    WIDE_STRING,
    INT_BOOL,
    NUMERIC,
    BINARY,
    GUID,
    DATETIME,
    INTERVAL
};

// helper functions

static bool get_odbc_data_types(const SimpleSqlTypes::SQLDataType &data_type, SQLSMALLINT &odbc_c_type, SQLSMALLINT &odbc_sql_type) {

    switch (data_type) {
    case NOT_SET:
        return false;
    case CHAR:
        odbc_c_type = SQL_C_CHAR;
        odbc_sql_type = SQL_CHAR;
        break;
    case VARCHAR:
        odbc_c_type = SQL_C_CHAR;
        odbc_sql_type = SQL_VARCHAR;
        break;
    case LONG_VARCHAR:
        odbc_c_type = SQL_C_CHAR;
        odbc_sql_type = SQL_LONGVARCHAR;
        break;
    case WCHAR:
        odbc_c_type = SQL_C_WCHAR;
        odbc_sql_type = SQL_CHAR;
        break;
    case VARWCHAR:
        odbc_c_type = SQL_C_WCHAR;
        odbc_sql_type = SQL_WVARCHAR;
        break;
    case LONG_VARWCHAR:
        odbc_c_type = SQL_C_WCHAR;
        odbc_sql_type = SQL_WLONGVARCHAR;
        break;
    case TINYINT:
        odbc_c_type = SQL_C_STINYINT;
        odbc_sql_type = SQL_TINYINT;
        break;
    case SMALLINT:
        odbc_c_type = SQL_C_SSHORT;
        odbc_sql_type = SQL_SMALLINT;
        break;
    case INTEGER:
        odbc_c_type = SQL_C_SLONG;
        odbc_sql_type = SQL_INTEGER;
        break;
    case BIGINT:
        odbc_c_type = SQL_SBIGINT;
        odbc_sql_type = SQL_BIGINT;
        break;
    case FLOAT:
        odbc_c_type = SQL_C_FLOAT;
        odbc_sql_type = SQL_FLOAT;
        break;
    case DOUBLE:
        odbc_c_type = SQL_C_DOUBLE;
        odbc_sql_type = SQL_DOUBLE;
        break;
    case BIT:
        odbc_c_type = SQL_C_BIT;
        odbc_sql_type = SQL_BIT;
        break;
    case BINARY:
        odbc_c_type = SQL_C_BINARY;
        odbc_sql_type = SQL_BINARY;
        break;
    case VARBINARY:
        odbc_c_type = SQL_C_BINARY;
        odbc_sql_type = SQL_VARBINARY;
        break;
    case LONG_VARBINARY:
        odbc_c_type = SQL_C_BINARY;
        odbc_sql_type = SQL_LONGVARBINARY;
        break;
    case GUID:
        odbc_c_type = SQL_C_GUID;
        odbc_sql_type = SQL_GUID;
        break;
    case DATE:
        odbc_c_type = SQL_C_TYPE_DATE;
        odbc_sql_type = SQL_TYPE_DATE;
        break;
    case TIME:
        odbc_c_type = SQL_C_TYPE_TIME;
        odbc_sql_type = SQL_TYPE_TIME;
        break;
    case TIMESTAMP:
        odbc_c_type = SQL_C_TYPE_TIMESTAMP;
        odbc_sql_type = SQL_TYPE_TIMESTAMP;
        break;
    case UTC_DATETIME:
        odbc_c_type = SQL_C_TYPE_TIMESTAMP;
        odbc_sql_type = SQL_TYPE_UTCDATETIME;
        break;
    case UTC_TIME:
        odbc_c_type = SQL_C_TYPE_TIME;
        odbc_sql_type = SQL_TYPE_UTCTIME;
        break;
    case INTERVAL_YEAR:
        odbc_c_type = SQL_C_INTERVAL_YEAR;
        odbc_sql_type = SQL_INTERVAL_YEAR;
        break;
    case INTERVAL_MONTH:
        odbc_c_type = SQL_C_INTERVAL_MONTH;
        odbc_sql_type = SQL_INTERVAL_MONTH;
        break;
    case INTERVAL_DAY:
        odbc_c_type = SQL_C_INTERVAL_DAY;
        odbc_sql_type = SQL_INTERVAL_DAY;
        break;
    case INTERVAL_HOUR:
        odbc_c_type = SQL_C_INTERVAL_HOUR;
        odbc_sql_type = SQL_INTERVAL_HOUR;
        break;
    case INTERVAL_MINUTE:
        odbc_c_type = SQL_C_INTERVAL_MINUTE;
        odbc_sql_type = SQL_INTERVAL_MINUTE;
        break;
    case INTERVAL_SECOND:
        odbc_c_type = SQL_C_INTERVAL_SECOND;
        odbc_sql_type = SQL_INTERVAL_SECOND;
        break;
    case INTERVAL_YEAR2MONTH:
        odbc_c_type = SQL_C_INTERVAL_YEAR_TO_MONTH;
        odbc_sql_type = SQL_INTERVAL_YEAR_TO_MONTH;
        break;
    case INTERVAL_DAY2HOUR:
        odbc_c_type = SQL_C_INTERVAL_DAY_TO_HOUR;
        odbc_sql_type = SQL_INTERVAL_DAY_TO_HOUR;
        break;
    case INTERVAL_DAY2MINUTE:
        odbc_c_type = SQL_C_INTERVAL_DAY_TO_MINUTE;
        odbc_sql_type = SQL_INTERVAL_DAY_TO_MINUTE;
        break;
    case INTERVAL_DAY2SECOND:
        odbc_c_type = SQL_C_INTERVAL_DAY_TO_SECOND;
        odbc_sql_type = SQL_INTERVAL_DAY_TO_SECOND;
        break;
    case INTERVAL_HOUR2MINUTE:
        odbc_c_type = SQL_C_INTERVAL_HOUR_TO_MINUTE;
        odbc_sql_type = SQL_INTERVAL_HOUR_TO_MINUTE;
        break;
    case INTERVAL_HOUR2SECOND:
        odbc_c_type = SQL_C_INTERVAL_HOUR_TO_SECOND;
        odbc_sql_type = SQL_INTERVAL_HOUR_TO_SECOND;
        break;
    case INTERVAL_MINUTE2SECOND:
        odbc_c_type = SQL_C_INTERVAL_MINUTE_TO_SECOND;
        odbc_sql_type = SQL_INTERVAL_MINUTE_TO_SECOND;
        break;
    }
    return true;
}

static bool get_binding_family(const SimpleSqlTypes::SQLDataType &data_type, BindingFamily &family) {

    family = BindingFamily::NOT_SET;

    // for narrow strings
    family = binding.sql_data_type ^ SQLDataType::CHAR &&
             binding.sql_data_type ^ SQLDataType::VARCHAR &&
             binding.sql_data_type ^ SQLDataType::LONG_VARCHAR ? BindingFamily::NARROW_STRING : family;

    // for wide strings
    family = binding.sql_data_type ^ SQLDataType::WCHAR &&
             binding.sql_data_type ^ SQLDataType::VARWCHAR &&
             binding.sql_data_type ^ SQLDataType::LONG_VARWCHAR ? BindingFamily::WIDE_STRING : family;

    // for integer / boolean types
    family = binding.sql_data_type ^ SQLDataType::TINYINT &&
             binding.sql_data_type ^ SQLDataType::SMALLINT &&
             binding.sql_data_type ^ SQLDataType::INTEGER &&
             binding.sql_data_type ^ SQLDataType::BIGINT &&
             binding.sql_data_type ^ SQLDataType::BIT ? BindingFamily::INT_BOOL : family;

    // for numeric types
    family = binding.sql_data_type ^ SQLDataType::FLOAT &&
             binding.sql_data_type ^ SQLDataType::DOUBLE ? BindingFamily::NUMERIC : family;

    // for binary types
    family = binding.sql_data_type ^ SQLDataType::BINARY &&
             binding.sql_data_type ^ SQLDataType::VARBINARY &&
             binding.sql_data_type ^ SQLDataType::LONG_VARBINARY ? BindingFamily::BINARY : family;

    // for GUID types
    family = binding.sql_data_type ^ SQLDataType::GUID ? BindingFamily::GUID : family;

    // for datetime types
    family = binding.sql_data_type ^ SQLDataType::DATE &&
             binding.sql_data_type ^ SQLDataType::TIME &&
             binding.sql_data_type ^ SQLDataType::TIMESTAMP &&
             binding.sql_data_type ^ SQLDataType::UTC_DATETIME &&
             binding.sql_data_type ^ SQLDataType::UTC_TIME ? BindingFamily::DATETIME : family;

    // for interval types
    family = binding.sql_data_type ^ SQLDataType::INTERVAL_YEAR &&
             binding.sql_data_type ^ SQLDataType::INTERVAL_MONTH &&
             binding.sql_data_type ^ SQLDataType::INTERVAL_DAY &&
             binding.sql_data_type ^ SQLDataType::INTERVAL_HOUR &&
             binding.sql_data_type ^ SQLDataType::INTERVAL_MINUTE &&
             binding.sql_data_type ^ SQLDataType::INTERVAL_SECOND &&
             binding.sql_data_type ^ SQLDataType::INTERVAL_YEAR2MONTH &&
             binding.sql_data_type ^ SQLDataType::INTERVAL_DAY2HOUR &&
             binding.sql_data_type ^ SQLDataType::INTERVAL_DAY2MINUTE &&
             binding.sql_data_type ^ SQLDataType::INTERVAL_DAY2SECOND &&
             binding.sql_data_type ^ SQLDataType::INTERVAL_HOUR2MINUTE &&
             binding.sql_data_type ^ SQLDataType::INTERVAL_HOUR2SECOND &&
             binding.sql_data_type ^ SQLDataType::INTERVAL_MINUTE2SECOND ? BindingFamily::INTERVAL : family;

    if (family == BindingFamily::NOT_SET)
        return false;

    return true;
}

static bool get_io_type(const SimpleSqlTypes::BindingType &binding_type, SQLSMALLINT &output) {
    switch (binding_type) {
    case SimpleSqlTypes::BindingType::INPUT_OUTPUT:         output = SQL_PARAM_INPUT_OUTPUT; break;
    case SimpleSqlTypes::BindingType::INPUT:                output = SQL_PARAM_INPUT; break;
    case SimpleSqlTypes::BindingType::OUTPUT:               output = SQL_PARAM_OUTPUT; break;
    default:                                                return false;
    }
    return true;
}

static SimpleSqlTypes::SQLDataType ODBC_SQL_to_SQLDataType(const SQLSMALLINT &data_type) {
    switch (data_type) {
    case SQL_CHAR:                      return SimpleSqlTypes::SQLDataType::CHAR;
    case SQL_VARCHAR:                   return SimpleSqlTypes::SQLDataType::VARCHAR;
    case SQL_LONGVARCHAR:               return SimpleSqlTypes::SQLDataType::LONG_VARCHAR;
    case SQL_WCHAR:                     return SimpleSqlTypes::SQLDataType::WCHAR;
    case SQL_WVARCHAR:                  return SimpleSqlTypes::SQLDataType::VARWCHAR;
    case SQL_WLONGVARCHAR:              return SimpleSqlTypes::SQLDataType::LONG_VARWCHAR;
    case SQL_DECIMAL:                   return SimpleSqlTypes::SQLDataType::DECIMAL;
    case SQL_NUMERIC:                   return SimpleSqlTypes::SQLDataType::NUMERIC;
    case SQL_SMALLINT:                  return SimpleSqlTypes::SQLDataType::SMALLINT;
    case SQL_INTEGER:                   return SimpleSqlTypes::SQLDataType::INTEGER;
    case SQL_REAL:                      return SimpleSqlTypes::SQLDataType::REAL;
    case SQL_FLOAT:                     return SimpleSqlTypes::SQLDataType::FLOAT;
    case SQL_DOUBLE:                    return SimpleSqlTypes::SQLDataType::DOUBLE;
    case SQL_BIT:                       return SimpleSqlTypes::SQLDataType::BIT;
    case SQL_TINYINT:                   return SimpleSqlTypes::SQLDataType::TINYINT;
    case SQL_BIGINT:                    return SimpleSqlTypes::SQLDataType::BIGINT;
    case SQL_BINARY:                    return SimpleSqlTypes::SQLDataType::BINARY;
    case SQL_VARBINARY:                 return SimpleSqlTypes::SQLDataType::VARBINARY;
    case SQL_LONGVARBINARY:             return SimpleSqlTypes::SQLDataType::LONG_VARBINARY;
    case SQL_TYPE_DATE:                 return SimpleSqlTypes::SQLDataType::DATE;
    case SQL_TYPE_TIME:                 return SimpleSqlTypes::SQLDataType::TIME;
    case SQL_TYPE_TIMESTAMP:            return SimpleSqlTypes::SQLDataType::TIMESTAMP;
    case SQL_TYPE_UTCDATETIME:          return SimpleSqlTypes::SQLDataType::UTC_DATETIME;
    case SQL_TYPE_UTCTIME:              return SimpleSqlTypes::SQLDataType::UTC_TIME;
    case SQL_INTERVAL_YEAR:             return SimpleSqlTypes::SQLDataType::INTERVAL_YEAR;
    case SQL_INTERVAL_MONTH:            return SimpleSqlTypes::SQLDataType::INTERVAL_MONTH;
    case SQL_INTERVAL_DAY:              return SimpleSqlTypes::SQLDataType::INTERVAL_DAY;
    case SQL_INTERVAL_HOUR:             return SimpleSqlTypes::SQLDataType::INTERVAL_HOUR;
    case SQL_INTERVAL_MINUTE:           return SimpleSqlTypes::SQLDataType::INTERVAL_MINUTE;
    case SQL_INTERVAL_SECOND:           return SimpleSqlTypes::SQLDataType::INTERVAL_SECOND;
    case SQL_INTERVAL_YEAR_TO_MONTH:    return SimpleSqlTypes::SQLDataType::INTERVAL_YEAR2MONTH;
    case SQL_INTERVAL_DAY_TO_HOUR:      return SimpleSqlTypes::SQLDataType::INTERVAL_DAY2HOUR;
    case SQL_INTERVAL_DAY_TO_MINUTE:    return SimpleSqlTypes::SQLDataType::INTERVAL_DAY2MINUTE;
    case SQL_INTERVAL_DAY_TO_SECOND:    return SimpleSqlTypes::SQLDataType::INTERVAL_DAY2SECOND;
    case SQL_INTERVAL_HOUR_TO_MINUTE:   return SimpleSqlTypes::SQLDataType::INTERVAL_HOUR2MINUTE;
    case SQL_INTERVAL_HOUR_TO_SECOND:   return SimpleSqlTypes::SQLDataType::INTERVAL_HOUR2SECOND;
    case SQL_INTERVAL_MINUTE_TO_SECOND: return SimpleSqlTypes::SQLDataType::INTERVAL_MINUTE2SECOND;
    case SQL_GUID:                      return SimpleSqlTypes::SQLDataType::GUID;
    default:                            return SimpleSqlTypes::SQLDataType::UNKNOWN;
    }
}

static SimpleSqlTypes::NullRuleType ODBC_NULL_to_NullRuleType(const SQLSMALLINT &null_type) {
    switch (null_type) {
    case SQL_NO_NULLS:                  return SimpleSqlTypes::NullRuleType::NOT_ALLOWED;
    case SQL_NULLABLE:                  return SimpleSqlTypes::NullRuleType::ALLOWED;
    case SQL_NULLABLE_UNKNOWN:          return SimpleSqlTypes::NullRuleType::UNKNOWN;
    default:                            return SimpleSqlTypes::NullRuleType::NOT_SET;
    }
}

static bool infer_column_metadata(void* handle, const SQLUSMALLINT &parameter_index, SQLULEN &size, SQLSMALLINT &precision) {
    SQLSMALLINT data_type;
    SQLSMALLINT null_rule;
    switch (SQLDescribeParam(handle, parameter_index, &data_type, &size, &precision, &null_rule)) {
    case SQL_SUCCESS:           return true;
    case SQL_SUCCESS_WITH_INFO: return true;
    case SQL_STILL_EXECUTING:   return false;
    case SQL_ERROR:             return false;
    case SQL_INVALID_HANDLE:    return false;
    case SQL_NO_DATA:           return false;
    case SQL_NEED_DATA:         return false;
    default:                    return false;
    }
}

static SQLRETURN bind_narrow_string(void* handle, const SQLUSMALLINT &index, const SQLSMALLINT &io_type, const SQLSMALLINT &odbc_c_type, const SQLSMALLINT &odbc_sql_type, const SimpleSqlTypes::SQLBinding &binding) {

    std::string str = std::get<std::string>(binding.data);

    // try to get the column metadata
    SQLULEN column_size; // column size or character count
    SQLSMALLINT precision; // unused, only here for compatability
    if (!infer_column_metadata(handle, index, column_size, precision))
        column_size = static_cast<SQLULEN>(str.size());

    SQLRETURN return_code;
    switch (io_type) {
    case SQL_PARAM_INPUT_OUTPUT:
        Output output;
        output.binding = binding;
        output.buffer_size = SQL_NTS;
        m_output_buffer.push_back(output);
        std::string& bound_str = std::get<std::string>(m_output_buffer.back().data);
        bound_str.resize(column_size);
        return_code = SQLBindParameter(
            handle,
            parameter_number,
            binding_type,
            odbc_c_type,
            odbc_sql_type,
            bound_str.size(),
            0,
            bound_str.data(),
            bound_str.size(),
            &m_output_buffer.back().buffer_size
        );
        break;
    case SQL_PARAM_INPUT:
        SQLLEN indicator = binding.set_null ? SQL_NULL_DATA : SQL_NTS;
        SQLLEN buffer_size = static_cast<SQLLEN>(str.size()); // count of bytes
        SQLCHAR* string_value = reinterpret_cast<SQLCHAR*>(const_cast<char*>(str.c_str()));
        return_code = SQLBindParameter(
            handle,
            parameter_number,
            binding_type,
            odbc_c_type,
            odbc_sql_type,
            column_size,
            0,
            (SQLPOINTER)string_value,
            buffer_size,
            &indicator
        );
        break;
    case SQL_PARAM_OUTPUT:
        Output output;
        output.binding = binding;
        output.buffer_size = 0;
        m_output_buffer.push_back(output);
        std::string& bound_str = std::get<std::string>(m_output_buffer.back().data);
        bound_str.resize(column_size);
        return_code = SQLBindParameter(
            handle,
            parameter_number,
            binding_type,
            odbc_c_type,
            odbc_sql_type,
            bound_str.size(),
            0,
            bound_str.data(),
            bound_str.size(),
            &m_output_buffer.back().buffer_size
        );
        break;
    }
    return return_code;
}

static SQLRETURN bind_wide_string(void* handle, const SQLUSMALLINT &index, const SQLSMALLINT &io_type, const SQLSMALLINT &odbc_c_type, const SQLSMALLINT &odbc_sql_type, const SimpleSqlTypes::SQLBinding &binding) {

    std::wstring str = std::get<std::wstring>(binding.data);

    // try to get the column metadata
    SQLULEN column_size; // column size or character count
    SQLSMALLINT precision; // unused, only here for compatability
    if (!infer_column_metadata(handle, index, column_size, precision))
        column_size = static_cast<SQLULEN>(str.size());

    SQLRETURN return_code;
    switch (io_type) {
    case SQL_PARAM_INPUT_OUTPUT:
        Output output;
        output.binding = binding;
        output.buffer_size = SQL_NTS;
        m_output_buffer.push_back(output);
        std::wstring& bound_str = std::get<std::wstring>(m_output_buffer.back().data);
        bound_str.resize(column_size);
        return_code = SQLBindParameter(
            handle,
            parameter_number,
            binding_type,
            odbc_c_type,
            odbc_sql_type,
            bound_str.size(),
            0,
            bound_str.data(),
            bound_str.size() * sizeof(SQLWCHAR),
            &m_output_buffer.back().buffer_size
        );
        break;
    case SQL_PARAM_INPUT:
        SQLLEN indicator = binding.set_null ? SQL_NULL_DATA : SQL_NTS;
        SQLLEN buffer_size = static_cast<SQLLEN>(str.size()) * sizeof(SQLWCHAR); // count of bytes
        SQLCHAR* string_value = reinterpret_cast<SQLCHAR*>(const_cast<char*>(str.c_str()));
        return_code = SQLBindParameter(
            handle,
            parameter_number,
            binding_type,
            odbc_c_type,
            odbc_sql_type,
            column_size,
            0,
            (SQLPOINTER)string_value,
            buffer_size,
            &indicator
        );
        break;
    case SQL_PARAM_OUTPUT:
        Output output;
        output.binding = binding;
        output.buffer_size = 0;
        m_output_buffer.push_back(output);
        std::wstring& bound_str = std::get<std::wstring>(m_output_buffer.back().data);
        bound_str.resize(column_size);
        return_code = SQLBindParameter(
            handle,
            parameter_number,
            binding_type,
            odbc_c_type,
            odbc_sql_type,
            bound_str.size(),
            0,
            bound_str.data(),
            bound_str.size() * sizeof(SQLWCHAR),
            &m_output_buffer.back().buffer_size
        );
        break;
    }
    return return_code;
}

static SQLRETURN bind_interval(void* handle, const SQLUSMALLINT &index, const SQLSMALLINT &io_type, const SQLSMALLINT &odbc_c_type, const SQLSMALLINT &odbc_sql_type, const SimpleSqlTypes::SQLBinding &binding) {

    SimpleSqlTypes::Interval& intv = std::get<SimpleSqlTypes::Interval>(binding.data);

    SQL_INTERVAL_STRUCT interval;
    interval.interval_sign = intv.interval_sign == SimpleSqlTypes::IntervalSign::POSITIVE ? SQL_FALSE : SQL_TRUE;
    switch (odbc_c_type) {
    case SQL_C_INTERVAL_YEAR:
        interval.interval_type = SQL_IS_YEAR;
        interval.year_month.year = binding_value;
        break;
    case SQL_C_INTERVAL_MONTH:
        
        break;
    case SQL_C_INTERVAL_DAY:
        
        break;
    case SQL_C_INTERVAL_HOUR:
        
        break;
    case SQL_C_INTERVAL_MINUTE:
        
        break;
    case SQL_C_INTERVAL_SECOND:
        
        break;
    case SQL_C_INTERVAL_YEAR_TO_MONTH:
        
        break;
    case SQL_C_INTERVAL_DAY_TO_HOUR:
        
        break;
    case SQL_C_INTERVAL_DAY_TO_MINUTE:
        
        break;
    case SQL_C_INTERVAL_DAY_TO_SECOND:
        
        break;
    case SQL_C_INTERVAL_HOUR_TO_MINUTE:
        
        break;
    case SQL_C_INTERVAL_HOUR_TO_SECOND:
        
        break;
    case SQL_C_INTERVAL_MINUTE_TO_SECOND:
        
        break;
    }

    int8_t binding_value = std::get<int8_t>(value);
    
    
    column_size = 0;
    
    
    SQLLEN indicator = sizeof(SQL_INTERVAL_STRUCT);
    
    
    
    SQLBindParameter(handle, index, SQL_PARAM_INPUT, c_data_type, sql_data_type, sizeof(SQL_INTERVAL_STRUCT), &indicator);
    

    SQLRETURN return_code;
    switch (io_type) {
    case SQL_PARAM_INPUT_OUTPUT:
        Output output;
        output.binding = binding;
        output.buffer_size = SQL_NTS;
        m_output_buffer.push_back(output);
        std::string& bound_str = std::get<std::string>(m_output_buffer.back().data);
        bound_str.resize(column_size);
        return_code = SQLBindParameter(
            handle,
            parameter_number,
            binding_type,
            odbc_c_type,
            odbc_sql_type,
            bound_str.size(),
            0,
            bound_str.data(),
            bound_str.size(),
            &m_output_buffer.back().buffer_size
        );
        break;
    case SQL_PARAM_INPUT:
        SQLLEN indicator = binding.set_null ? SQL_NULL_DATA : SQL_NTS;
        SQLLEN buffer_size = static_cast<SQLLEN>(str.size()); // count of bytes
        SQLCHAR* string_value = reinterpret_cast<SQLCHAR*>(const_cast<char*>(str.c_str()));
        return_code = SQLBindParameter(
            handle,
            parameter_number,
            binding_type,
            odbc_c_type,
            odbc_sql_type,
            column_size,
            0,
            (SQLPOINTER)string_value,
            buffer_size,
            &indicator
        );
        break;
    case SQL_PARAM_OUTPUT:
        Output output;
        output.binding = binding;
        output.buffer_size = 0;
        m_output_buffer.push_back(output);
        std::string& bound_str = std::get<std::string>(m_output_buffer.back().data);
        bound_str.resize(column_size);
        return_code = SQLBindParameter(
            handle,
            parameter_number,
            binding_type,
            odbc_c_type,
            odbc_sql_type,
            bound_str.size(),
            0,
            bound_str.data(),
            bound_str.size(),
            &m_output_buffer.back().buffer_size
        );
        break;
    }
    return return_code;
}

// class definition

bool SimpleSql::SimQuery::define_columns(std::string &error) {

    SQLSMALLINT column_count;
    switch (SQLNumResultCols(mp_stmt_handle.get(), &column_count)) {
    case SQL_SUCCESS:
        break;
    case SQL_SUCCESS_WITH_INFO:
        m_info_pending = true;
        define_diagnostics();
        break;
    case SQL_STILL_EXECUTING:
        error = std::string("SQLNumResultCols() is still executing");
        return false;
    case SQL_ERROR:
        error = std::string("could not get the resulting column count");
        define_diagnostics();
        return false;
    case SQL_INVALID_HANDLE:
        error = std::string("statement handle is invalid");
        return false;
    case SQL_NO_DATA:
        error = std::string("no data available");
        return false;
    case SQL_NEED_DATA:
        error = std::string("additional data is required");
        return false;
    }

    bool columns_defined = false;
    m_is_select = column_count == 0;
    m_is_select ? columns_defined = [&column_count, &error]() -> bool {
        for (SQLUSMALLINT i = 0; i < column_count; ++i) {
            SQLCHAR[m_max_column_name_length] column_name;
            SQLSMALLINT column_name_length;
            SQLSMALLINT data_type_id;
            SQLULEN data_size;
            SQLSMALLINT precision;
            SQLSMALLINT null_id;
            switch (SQLDescribeCol(mp_stmt_handle.get(), i, &column_name, sizeof(column_name), &column_name_length, &data_type_id, &data_size, &precision, &null_id)) {
            case SQL_SUCCESS:
                break;
            case SQL_SUCCESS_WITH_INFO:
                m_info_pending = true;
                define_diagnostics();
                break;
            default:
                define_diagnostics();
                error = std::string("could not fully define the selected column(s)");
                return false;
            }

            m_columns.push_back(SimpleSqlTypes::ColumnMetadata(
                    static_cast<uint16_t>(i),
                    std::string(reinterpret_cast<const char*>(column_name)),
                    ODBC_SQL_to_SQLDataType(data_type_id),
                    static_cast<uint64_t>(data_size),
                    static_cast<uint16_t>(precision),
                    ODBC_NULL_to_NullRuleType(null_id)
                )
            );
        }
    } : (void)0;
}

void SimpleSql::SimQuery::define_diagnostics() {
    if (reset)
        m_diagnostics.clear();

    SQLSMALLINT current_record_number = static_cast<SQLSMALLINT>(m_diagnostic_record_number);
    SQLCHAR sql_state[6];
    SQLINTEGER native_error;
    SQLCHAR message[SQL_MAX_MESSAGE_LENGTH];
    SQLSMALLINT message_length;
    SQLRETURN rc = SQLGetDiagRec(SQL_HANDLE_STMT, mp_stmt_handle.get(), current_record_number, &sql_state, &native_error, &message, sizeof(message), &message_length);
    while (rc != SQL_NO_DATA || rc != SQL_ERROR || rc != SQL_INVALID_HANDLE) {
        m_diagnostics.push_back(SimpleSqlTypes::DiagnosticRecord(
                static_cast<int16_t>(current_record_number),
                std::string(reinterpret_cast<const char*>(sql_state)),
                static_cast<int32_t>(native_error),
                std::string(reinterpret_cast<const char*>(message))
            )
        );
        current_record_number++;
    }
    m_diagnostic_record_number = current_record_number;
}

bool SimpleSql::SimQuery::claim_handle(std::unique_ptr<void> &&stmt_handle, std::string &error) {
    if (!stmt_handle) {
        error = std::string("the statement handle is a nullptr");
        return false;
    }
    mp_stmt_handle = std::move(stmt_handle);
    return true;
}

std::unique_ptr<void> SimpleSql::SimQuery::return_handle() {
    return mp_stmt_handle;
}

bool SimpleSql::SimQuery::set_sql(const std::string &sql, std::string &error) {
    if (sql.empty()) {
        error = std::string("the SQL statement is empty");
        return false;
    }
    m_sql = sql;
    return true;
}

bool SimpleSql::SimQuery::prepare(std::string &error) {

    SQLCHAR* sql = reinterpret_cast<SQLCHAR*>(const_cast<char*>(m_sql.c_str()));
    switch (SQLPrepare(mp_stmt_handle, sql, SQL_NTS)) {
    case SQL_SUCCESS:
        define_columns();
        return true;
    case SQL_SUCCESS_WITH_INFO:
        m_info_pending = true;
        define_diagnostics();
        define_columns();
        return true;
    case SQL_STILL_EXECUTING:
        error = std::string("SQLPrepare() is still executing");
        return false;
    case SQL_ERROR:
        error = std::string("could not prepare the SQL query");
        define_diagnostics();
        return false;
    case SQL_INVALID_HANDLE:
        error = std::string("statement handle is invalid");
        return false;
    case SQL_NO_DATA:
        error = std::string("no data available");
        return false;
    case SQL_NEED_DATA:
        error = std::string("additional data is required");
        return false;
    }
}

bool SimpleSql::SimQuery::bind_parameter(const SimpleSqlTypes::SQLBinding &binding, std::string &error) {

    // use SQLNumParams to ensure there are parameters in the SQL statement

    SQLUSMALLINT parameter_number = static_cast<SQLUSMALLINT>(m_binding_index);

    SQLSMALLINT io_type;
    if (!get_io_type(binding.binding_type, io_type))
        return false; // could not determine the ODBC Input-Output type

    BindingFamily family;
    if (!get_binding_family(binding.sql_data_type, family))
        return false; // could not determine a proper binding family

    SQLSMALLINT c_data_type, sql_data_type;
    if (!get_odbc_data_types(binding.sql_data_type, c_data_type, sql_data_type))
        return false; // could not determine the ODBC C & SQL data types

    SQLRETURN return_code;
    switch (family) {
    case BindingFamily::NARROW_STRING:
        return_code = bind_narrow_string(mp_stmt_handle.get(), parameter_number, io_type, c_data_type, sql_data_type, binding.data);
        break;
    case BindingFamily::WIDE_STRING:
        return_code = bind_narrow_string(mp_stmt_handle.get(), parameter_number, io_type, c_data_type, sql_data_type, binding.data);
        break;
    case BindingFamily::INT_BOOL:
        
        break;
    case BindingFamily::NUMERIC:
        
        break;
    case BindingFamily::BINARY:
        
        break;
    case BindingFamily::GUID:
        
        break;
    case BindingFamily::DATETIME:
        
        break;
    case BindingFamily::INTERVAL:
        return_code = bind_interval(mp_stmt_handle.get(), parameter_number, io_type, c_data_type, sql_data_type, binding.data);
        break;
    }

    switch (return_code) {
    case SQL_SUCCESS:
        break;
    case SQL_SUCCESS_WITH_INFO:
        m_info_pending = true;
        define_diagnostics();
        break;
    case SQL_STILL_EXECUTING:
        error = std::string("SQLBindParameter() is still executing");
        return false;
    case SQL_ERROR:
        error = std::string("could not bind the provided parameter");
        return false;
    case SQL_INVALID_HANDLE:
        error = std::string("statement handle is invalid");
        return false;
    case SQL_NO_DATA:
        error = std::string("no data available");
        return false;
    case SQL_NEED_DATA:
        error = std::string("additional data is required");
        return false;
    }
    m_binding_index++;
    return true;
}

bool SimpleSql::SimQuery::bulk_bind_parameters(const std::vector<SimpleSqlTypes::SQLBinding> &bindings, std::string &error) {
    
    
    return true;
}

SimpleSqlTypes::Cell SimpleSql::SimQuery::get_cell(const std::string &key) {

}

SimpleSqlTypes::Cell SimpleSql::SimQuery::get_cell(const std::string &column_name, const int &row) {

}

SimpleSqlTypes::Cell SimpleSql::SimQuery::get_cell(const int &column_index, const int &row) {

}

std::vector<SimpleSqlTypes::Cell> SimpleSql::SimQuery::claim_data() {

}

uint64_t SimpleSql::SimQuery::get_rowcount() {

}

uint8_t SimpleSql::SimQuery::get_columncount() {

}

void SimpleSql::SimQuery::destroy() {

}