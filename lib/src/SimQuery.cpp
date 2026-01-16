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

// helper functions

static SQLSMALLINT SQLDataType_to_ODBC_C(const SimpleSqlTypes::SQLDataType &data_type) {
    switch (data_type) {
    case SimpleSqlTypes::SQLDataType::CHAR;                     return SQL_C_CHAR;
    case SimpleSqlTypes::SQLDataType::VARCHAR;                  return SQL_C_CHAR;
    case SimpleSqlTypes::SQLDataType::LONG_VARCHAR;             return SQL_C_CHAR;
    case SimpleSqlTypes::SQLDataType::WCHAR;                    return SQL_C_WCHAR;
    case SimpleSqlTypes::SQLDataType::VARWCHAR;                 return SQL_C_WCHAR;
    case SimpleSqlTypes::SQLDataType::LONG_VARWCHAR;            return SQL_C_WCHAR;
    case SimpleSqlTypes::SQLDataType::DECIMAL;                  return SQL_C_DOUBLE;
    case SimpleSqlTypes::SQLDataType::NUMERIC;                  return SQL_C_NUMERIC;
    case SimpleSqlTypes::SQLDataType::SMALLINT;                 return SQL_C_SSHORT;
    case SimpleSqlTypes::SQLDataType::INTEGER;                  return SQL_C_SLONG;
    case SimpleSqlTypes::SQLDataType::REAL;                     return SQL_C_FLOAT;
    case SimpleSqlTypes::SQLDataType::FLOAT;                    return SQL_C_FLOAT;
    case SimpleSqlTypes::SQLDataType::DOUBLE;                   return SQL_C_DOUBLE;
    case SimpleSqlTypes::SQLDataType::BIT;                      return SQL_C_BIT;
    case SimpleSqlTypes::SQLDataType::TINYINT;                  return SQL_C_STINYINT;
    case SimpleSqlTypes::SQLDataType::BIGINT;                   return SQL_C_SBIGINT;
    case SimpleSqlTypes::SQLDataType::BINARY;                   return SQL_C_BINARY;
    case SimpleSqlTypes::SQLDataType::VARBINARY;                return SQL_C_BINARY;
    case SimpleSqlTypes::SQLDataType::LONG_VARBINARY;           return SQL_C_BINARY;
    case SimpleSqlTypes::SQLDataType::DATE;                     return SQL_C_TYPE_DATE;
    case SimpleSqlTypes::SQLDataType::TIME;                     return SQL_C_TYPE_TIME;
    case SimpleSqlTypes::SQLDataType::TIMESTAMP;                return SQL_C_TYPE_TIMESTAMP;
    case SimpleSqlTypes::SQLDataType::UTC_DATETIME;             return SQL_C_TYPE_TIMESTAMP;
    case SimpleSqlTypes::SQLDataType::UTC_TIME;                 return SQL_C_TYPE_TIMESTAMP;
    case SimpleSqlTypes::SQLDataType::INTERVAL_YEAR;            return SQL_C_INTERVAL_YEAR;
    case SimpleSqlTypes::SQLDataType::INTERVAL_MONTH;           return SQL_C_INTERVAL_MONTH;
    case SimpleSqlTypes::SQLDataType::INTERVAL_DAY;             return SQL_C_INTERVAL_DAY;
    case SimpleSqlTypes::SQLDataType::INTERVAL_HOUR;            return SQL_C_INTERVAL_HOUR;
    case SimpleSqlTypes::SQLDataType::INTERVAL_MINUTE;          return SQL_C_INTERVAL_MINUTE;
    case SimpleSqlTypes::SQLDataType::INTERVAL_SECOND;          return SQL_C_INTERVAL_SECOND;
    case SimpleSqlTypes::SQLDataType::INTERVAL_YEAR2MONTH;      return SQL_C_INTERVAL_YEAR_TO_MONTH;
    case SimpleSqlTypes::SQLDataType::INTERVAL_DAY2HOUR;        return SQL_C_INTERVAL_DAY_TO_HOUR;
    case SimpleSqlTypes::SQLDataType::INTERVAL_DAY2MINUTE;      return SQL_C_INTERVAL_DAY_TO_MINUTE;
    case SimpleSqlTypes::SQLDataType::INTERVAL_DAY2SECOND;      return SQL_C_INTERVAL_DAY_TO_SECOND;
    case SimpleSqlTypes::SQLDataType::INTERVAL_HOUR2MINUTE;     return SQL_C_INTERVAL_HOUR_TO_MINUTE;
    case SimpleSqlTypes::SQLDataType::INTERVAL_HOUR2SECOND;     return SQL_C_INTERVAL_HOUR_TO_SECOND;
    case SimpleSqlTypes::SQLDataType::INTERVAL_MINUTE2SECOND;   return SQL_C_INTERVAL_MINUTE_TO_SECOND;
    case SimpleSqlTypes::SQLDataType::GUID;                     return SQL_C_GUID;
    case SimpleSqlTypes::SQLDataType::UNKNOWN;                  return SQL_C_CHAR;
    }
}

