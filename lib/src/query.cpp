#include <query.h>
#include <sql.h>
#include <sqltypes.h>
#include <sqlext.h>
#include <cstdint>

bool SimpleSql::query::define_columns(std::string &error) {

    SimpleSqlTypes::SQLDataType translate_data_type = [&](SQLSMALLINT i) {
        switch (i) {
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

    SimpleSqlTypes::NullRuleType translate_null_type = [&](SQLSMALLINT i) {
        switch (i) {
        case SQL_NO_NULLS:                  return SimpleSqlTypes::NullRuleType::NOT_ALLOWED;
        case SQL_NULLABLE:                  return SimpleSqlTypes::NullRuleType::ALLOWED;
        case SQL_NULLABLE_UNKNOWN:          return SimpleSqlTypes::NullRuleType::UNKNOWN;
        default:                            return SimpleSqlTypes::NullRuleType::UNKNOWN;
        }
    }

    SQLRETURN rc;
    SQLSMALLINT column_count;
    rc = SQLNumResultCols(mp_stmt_handle, &column_count);
    if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO) {
        define_diagnostics(false);
        error = std::string("could not get the resulting column count");
        return false;
    }

    bool columns_defined = false;
    m_is_select = column_count == 0;
    m_is_select ? columns_defined = [&column_count, &error]() -> bool {
        int16_t rc;
        for (SQLUSMALLINT i = 0; i < column_count; ++i) {

            SQLCHAR[m_max_column_name_length] column_name;
            SQLSMALLINT column_name_length;
            SQLSMALLINT data_type_id;
            SQLULEN data_size;
            SQLSMALLINT precision;
            SQLSMALLINT null_id;
            rc = SQLDescribeCol(mp_stmt_handle, i, &column_name, sizeof(column_name), &column_name_length, &data_type_id, &data_size, &precision, &null_id);
            if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO) {
                define_diagnostics(false);
                error = std::string("could not fully define the selected column(s)");
                return false;
            }

            m_columns.push_back(SimpleSqlTypes::ColumnMetadata(
                    static_cast<uint16_t>(i),
                    std::string(reinterpret_cast<const char*>(column_name)),
                    translate_data_type(data_type_id),
                    static_cast<uint64_t>(data_size),
                    static_cast<uint16_t>(precision),
                    translate_null_type(null_id)
                )
            );
        }
    } : (void)0;
}

void SimpleSql::query::define_diagnostics(const bool &reset) {
    if (reset)
        m_diagnostics.clear();

    SQLSMALLINT current_record_number = 1;
    SQLCHAR sql_state[6];
    SQLINTEGER native_error;
    SQLCHAR message[SQL_MAX_MESSAGE_LENGTH];
    SQLSMALLINT message_length;
    SQLRETURN rc = SQLGetDiagRec(SQL_HANDLE_STMT, mp_stmt_handle, current_record_number, &sql_state, &native_error, &message, sizeof(message), &message_length);
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
}

void SimpleSql::query::set_handle(void* stmt_handle) {
    mp_stmt_handle = stmthandle;
}

void SimpleSql::query::set_sql(const std::string &sql) {
    m_sql = sql;
}

bool SimpleSql::query::prepare(std::string &error) {

    SQLCHAR* sql = reinterpret_cast<SQLCHAR*>(const_cast<char*>(m_sql.c_str()));

    // prepare here
    SQLRETURN rc = SQLPrepare(mp_stmt_handle, sql, SQL_NTS);
    switch (rc) {
    case SQL_SUCCESS:
        define_columns();
        return true;
    case SQL_SUCCESS_WITH_INFO:
        define_diagnostics(true);
        define_columns();
    return true;
    case SQL_STILL_EXECUTING:
        error = std::string("SQLPrepare() is still executing");
        return false;
    case SQL_ERROR:
        error = std::string("could not prepare the SQL query");
        define_diagnostics(true);
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

SimpleSqlTypes::Cell SimpleSql::query::get_cell(const std::string &key) {

}

SimpleSqlTypes::Cell SimpleSql::query::get_cell(const std::string &column_name, const int &row) {

}

SimpleSqlTypes::Cell SimpleSql::query::get_cell(const int &column_index, const int &row) {

}

std::vector<SimpleSqlTypes::Cell> SimpleSql::query::claim_data() {

}

uint64_t SimpleSql::query::get_rowcount() {

}

uint8_t SimpleSql::query::get_columncount() {

}

void SimpleSql::query::destroy() {

}