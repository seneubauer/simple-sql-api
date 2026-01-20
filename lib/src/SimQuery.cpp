// SimQL stuff
#include <SimQuery.h>
#include <SimQL_Types.h>

// STL stuff
#include <cstdint>
#include <memory>
#include <utility>
#include <vector>
#include <unordered_map>
#include <span>

// ODBC stuff
#include <sql.h>
#include <sqltypes.h>
#include <sqlext.h>

// private enums

enum class BindingFamily : uint8_t {
    NOT_SET         = 0,
    STRING_UTF8     = 1,
    STRING_UTF16    = 2,
    NUMERIC         = 3,
    BOOL_INT        = 4,
    GUID            = 5,
    DATETIME        = 6,
    BLOB            = 7
};

// helper functions

static bool get_odbc_data_types(const SimpleSqlTypes::SimDataType &data_type, SQLSMALLINT &odbc_c_type, SQLSMALLINT &odbc_sql_type) {

    switch (data_type) {
    case SimpleSqlTypes::SimDataType::STRING_UTF8:
        odbc_c_type = SQL_C_CHAR;
        odbc_sql_type = SQL_VARCHAR;
        break;
    case SimpleSqlTypes::SimDataType::STRING_UTF16:
        odbc_c_type = SQL_C_WCHAR;
        odbc_sql_type = SQL_WVARCHAR;
        break;
    case SimpleSqlTypes::SimDataType::LONG_STRING_UTF8:
        odbc_c_type = SQL_C_CHAR;
        odbc_sql_type = SQL_LONGVARCHAR;
        break;
    case SimpleSqlTypes::SimDataType::LONG_STRING_UTF16:
        odbc_c_type = SQL_C_WCHAR;
        odbc_sql_type = SQL_WLONGVARCHAR;
        break;
    case SimpleSqlTypes::SimDataType::FLOAT:
        odbc_c_type = SQL_C_FLOAT;
        odbc_sql_type = SQL_FLOAT;
        break;
    case SimpleSqlTypes::SimDataType::DOUBLE:
        odbc_c_type = SQL_C_DOUBLE;
        odbc_sql_type = SQL_DOUBLE;
        break;
    case SimpleSqlTypes::SimDataType::BOOLEAN:
        odbc_c_type = SQL_C_BIT;
        odbc_sql_type = SQL_BIT;
        break;
    case SimpleSqlTypes::SimDataType::INT_8:
        odbc_c_type = SQL_C_STINYINT;
        odbc_sql_type = SQL_TINYINT;
        break;
    case SimpleSqlTypes::SimDataType::INT_16:
        odbc_c_type = SQL_C_SSHORT;
        odbc_sql_type = SQL_SMALLINT;
        break;
    case SimpleSqlTypes::SimDataType::INT_32:
        odbc_c_type = SQL_C_SLONG;
        odbc_sql_type = SQL_INTEGER;
        break;
    case SimpleSqlTypes::SimDataType::INT_64:
        odbc_c_type = SQL_C_SBIGINT;
        odbc_sql_type = SQL_BIGINT;
        break;
    case SimpleSqlTypes::SimDataType::GUID:
        odbc_c_type = SQL_C_GUID;
        odbc_sql_type = SQL_GUID;
        break;
    case SimpleSqlTypes::SimDataType::DATETIME:
        odbc_c_type = SQL_C_TYPE_TIMESTAMP;
        odbc_sql_type = SQL_TYPE_TIMESTAMP;
        break;
    case SimpleSqlTypes::SimDataType::DATE:
        odbc_c_type = SQL_C_TYPE_DATE;
        odbc_sql_type = SQL_TYPE_DATE;
        break;
    case SimpleSqlTypes::SimDataType::TIME:
        odbc_c_type = SQL_C_TYPE_TIME;
        odbc_sql_type = SQL_TYPE_TIME;
        break;
    case SimpleSqlTypes::SimDataType::BLOB:
        odbc_c_type = SQL_C_BINARY;
        odbc_sql_type = SQL_VARBINARY;
        break;
    case SimpleSqlTypes::SimDataType::LONG_BLOB:
        odbc_c_type = SQL_C_BINARY;
        odbc_sql_type = SQL_LONGVARBINARY;
        break;
    default:
        return false;
    }
    return true;
}