static SQLSMALLINT SQLDataType_to_ODBC_SQL(const SimpleSqlTypes::SQLDataType &data_type) {
    switch (data_type) {
    case SimpleSqlTypes::SQLDataType::CHAR:                     return SQL_CHAR;
    case SimpleSqlTypes::SQLDataType::VARCHAR:                  return SQL_VARCHAR;
    case SimpleSqlTypes::SQLDataType::LONG_VARCHAR:             return SQL_LONGVARCHAR;
    case SimpleSqlTypes::SQLDataType::WCHAR:                    return SQL_WCHAR;
    case SimpleSqlTypes::SQLDataType::VARWCHAR:                 return SQL_WVARCHAR;
    case SimpleSqlTypes::SQLDataType::LONG_VARWCHAR:            return SQL_WLONGVARCHAR;
    case SimpleSqlTypes::SQLDataType::DECIMAL:                  return SQL_DECIMAL;
    case SimpleSqlTypes::SQLDataType::NUMERIC:                  return SQL_NUMERIC;
    case SimpleSqlTypes::SQLDataType::SMALLINT:                 return SQL_SMALLINT;
    case SimpleSqlTypes::SQLDataType::INTEGER:                  return SQL_INTEGER;
    case SimpleSqlTypes::SQLDataType::REAL:                     return SQL_REAL;
    case SimpleSqlTypes::SQLDataType::FLOAT:                    return SQL_FLOAT;
    case SimpleSqlTypes::SQLDataType::DOUBLE:                   return SQL_DOUBLE;
    case SimpleSqlTypes::SQLDataType::BIT:                      return SQL_BIT;
    case SimpleSqlTypes::SQLDataType::TINYINT:                  return SQL_TINYINT;
    case SimpleSqlTypes::SQLDataType::BIGINT:                   return SQL_BIGINT;
    case SimpleSqlTypes::SQLDataType::BINARY:                   return SQL_BINARY;
    case SimpleSqlTypes::SQLDataType::VARBINARY:                return SQL_VARBINARY;
    case SimpleSqlTypes::SQLDataType::LONG_VARBINARY:           return SQL_LONGVARBINARY;
    case SimpleSqlTypes::SQLDataType::DATE:                     return SQL_TYPE_DATE;
    case SimpleSqlTypes::SQLDataType::TIME:                     return SQL_TYPE_TIME;
    case SimpleSqlTypes::SQLDataType::TIMESTAMP:                return SQL_TYPE_TIMESTAMP;
    case SimpleSqlTypes::SQLDataType::UTC_DATETIME:             return SQL_TYPE_UTCDATETIME;
    case SimpleSqlTypes::SQLDataType::UTC_TIME:                 return SQL_TYPE_UTCTIME;
    case SimpleSqlTypes::SQLDataType::INTERVAL_YEAR:            return SQL_INTERVAL_YEAR;
    case SimpleSqlTypes::SQLDataType::INTERVAL_MONTH:           return SQL_INTERVAL_MONTH;
    case SimpleSqlTypes::SQLDataType::INTERVAL_DAY:             return SQL_INTERVAL_DAY;
    case SimpleSqlTypes::SQLDataType::INTERVAL_HOUR:            return SQL_INTERVAL_HOUR;
    case SimpleSqlTypes::SQLDataType::INTERVAL_MINUTE:          return SQL_INTERVAL_MINUTE;
    case SimpleSqlTypes::SQLDataType::INTERVAL_SECOND:          return SQL_INTERVAL_SECOND;
    case SimpleSqlTypes::SQLDataType::INTERVAL_YEAR2MONTH:      return SQL_INTERVAL_YEAR_TO_MONTH;
    case SimpleSqlTypes::SQLDataType::INTERVAL_DAY2HOUR:        return SQL_INTERVAL_DAY_TO_HOUR;
    case SimpleSqlTypes::SQLDataType::INTERVAL_DAY2MINUTE:      return SQL_INTERVAL_DAY_TO_MINUTE;
    case SimpleSqlTypes::SQLDataType::INTERVAL_DAY2SECOND:      return SQL_INTERVAL_DAY_TO_SECOND;
    case SimpleSqlTypes::SQLDataType::INTERVAL_HOUR2MINUTE:     return SQL_INTERVAL_HOUR_TO_MINUTE;
    case SimpleSqlTypes::SQLDataType::INTERVAL_HOUR2SECOND:     return SQL_INTERVAL_HOUR_TO_SECOND;
    case SimpleSqlTypes::SQLDataType::INTERVAL_MINUTE2SECOND:   return SQL_INTERVAL_MINUTE_TO_SECOND;
    case SimpleSqlTypes::SQLDataType::GUID:                     return SQL_GUID;
    default:                                                    return SQL_CHAR;
    }
}

static SQLSMALLINT CellDataType_to_ODBC_SQL(const SimpleSqlTypes::CellDataType &data_type) {
    switch (data_type) {
    case SimpleSqlTypes::CellDataType::BOOLEAN:         return SQL_BIT;
    case SimpleSqlTypes::CellDataType::INT_8:           return SQL_TINYINT;
    case SimpleSqlTypes::CellDataType::INT_16:          return SQL_SMALLINT;
    case SimpleSqlTypes::CellDataType::INT_32:          return SQL_INTEGER;
    case SimpleSqlTypes::CellDataType::INT_64:          return SQL_BIGINT;
    case SimpleSqlTypes::CellDataType::UINT_8:          return SQL_TINYINT;
    case SimpleSqlTypes::CellDataType::UINT_16:         return SQL_SMALLINT;
    case SimpleSqlTypes::CellDataType::UINT_32:         return SQL_INTEGER;
    case SimpleSqlTypes::CellDataType::UINT_64:         return SQL_BIGINT;
    case SimpleSqlTypes::CellDataType::NUM_32:          return SQL_FLOAT;
    case SimpleSqlTypes::CellDataType::NUM_64:          return SQL_DOUBLE;
    case SimpleSqlTypes::CellDataType::SMALL_STRING:    return SQL_VARCHAR;
    case SimpleSqlTypes::CellDataType::BIG_STRING:      return SQL_LONGVARCHAR;
    case SimpleSqlTypes::CellDataType::DATE:            return SQL_TYPE_DATE;
    case SimpleSqlTypes::CellDataType::TIME:            return SQL_TYPE_TIME;
    case SimpleSqlTypes::CellDataType::DATETIME:        return SQL_TYPE_TIMESTAMP;
    case SimpleSqlTypes::CellDataType::BLOB:            return SQL_LONGVARBINARY;
    default:                                            return SQL_CHAR;
    }
}