static bool get_binding_family(const SimpleSqlTypes::SimDataType &data_type, BindingFamily &family) {

    family = BindingFamily::NOT_SET;

    // for narrow strings
    family = data_type ^ SimpleSqlTypes::SimDataType::STRING_UTF8 &&
             data_type ^ SimpleSqlTypes::SimDataType::LONG_STRING_UTF8 ? BindingFamily::STRING_UTF8 : family;

    // for wide strings
    family = data_type ^ SimpleSqlTypes::SimDataType::STRING_UTF16 &&
             data_type ^ SimpleSqlTypes::SimDataType::LONG_STRING_UTF16 ? BindingFamily::STRING_UTF16 : family;

    // for numeric types
    family = data_type ^ SimpleSqlTypes::SimDataType::FLOAT &&
             data_type ^ SimpleSqlTypes::SimDataType::DOUBLE ? BindingFamily::NUMERIC : family;

    // for integer / boolean types
    family = data_type ^ SimpleSqlTypes::SimDataType::BOOLEAN &&
             data_type ^ SimpleSqlTypes::SimDataType::INT_8 &&
             data_type ^ SimpleSqlTypes::SimDataType::INT_16 &&
             data_type ^ SimpleSqlTypes::SimDataType::INT_32 &&
             data_type ^ SimpleSqlTypes::SimDataType::INT_64 ? BindingFamily::BOOL_INT : family;

    // for GUID types
    family = data_type ^ SimpleSqlTypes::SimDataType::GUID ? BindingFamily::GUID : family;

    // for datetime types
    family = data_type ^ SimpleSqlTypes::SimDataType::DATETIME &&
             data_type ^ SimpleSqlTypes::SimDataType::DATE &&
             data_type ^ SimpleSqlTypes::SimDataType::TIME ? BindingFamily::DATETIME : family;

    // for binary types
    family = data_type ^ SimpleSqlTypes::SimDataType::BLOB &&
             data_type ^ SimpleSqlTypes::SimDataType::LONG_BLOB ? BindingFamily::BLOB : family;

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

static SimpleSqlTypes::SimDataType ODBC_SQL_to_SQLDataType(const SQLSMALLINT &data_type) {
    switch (data_type) {
    case SQL_CHAR:                      return SimpleSqlTypes::SimDataType::STRING_UTF8;
    case SQL_VARCHAR:                   return SimpleSqlTypes::SimDataType::STRING_UTF8;
    case SQL_LONGVARCHAR:               return SimpleSqlTypes::SimDataType::LONG_STRING_UTF8;
    case SQL_WCHAR:                     return SimpleSqlTypes::SimDataType::STRING_UTF16;
    case SQL_WVARCHAR:                  return SimpleSqlTypes::SimDataType::STRING_UTF16;
    case SQL_WLONGVARCHAR:              return SimpleSqlTypes::SimDataType::LONG_STRING_UTF16;
    case SQL_FLOAT:                     return SimpleSqlTypes::SimDataType::FLOAT;
    case SQL_DOUBLE:                    return SimpleSqlTypes::SimDataType::DOUBLE;
    case SQL_BIT:                       return SimpleSqlTypes::SimDataType::BOOLEAN;
    case SQL_TINYINT:                   return SimpleSqlTypes::SimDataType::INT_8;
    case SQL_SMALLINT:                  return SimpleSqlTypes::SimDataType::INT_16;
    case SQL_INTEGER:                   return SimpleSqlTypes::SimDataType::INT_32;
    case SQL_BIGINT:                    return SimpleSqlTypes::SimDataType::INT_64;
    case SQL_GUID:                      return SimpleSqlTypes::SimDataType::GUID;
    case SQL_TYPE_TIMESTAMP:            return SimpleSqlTypes::SimDataType::DATETIME;
    case SQL_TYPE_DATE:                 return SimpleSqlTypes::SimDataType::DATE;
    case SQL_TYPE_TIME:                 return SimpleSqlTypes::SimDataType::TIME;
    case SQL_BINARY:                    return SimpleSqlTypes::SimDataType::BLOB;
    case SQL_VARBINARY:                 return SimpleSqlTypes::SimDataType::BLOB;
    case SQL_LONGVARBINARY:             return SimpleSqlTypes::SimDataType::LONG_BLOB;
    default:                            return SimpleSqlTypes::SimDataType::UNKNOWN;
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

static SQLRETURN bind_string_utf8(void* handle, const SQLUSMALLINT &index, const SQLSMALLINT &io_type, const SQLSMALLINT &odbc_c_type, const SQLSMALLINT &odbc_sql_type, const SimpleSqlTypes::SQLBinding &binding, std::vector<SimpleSqlTypes::SQLBoundOutput> &output_buffer) {

    SQLULEN column_size;
    SQLSMALLINT precision;
    SQLRETURN return_code;
    switch (io_type) {
    case SQL_PARAM_INPUT_OUTPUT:

        // create the output object and store it
        output_buffer.push_back(SimpleSqlTypes::SQLBoundOutput(binding, SQL_NTS));

        // get the value
        std::string& str = std::get<std::string&>(output_buffer.back().binding().data());

        // infer column metadata
        if (!infer_column_metadata(handle, index, column_size, precision))
            column_size = static_cast<SQLULEN>(str.size()) + 1;

        // ensure the value buffer is large enough
        str.resize(column_size);

        // define specific parameters
        SQLLEN input_buffer_size = static_cast<SQLLEN>(str.size()) + 1;

        // finally bind the parameter
        return_code = SQLBindParameter(
            handle,
            index,
            io_type,
            odbc_c_type,
            odbc_sql_type,
            column_size,
            0,
            (SQLPOINTER)str.c_str(),
            input_buffer_size,
            &output_buffer.back().buffer_size()
        );
        break;
    case SQL_PARAM_INPUT:

        // get the value
        std::string& str = std::get<std::string&>(binding.data());

        // infer column metadata
        if (!infer_column_metadata(handle, index, column_size, precision))
            column_size = static_cast<SQLULEN>(str.size()) + 1;

        // define specific parameters
        SQLLEN indicator = binding.set_null() ? SQL_NULL_DATA : SQL_NTS;
        SQLLEN input_buffer_size = static_cast<SQLLEN>(str.size()) + 1;

        // finally bind the parameter
        return_code = SQLBindParameter(
            handle,
            index,
            io_type,
            odbc_c_type,
            odbc_sql_type,
            column_size,
            0,
            (SQLPOINTER)str.c_str(),
            input_buffer_size,
            &indicator
        );
        break;
    case SQL_PARAM_OUTPUT:

        // create the output object and store it
        output_buffer.push_back(SimpleSqlTypes::SQLBoundOutput(binding, 0));

        // get the value
        std::string& str = std::get<std::string&>(output_buffer.back().binding().data());

        // infer column metadata
        if (!infer_column_metadata(handle, index, column_size, precision))
            column_size = static_cast<SQLULEN>(str.size()) + 1;

        // ensure the value buffer is large enough
        str.resize(column_size);

        // define specific parameters
        SQLLEN input_buffer_size = static_cast<SQLLEN>(str.size()) + 1;

        // finally bind the parameter
        return_code = SQLBindParameter(
            handle,
            index,
            io_type,
            odbc_c_type,
            odbc_sql_type,
            column_size,
            0,
            (SQLPOINTER)str.c_str(),
            input_buffer_size,
            &output_buffer.back().buffer_size()
        );
        break;
    }
    return return_code;
}

static SQLRETURN bind_string_utf16(void* handle, const SQLUSMALLINT &index, const SQLSMALLINT &io_type, const SQLSMALLINT &odbc_c_type, const SQLSMALLINT &odbc_sql_type, const SimpleSqlTypes::SQLBinding &binding, std::vector<SimpleSqlTypes::SQLBoundOutput> &output_buffer) {

    SQLULEN column_size;
    SQLSMALLINT precision;
    SQLRETURN return_code;
    switch (io_type) {
    case SQL_PARAM_INPUT_OUTPUT:

        // create the output object and store it
        output_buffer.push_back(SimpleSqlTypes::SQLBoundOutput(binding, SQL_NTS));

        // get the value
        std::wstring& str = std::get<std::wstring&>(output_buffer.back().binding().data());

        // infer column metadata
        if (!infer_column_metadata(handle, index, column_size, precision))
            column_size = static_cast<SQLULEN>(str.size()) + 1;

        // ensure the value buffer is large enough (plus null-terminator)
        str.resize(column_size);

        // define specific parameters
        SQLLEN input_buffer_size = static_cast<SQLLEN>(str.size() * sizeof(SQLWCHAR)) + 1;

        // finally bind the parameter
        return_code = SQLBindParameter(
            handle,
            index,
            io_type,
            odbc_c_type,
            odbc_sql_type,
            column_size,
            0,
            (SQLPOINTER)str.c_str(),
            input_buffer_size,
            &output_buffer.back().buffer_size()
        );
        break;
    case SQL_PARAM_INPUT:

        // get the value
        std::wstring& str = std::get<std::wstring&>(binding.data());

        // infer column metadata
        if (!infer_column_metadata(handle, index, column_size, precision))
            column_size = static_cast<SQLULEN>(str.size()) + 1;

        // define specific parameters
        SQLLEN indicator = binding.set_null() ? SQL_NULL_DATA : SQL_NTS;
        SQLLEN input_buffer_size = static_cast<SQLLEN>(str.size() * sizeof(SQLWCHAR)) + 1;

        // finally bind the parameter
        return_code = SQLBindParameter(
            handle,
            index,
            io_type,
            odbc_c_type,
            odbc_sql_type,
            column_size,
            0,
            (SQLPOINTER)str.c_str(),
            input_buffer_size,
            &indicator
        );
        break;
    case SQL_PARAM_OUTPUT:

        // create the output object and store it
        output_buffer.push_back(SimpleSqlTypes::SQLBoundOutput(binding, 0));

        // get the value
        std::wstring& str = std::get<std::wstring&>(output_buffer.back().binding().data());

        // infer column metadata
        if (!infer_column_metadata(handle, index, column_size, precision))
            column_size = static_cast<SQLULEN>(str.size()) + 1;

        // ensure the value buffer is large enough
        str.resize(column_size);

        // define specific parameters
        SQLLEN input_buffer_size = static_cast<SQLLEN>(str.size() * sizeof(SQLWCHAR)) + 1;

        // finally bind the parameter
        return_code = SQLBindParameter(
            handle,
            index,
            io_type,
            odbc_c_type,
            odbc_sql_type,
            column_size,
            0,
            (SQLPOINTER)str.c_str(),
            input_buffer_size,
            &output_buffer.back().buffer_size()
        );
        break;
    }
    return return_code;
}

template<typename T>
static SQLRETURN bind_numeric(void* handle, const SQLUSMALLINT &index, const SQLSMALLINT &io_type, const SQLSMALLINT &odbc_c_type, const SQLSMALLINT &odbc_sql_type, const SimpleSqlTypes::SQLBinding &binding, std::vector<SimpleSqlTypes::SQLBoundOutput> &output_buffer) {

    SQLRETURN return_code;
    switch (io_type) {
    case SQL_PARAM_INPUT_OUTPUT:

        // create the output object and store it
        output_buffer.push_back(SimpleSqlTypes::SQLBoundOutput(binding, 0));

        // get the value
        T& dbl = std::get<T&>(output_buffer.back().binding().data());

        // finally bind the parameter
        return_code = SQLBindParameter(
            handle,
            index,
            io_type,
            odbc_c_type,
            odbc_sql_type,
            0,
            0,
            &dbl,
            0,
            &output_buffer.back().buffer_size()
        );
        break;
    case SQL_PARAM_INPUT:

        // get the value
        T& dbl = std::get<T&>(output_buffer.back().binding().data());

        // define specific parameters
        SQLLEN indicator = binding.set_null() ? SQL_NULL_DATA : 0;

        // finally bind the parameter
        return_code = SQLBindParameter(
            handle,
            index,
            io_type,
            odbc_c_type,
            odbc_sql_type,
            0,
            0,
            &dbl,
            0,
            &indicator
        );
        break;
    case SQL_PARAM_OUTPUT:

        // create the output object and store it
        output_buffer.push_back(SimpleSqlTypes::SQLBoundOutput(binding, 0));

        // get the value
        T& dbl = std::get<T&>(output_buffer.back().binding().data());

        // finally bind the parameter
        return_code = SQLBindParameter(
            handle,
            index,
            io_type,
            odbc_c_type,
            odbc_sql_type,
            0,
            0,
            &dbl,
            0,
            &output_buffer.back().buffer_size()
        );
        break;
    }
    return return_code;
}

static SQLRETURN bind_bool_int(void* handle, const SQLUSMALLINT &index, const SQLSMALLINT &io_type, const SQLSMALLINT &odbc_c_type, const SQLSMALLINT &odbc_sql_type, const SimpleSqlTypes::SQLBinding &binding, std::vector<SimpleSqlTypes::SQLBoundOutput> &output_buffer) {
    
}

static SQLRETURN bind_guid(void* handle, const SQLUSMALLINT &index, const SQLSMALLINT &io_type, const SQLSMALLINT &odbc_c_type, const SQLSMALLINT &odbc_sql_type, const SimpleSqlTypes::SQLBinding &binding, std::vector<SimpleSqlTypes::SQLBoundOutput> &output_buffer) {
    
}

static SQLRETURN bind_datetime(void* handle, const SQLUSMALLINT &index, const SQLSMALLINT &io_type, const SQLSMALLINT &odbc_c_type, const SQLSMALLINT &odbc_sql_type, const SimpleSqlTypes::SQLBinding &binding, std::vector<SimpleSqlTypes::SQLBoundOutput> &output_buffer) {
    
}

static SQLRETURN bind_blob(void* handle, const SQLUSMALLINT &index, const SQLSMALLINT &io_type, const SQLSMALLINT &odbc_c_type, const SQLSMALLINT &odbc_sql_type, const SimpleSqlTypes::SQLBinding &binding, std::vector<SimpleSqlTypes::SQLBoundOutput> &output_buffer) {
    
}

// class definition

bool SimpleSql::SimQuery::define_columns(std::string &error) {

    auto get_columns = [&](SQLSMALLINT &column_count, std::string &error) -> bool {
        for (SQLUSMALLINT i = 0; i < column_count; ++i) {
            std::vector<SQLCHAR> column_name_buffer(cm_colname_size);
            SQLSMALLINT column_name_length;
            SQLSMALLINT data_type_id;
            SQLULEN data_size;
            SQLSMALLINT precision;
            SQLSMALLINT null_id;
            switch (SQLDescribeCol(mp_stmt_handle.get(), i, column_name_buffer.data(), cm_colname_size, &column_name_length, &data_type_id, &data_size, &precision, &null_id)) {
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

            std::string column_name(reinterpret_cast<const char*>(column_name_buffer.data()), column_name_length);
            m_column_map.emplace(column_name, static_cast<uint8_t>(i));
            m_columns.push_back(SimpleSqlTypes::ColumnMetadata(
                    static_cast<uint16_t>(i),
                    column_name,
                    ODBC_SQL_to_SQLDataType(data_type_id),
                    static_cast<uint64_t>(data_size),
                    static_cast<uint16_t>(precision),
                    ODBC_NULL_to_NullRuleType(null_id)
                )
            );
        }
    };

    SQLSMALLINT column_count = 0;
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

    if (column_count > 0) {
        m_is_select = true;
        if (!get_columns(column_count, error))
            return false;

        if (column_count != m_column_map.size()) {
            error = std::string("probable duplicate column names detected");
            return false;
        }

        m_matrix.make_valid(column_count);
    } else {
        m_is_select = false;
    }
    return true;
}

void SimpleSql::SimQuery::define_diagnostics() {
    SQLSMALLINT current_record_number = static_cast<SQLSMALLINT>(m_diagnostic_record_number);
    std::vector<SQLCHAR> sql_state(6);
    SQLINTEGER native_error;
    std::vector<SQLCHAR> message(SQL_MAX_MESSAGE_LENGTH);
    SQLSMALLINT message_length;
    SQLRETURN rc = SQLGetDiagRec(SQL_HANDLE_STMT, mp_stmt_handle.get(), current_record_number, sql_state.data(), &native_error, message.data(), sizeof(message), &message_length);
    while (rc != SQL_NO_DATA || rc != SQL_ERROR || rc != SQL_INVALID_HANDLE) {
        m_diagnostics.push_back(SimpleSqlTypes::DiagnosticRecord(
                static_cast<int16_t>(current_record_number),
                std::string(reinterpret_cast<const char*>(sql_state.data()), 6),
                static_cast<int32_t>(native_error),
                std::string(reinterpret_cast<const char*>(message.data()), message_length)
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
    return std::move(mp_stmt_handle);
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
    switch (SQLPrepare(mp_stmt_handle.get(), sql, SQL_NTS)) {
    case SQL_SUCCESS:
        return define_columns(error);
    case SQL_SUCCESS_WITH_INFO:
        m_info_pending = true;
        define_diagnostics();
        return define_columns(error);
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

    SQLSMALLINT parameter_count;
    SQLRETURN return_code = SQLNumParams(mp_stmt_handle.get(), &parameter_count);
    if (return_code != SQL_SUCCESS && return_code != SQL_SUCCESS_WITH_INFO) {
        error = std::string("could not calculate the prepared SQL statement's parameter count");
        return false;
    }

    if (parameter_count < 1) {
        error = std::string("the prepared SQL statement does not contain any parameters");
        return false;
    }

    SQLUSMALLINT parameter_index = static_cast<SQLUSMALLINT>(m_binding_index);

    SQLSMALLINT io_type;
    if (!get_io_type(binding.type(), io_type)) {
        error = std::string("could not determine the ODBC Input-Output type");
        return false;
    }

    BindingFamily family;
    if (!get_binding_family(binding.data_type(), family)) {
        error = std::string("could not determine a proper binding family");
        return false;
    }

    SQLSMALLINT c_data_type, sql_data_type;
    if (!get_odbc_data_types(binding.data_type(), c_data_type, sql_data_type)) {
        error = std::string("could not determine the ODBC C & SQL data types");
        return false;
    }

    switch (family) {
    case BindingFamily::STRING_UTF8:
        return_code = bind_string_utf8(
            mp_stmt_handle.get(),
            parameter_index,
            io_type,
            c_data_type,
            sql_data_type,
            binding,
            m_output_buffer
        );
        break;
    case BindingFamily::STRING_UTF16:
        return_code = bind_string_utf16(
            mp_stmt_handle.get(),
            parameter_index,
            io_type,
            c_data_type,
            sql_data_type,
            binding,
            m_output_buffer
        );
        break;
    case BindingFamily::NUMERIC:
        switch (binding.data_type()) {
        case SimpleSqlTypes::SimDataType::FLOAT:
            return_code = bind_numeric<float>(
                mp_stmt_handle.get(),
                parameter_index,
                io_type,
                c_data_type,
                sql_data_type,
                binding,
                m_output_buffer
            );
            break;
        case SimpleSqlTypes::SimDataType::DOUBLE:
            return_code = bind_numeric<double>(
                mp_stmt_handle.get(),
                parameter_index,
                io_type,
                c_data_type,
                sql_data_type,
                binding,
                m_output_buffer
            );
            break;
        default:
            error = std::string("cannot bind an undefined numeric type");
            return false;
        }
        break;
    case BindingFamily::BOOL_INT:
        return_code = bind_bool_int(
            mp_stmt_handle.get(),
            parameter_index,
            io_type,
            c_data_type,
            sql_data_type,
            binding,
            m_output_buffer
        );
        break;
    case BindingFamily::GUID:
        return_code = bind_guid(
            mp_stmt_handle.get(),
            parameter_index,
            io_type,
            c_data_type,
            sql_data_type,
            binding,
            m_output_buffer
        );
        break;
    case BindingFamily::DATETIME:
        return_code = bind_datetime(
            mp_stmt_handle.get(),
            parameter_index,
            io_type,
            c_data_type,
            sql_data_type,
            binding,
            m_output_buffer
        );
        break;
    case BindingFamily::BLOB:
        return_code = bind_blob(
            mp_stmt_handle.get(),
            parameter_index,
            io_type,
            c_data_type,
            sql_data_type,
            binding,
            m_output_buffer
        );
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

const size_t& SimpleSql::SimQuery::get_row_count() const {
    if (!m_matrix.is_valid())
        return -1;

    return m_matrix.rows();
}

const size_t& SimpleSql::SimQuery::get_column_count() const {
    if (!m_matrix.is_valid())
        return -1;

    return m_matrix.columns();
}

const SimpleSqlTypes::SQLCell& SimpleSql::SimQuery::get_cell(const std::string &key) const {
    auto iterator = m_key_data.find(key);
    if (iterator != m_key_data.end()) {
        return iterator->second;
    } else {
        return m_invalid_cell;
    }
}

const SimpleSqlTypes::SQLCell& SimpleSql::SimQuery::get_cell(const std::string &column, const size_t &row) const {
    if (!m_matrix.is_valid())
        return m_invalid_cell;

    auto iterator = m_column_map.find(column);
    if (iterator == m_column_map.end())
        return m_invalid_cell;

    size_t column_index = iterator->second;
    return get_cell(column_index, row);
}

const SimpleSqlTypes::SQLCell& SimpleSql::SimQuery::get_cell(const size_t &column, const size_t &row) const {
    if (column >= m_matrix.columns() || row >= m_matrix.rows() || !m_matrix.is_valid())
        return m_invalid_cell;

    return m_matrix.cells()[column + row * m_matrix.columns()];
}

const std::vector<SimpleSqlTypes::SQLCell>& SimpleSql::SimQuery::get_column(const std::string &column) const {
    auto iterator = m_column_map.find(column);
    if (iterator == m_column_map.end()) {
        std::vector<SimpleSqlTypes::SQLCell> v;
        return v;
    }

    size_t column_index = iterator->second;
    return get_column(column_index);
}

const std::vector<SimpleSqlTypes::SQLCell>& SimpleSql::SimQuery::get_column(const size_t &column) const {
    if (column >= m_matrix.columns() || !m_matrix.is_valid()) {
        std::vector<SimpleSqlTypes::SQLCell> v;
        return v;
    }

    auto column_count = m_matrix.columns();
    auto cells = m_matrix.cells();
    std::vector<SimpleSqlTypes::SQLCell> output;
    for (int i = 0; i < m_matrix.rows(); ++i) {
        output.push_back(cells[i * column_count + column]);
    }
    return output;
}

const std::vector<SimpleSqlTypes::SQLCell>& SimpleSql::SimQuery::get_row(const size_t &row) const {
    if (row >= m_matrix.rows() || !m_matrix.is_valid()) {
        std::vector<SimpleSqlTypes::SQLCell> v;
        return v;
    }

    size_t column_count = m_matrix.columns();
    std::vector<SimpleSqlTypes::SQLCell> cells = m_matrix.cells();
    size_t beginning = row * column_count;
    size_t ending = row * column_count + column_count;

    return std::vector<SimpleSqlTypes::SQLCell>(
        cells.begin() + row * m_matrix.columns(),
        cells.end() + row * m_matrix.columns() + m_matrix.columns()
    );
}

const std::vector<SimpleSqlTypes::SQLCell>& SimpleSql::SimQuery::get_data() const {
    if (!m_matrix.is_valid()) {
        std::vector<SimpleSqlTypes::SQLCell> v;
        return v;
    }
    return m_matrix.cells();
}

void SimpleSql::SimQuery::destroy() {

}