static SQLSMALLINT BindingType_to_ODBC_IOT(const SimpleSqlTypes::BindingType &binding_type) {
    switch (binding_type) {
    case SimpleSqlTypes::BindingType::INPUT_OUTPUT_STREAM:  return SQL_PARAM_INPUT_OUTPUT_STREAM;
    case SimpleSqlTypes::BindingType::INPUT_OUTPUT:         return SQL_PARAM_INPUT_OUTPUT;
    case SimpleSqlTypes::BindingType::INPUT:                return SQL_PARAM_INPUT;
    case SimpleSqlTypes::BindingType::OUTPUT:               return SQL_PARAM_OUTPUT;
    case SimpleSqlTypes::BindingType::OUTPUT_STREAM:        return SQL_PARAM_OUTPUT_STREAM;
    default:                                                return SQL_PARAM_INPUT;
    }
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
    default:                            return SimpleSqlTypes::NullRuleType::UNKNOWN;
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

static bool bind_value_input(void* handle, const SQLUSMALLINT &index) {
    
    SQLULEN column_size;
    SQLSMALLINT column_precision;
    switch (column_action) {
    0:
        if (!infer_column_metadata(handle, index, column_size, column_precision)) {
            column_size = sizeof()
        }
        break;
    1:
        
    2:
    }
    
    // bind SQL_CHAR
    // bind SQL_VARCHAR
    // bind SQL_LONGVARCHAR
    // bind SQL_WCHAR
    // bind SQL_WVARCHAR
    // bind SQL_WLONGVARCHAR
    // bind SQL_DECIMAL
    // bind SQL_NUMERIC
    // bind SQL_SMALLINT
    // bind SQL_INTEGER
    // bind SQL_REAL
    // bind SQL_FLOAT
    // bind SQL_DOUBLE
    // bind SQL_BIT
    // bind SQL_TINYINT
    // bind SQL_BIGINT
    // bind SQL_BINARY
    // bind SQL_VARBINARY
    // bind SQL_LONGVARBINARY
    // bind SQL_TYPE_DATE
    // bind SQL_TYPE_TIME
    // bind SQL_TYPE_TIMESTAMP
    // bind SQL_TYPE_UTCDATETIME
    // bind SQL_TYPE_UTCTIME
    // bind SQL_INTERVAL_YEAR
    // bind SQL_INTERVAL_MONTH
    // bind SQL_INTERVAL_DAY
    // bind SQL_INTERVAL_HOUR
    // bind SQL_INTERVAL_MINUTE
    // bind SQL_INTERVAL_SECOND
    // bind SQL_INTERVAL_YEAR_TO_MONTH
    // bind SQL_INTERVAL_DAY_TO_HOUR
    // bind SQL_INTERVAL_DAY_TO_MINUTE
    // bind SQL_INTERVAL_DAY_TO_SECOND
    // bind SQL_INTERVAL_HOUR_TO_MINUTE
    // bind SQL_INTERVAL_HOUR_TO_SECOND
    // bind SQL_INTERVAL_MINUTE_TO_SECOND
    // bind SQL_GUID
    // bind SQL_CHAR

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
    SQLSMALLINT binding_type = BindingType_to_ODBC_IOT(binding.binding_type);
    SQLSMALLINT value_type = SQLDataType_to_ODBC_C(binding.sql_data_type);
    SQLSMALLINT parameter_type = SQLDataType_to_ODBC_SQL(binding.sql_data_type);
    
    
    // try to infer column size
    uint8_t column_action = 0;
    
    // need column size
    column_action = parameter_type ^ SQL_CHAR &&
                    parameter_type ^ SQL_VARCHAR &&
                    parameter_type ^ SQL_LONGVARCHAR &&
                    parameter_type ^ SQL_WCHAR &&
                    parameter_type ^ SQL_WVARCHAR &&
                    parameter_type ^ SQL_WLONGVARCHAR &&
                    parameter_type ^ SQL_BINARY &&
                    parameter_type ^ SQL_VARBINARY &&
                    parameter_type ^ SQL_LONGVARBINARY &&
                    parameter_type ^ SQL_TYPE_DATE &&
                    parameter_type ^ SQL_TYPE_TIME &&
                    parameter_type ^ SQL_TYPE_TIMESTAMP &&
                    parameter_type ^ SQL_TYPE_UTCDATETIME &&
                    parameter_type ^ SQL_TYPE_UTCTIME &&
                    parameter_type ^ SQL_INTERVAL_YEAR &&
                    parameter_type ^ SQL_INTERVAL_MONTH &&
                    parameter_type ^ SQL_INTERVAL_DAY &&
                    parameter_type ^ SQL_INTERVAL_HOUR &&
                    parameter_type ^ SQL_INTERVAL_MINUTE &&
                    parameter_type ^ SQL_INTERVAL_SECOND &&
                    parameter_type ^ SQL_INTERVAL_YEAR_TO_MONTH &&
                    parameter_type ^ SQL_INTERVAL_DAY_TO_HOUR &&
                    parameter_type ^ SQL_INTERVAL_DAY_TO_MINUTE &&
                    parameter_type ^ SQL_INTERVAL_DAY_TO_SECOND &&
                    parameter_type ^ SQL_INTERVAL_HOUR_TO_MINUTE &&
                    parameter_type ^ SQL_INTERVAL_HOUR_TO_SECOND &&
                    parameter_type ^ SQL_INTERVAL_MINUTE_TO_SECOND ? 1 : column_action;
    
    // need column size and precision
    column_action = parameter_type ^ SQL_DECIMAL &&
                    parameter_type ^ SQL_NUMERIC &&
                    parameter_type ^ SQL_FLOAT &&
                    parameter_type ^ SQL_REAL &&
                    parameter_type ^ SQL_DOUBLE ? 2 : column_action;

    
                    
    
    
                            
                            
                            
                            
    
    
    
    SQLPOINTER parameter_buffer; // sizeof(parameter_buffer)
    SQLLEN indicator;
    
    /* for parameter_size...
    
    when the parameter is a null-terminated string, use SQL_NTS
    when the parameter is NULL, use SQL_NULL_DATA
    
    */
    
    
    /*
    
    SQLBindParameter() args...
    mp_stmt_handle.get()
    SQLUSMALLINT parameter index
    SQLSMALLINT binding type
    SQLSMALLINT ODBC C data type
    SQLSMALLINT ODBC SQL data type
    SQLSMALLINT column size ...
    
        if binding type is SQL_CHAR, SQL_VARCHAR, SQL_LONGVARCHAR, SQL_BINARY, SQL_VARBINARY, SQL_LONGVARBINARY
        then use sizeof(parameter_buffer)
        
        if binding type is SQL_DECIMAL, SQL_NUMERIC, SQL_FLOAT, SQL_REAL, SQL_DOUBLE
        
        
    
    */
    
    
    
    SQLRETURN return_code;
    switch (binding.type) {
    case SimpleSqlTypes::BindingType::NOT_SET:
        error = std::string("binding type not set");
        return false;
    case SimpleSqlTypes::BindingType::INPUT_OUTPUT_STREAM:
        
        break;
    case SimpleSqlTypes::BindingType::INPUT_OUTPUT:
        
        break;
    case SimpleSqlTypes::BindingType::INPUT:
        
        // binding a numeric value
        return_code = SQLBindParameter(
            mp_stmt_handle.get(),
            parameter_number,
            binding_type,
            value_type,
            parameter_type,
            column_size,
            precision,
            parameter_buffer,
            sizeof(parameter_buffer),
            nullptr
        );
        
        // binding a string value column_size = sizeof()
        
        break;
    case SimpleSqlTypes::BindingType::OUTPUT:
        
        break;
    case SimpleSqlTypes::BindingType::OUTPUT_STREAM:
        
        break;
    default:
        error = std::string("binding type is undefined");
        return false;
    }
    
    // when binding_type == INPUT .. 
    
    switch (SQLBindParameter(mp_stmt_handle.get(), parameter_number, binding_type, )) {
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