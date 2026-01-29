// SimQL stuff
#include <SimQuery.hpp>
#include <SimQL_Types.hpp>
#include <SimQL_Constants.hpp>

// STL stuff
#include <cstdint>
#include <memory>
#include <utility>
#include <vector>
#include <unordered_map>
#include <span>
#include <concepts>
#include <iostream>

// for compiling on Windows (ew)
#ifdef _WIN32

#define WIN32_LEAN_AND_MEAN
#define NOGDICAPMASKS               // CC_*, LC_*, PC_*, CP_*, TC_*, RC_
#define NOVIRTUALKEYCODES           // VK_*
#define NOWINMESSAGES               // WM_*, EM_*, LB_*, CB_*
#define NOWINSTYLES                 // WS_*, CS_*, ES_*, LBS_*, SBS_*, CBS_*
#define NOSYSMETRICS                // SM_*
#define NOMENUS                     // MF_*
#define NOICONS                     // IDI_*
#define NOKEYSTATES                 // MK_*
#define NOSYSCOMMANDS               // SC_*
#define NORASTEROPS                 // Binary and Tertiary raster ops
#define NOSHOWWINDOW                // SW_*
#define OEMRESOURCE                 // OEM Resource values
#define NOATOM                      // Atom Manager routines
#define NOCLIPBOARD                 // Clipboard routines
#define NOCOLOR                     // Screen colors
#define NOCTLMGR                    // Control and Dialog routines
#define NODRAWTEXT                  // DrawText() and DT_*
#define NOGDI                       // All GDI defines and routines
#define NOKERNEL                    // All KERNEL defines and routines
#define NOUSER                      // All USER defines and routines
#define NONLS                       // All NLS defines and routines
#define NOMB                        // MB_* and MessageBox()
#define NOMEMMGR                    // GMEM_*, LMEM_*, GHND, LHND, associated routines
#define NOMETAFILE                  // typedef METAFILEPICT
// #define NOMINMAX                 // Macros min(a,b) and max(a,b) || already set in mingw os_defines.h
#define NOMSG                       // typedef MSG and associated routines
#define NOOPENFILE                  // OpenFile(), OemToAnsi, AnsiToOem, and OF_*
#define NOSCROLL                    // SB_* and scrolling routines
#define NOSERVICE                   // All Service Controller routines, SERVICE_ equates, etc.
#define NOSOUND                     // Sound driver routines
#define NOTEXTMETRIC                // typedef TEXTMETRIC and associated routines
#define NOWH                        // SetWindowsHook and WH_*
#define NOWINOFFSETS                // GWL_*, GCL_*, associated routines
#define NOCOMM                      // COMM driver routines
#define NOKANJI                     // Kanji support stuff.
#define NOHELP                      // Help engine interface.
#define NOPROFILER                  // Profiler interface.
#define NODEFERWINDOWPOS            // DeferWindowPos routines
#define NOMCX                       // Modem Configuration Extensions
#include <windows.h>

#endif

// ODBC stuff
#include <sqltypes.h>
#include <sqlext.h>
#include <sql.h>

// private enums

enum class BindingFamily : std::uint8_t {
    STRING_UTF8     = 0,
    STRING_UTF16    = 1,
    NUMERIC         = 2,
    BOOL_INT        = 3,
    GUID            = 4,
    DATETIME        = 5,
    BLOB            = 6
};

template<typename T>
concept string_like = requires(T& t) {
    std::is_same_v<T, std::string> ||
    std::is_same_v<T, std::wstring> ||
    std::is_same_v<T, std::string_view> ||
    std::is_same_v<T, std::wstring_view>;
};

template<typename T>
concept fixed_size = requires(T& t) {
    std::is_same_v<T, float> ||
    std::is_same_v<T, double> ||
    std::is_same_v<T, bool> ||
    std::is_same_v<T, std::int8_t> ||
    std::is_same_v<T, std::int16_t> ||
    std::is_same_v<T, std::int32_t> ||
    std::is_same_v<T, std::int64_t>;
};

// helper functions

static bool get_odbc_data_types(const SimpleSqlTypes::SQLDataType& data_type, SQLSMALLINT& odbc_c_type, SQLSMALLINT& odbc_sql_type) {

    switch (data_type) {
    case SimpleSqlTypes::SQLDataType::STRING_UTF8:
        odbc_c_type = SQL_C_CHAR;
        odbc_sql_type = SQL_VARCHAR;
        break;
    case SimpleSqlTypes::SQLDataType::STRING_UTF16:
        odbc_c_type = SQL_C_WCHAR;
        odbc_sql_type = SQL_WVARCHAR;
        break;
    case SimpleSqlTypes::SQLDataType::LONG_STRING_UTF8:
        odbc_c_type = SQL_C_CHAR;
        odbc_sql_type = SQL_LONGVARCHAR;
        break;
    case SimpleSqlTypes::SQLDataType::LONG_STRING_UTF16:
        odbc_c_type = SQL_C_WCHAR;
        odbc_sql_type = SQL_WLONGVARCHAR;
        break;
    case SimpleSqlTypes::SQLDataType::FLOAT:
        odbc_c_type = SQL_C_FLOAT;
        odbc_sql_type = SQL_FLOAT;
        break;
    case SimpleSqlTypes::SQLDataType::DOUBLE:
        odbc_c_type = SQL_C_DOUBLE;
        odbc_sql_type = SQL_DOUBLE;
        break;
    case SimpleSqlTypes::SQLDataType::BOOLEAN:
        odbc_c_type = SQL_C_BIT;
        odbc_sql_type = SQL_BIT;
        break;
    case SimpleSqlTypes::SQLDataType::INT_8:
        odbc_c_type = SQL_C_STINYINT;
        odbc_sql_type = SQL_TINYINT;
        break;
    case SimpleSqlTypes::SQLDataType::INT_16:
        odbc_c_type = SQL_C_SSHORT;
        odbc_sql_type = SQL_SMALLINT;
        break;
    case SimpleSqlTypes::SQLDataType::INT_32:
        odbc_c_type = SQL_C_SLONG;
        odbc_sql_type = SQL_INTEGER;
        break;
    case SimpleSqlTypes::SQLDataType::INT_64:
        odbc_c_type = SQL_C_SBIGINT;
        odbc_sql_type = SQL_BIGINT;
        break;
    case SimpleSqlTypes::SQLDataType::ODBC_GUID:
        odbc_c_type = SQL_C_BINARY;
        odbc_sql_type = SQL_BINARY;
        break;
    case SimpleSqlTypes::SQLDataType::GUID:
        odbc_c_type = SQL_C_GUID;
        odbc_sql_type = SQL_GUID;
        break;
    case SimpleSqlTypes::SQLDataType::DATETIME:
        odbc_c_type = SQL_C_TYPE_TIMESTAMP;
        odbc_sql_type = SQL_TYPE_TIMESTAMP;
        break;
    case SimpleSqlTypes::SQLDataType::DATE:
        odbc_c_type = SQL_C_TYPE_DATE;
        odbc_sql_type = SQL_TYPE_DATE;
        break;
    case SimpleSqlTypes::SQLDataType::TIME:
        odbc_c_type = SQL_C_TYPE_TIME;
        odbc_sql_type = SQL_TYPE_TIME;
        break;
    case SimpleSqlTypes::SQLDataType::BLOB:
        odbc_c_type = SQL_C_BINARY;
        odbc_sql_type = SQL_VARBINARY;
        break;
    case SimpleSqlTypes::SQLDataType::LONG_BLOB:
        odbc_c_type = SQL_C_BINARY;
        odbc_sql_type = SQL_LONGVARBINARY;
        break;
    default:
        return false;
    }
    return true;
}

static bool get_binding_family(const SimpleSqlTypes::SQLDataType& data_type, BindingFamily& family) {

    // for narrow strings
    if (data_type == SimpleSqlTypes::SQLDataType::STRING_UTF8 ||
        data_type == SimpleSqlTypes::SQLDataType::LONG_STRING_UTF8) {

        family = BindingFamily::STRING_UTF8;
        return true;
    }

    // for wide strings
    if (data_type == SimpleSqlTypes::SQLDataType::STRING_UTF16 ||
        data_type == SimpleSqlTypes::SQLDataType::LONG_STRING_UTF16) {

        family = BindingFamily::STRING_UTF16;
        return true;
    }

    // for numeric types
    if (data_type == SimpleSqlTypes::SQLDataType::FLOAT ||
        data_type == SimpleSqlTypes::SQLDataType::DOUBLE) {

        family = BindingFamily::NUMERIC;
        return true;
    }

    // for integer / boolean types
    if (data_type == SimpleSqlTypes::SQLDataType::BOOLEAN ||
        data_type == SimpleSqlTypes::SQLDataType::INT_8 ||
        data_type == SimpleSqlTypes::SQLDataType::INT_16 ||
        data_type == SimpleSqlTypes::SQLDataType::INT_32 ||
        data_type == SimpleSqlTypes::SQLDataType::INT_64) {

        family = BindingFamily::BOOL_INT;
        return true;
    }

    // for GUID types
    if (data_type == SimpleSqlTypes::SQLDataType::ODBC_GUID ||
        data_type == SimpleSqlTypes::SQLDataType::GUID) {

        family = BindingFamily::GUID;
        return true;
    }

    // for datetime types
    if (data_type == SimpleSqlTypes::SQLDataType::DATETIME ||
        data_type == SimpleSqlTypes::SQLDataType::DATE ||
        data_type == SimpleSqlTypes::SQLDataType::TIME) {

        family = BindingFamily::DATETIME;
        return true;
    }

    // for binary types
    if (data_type == SimpleSqlTypes::SQLDataType::BLOB ||
        data_type == SimpleSqlTypes::SQLDataType::LONG_BLOB) {

        family = BindingFamily::BLOB;
        return true;
    }

    return false;
}

static bool get_io_type(const SimpleSqlTypes::BindingType& binding_type, SQLSMALLINT& output) {
    switch (binding_type) {
    case SimpleSqlTypes::BindingType::INPUT_OUTPUT:         output = SQL_PARAM_INPUT_OUTPUT; break;
    case SimpleSqlTypes::BindingType::INPUT:                output = SQL_PARAM_INPUT; break;
    case SimpleSqlTypes::BindingType::OUTPUT:               output = SQL_PARAM_OUTPUT; break;
    default:                                                return false;
    }
    return true;
}

static bool OBDC_SQL_to_ODBC_C(const SQLSMALLINT& odbc_sql_type, SQLSMALLINT &odbc_c_type) {
    switch (odbc_sql_type) {
    case SQL_VARCHAR:
        odbc_c_type = SQL_C_CHAR;
        break;
    case SQL_WVARCHAR:
        odbc_c_type = SQL_C_WCHAR;
        break;
    case SQL_LONGVARCHAR:
        odbc_c_type = SQL_C_CHAR;
        break;
    case SQL_WLONGVARCHAR:
        odbc_c_type = SQL_C_WCHAR;
        break;
    case SQL_FLOAT:
        odbc_c_type = SQL_C_FLOAT;
        break;
    case SQL_DOUBLE:
        odbc_c_type = SQL_C_DOUBLE;
        break;
    case SQL_BIT:
        odbc_c_type = SQL_C_BIT;
        break;
    case SQL_TINYINT:
        odbc_c_type = SQL_C_STINYINT;
        break;
    case SQL_SMALLINT:
        odbc_c_type = SQL_C_SSHORT;
        break;
    case SQL_INTEGER:
        odbc_c_type = SQL_C_SLONG;
        break;
    case SQL_BIGINT:
        odbc_c_type = SQL_C_SBIGINT;
        break;
    case SQL_BINARY:
        odbc_c_type = SQL_C_BINARY;
        break;
    case SQL_GUID:
        odbc_c_type = SQL_C_GUID;
        break;
    case SQL_TYPE_TIMESTAMP:
        odbc_c_type = SQL_C_TYPE_TIMESTAMP;
        break;
    case SQL_TYPE_DATE:
        odbc_c_type = SQL_C_TYPE_DATE;
        break;
    case SQL_TYPE_TIME:
        odbc_c_type = SQL_C_TYPE_TIME;
        break;
    case SQL_VARBINARY:
        odbc_c_type = SQL_C_BINARY;
        break;
    case SQL_LONGVARBINARY:
        odbc_c_type = SQL_C_BINARY;
        break;
    default:
        return false;
    }
    return true;
}

static SimpleSqlTypes::SQLDataType ODBC_SQL_to_SQLDataType(const SQLSMALLINT& data_type) {
    switch (data_type) {
    case SQL_CHAR:                      return SimpleSqlTypes::SQLDataType::STRING_UTF8;
    case SQL_VARCHAR:                   return SimpleSqlTypes::SQLDataType::STRING_UTF8;
    case SQL_LONGVARCHAR:               return SimpleSqlTypes::SQLDataType::LONG_STRING_UTF8;
    case SQL_WCHAR:                     return SimpleSqlTypes::SQLDataType::STRING_UTF16;
    case SQL_WVARCHAR:                  return SimpleSqlTypes::SQLDataType::STRING_UTF16;
    case SQL_WLONGVARCHAR:              return SimpleSqlTypes::SQLDataType::LONG_STRING_UTF16;
    case SQL_FLOAT:                     return SimpleSqlTypes::SQLDataType::FLOAT;
    case SQL_DOUBLE:                    return SimpleSqlTypes::SQLDataType::DOUBLE;
    case SQL_BIT:                       return SimpleSqlTypes::SQLDataType::BOOLEAN;
    case SQL_TINYINT:                   return SimpleSqlTypes::SQLDataType::INT_8;
    case SQL_SMALLINT:                  return SimpleSqlTypes::SQLDataType::INT_16;
    case SQL_INTEGER:                   return SimpleSqlTypes::SQLDataType::INT_32;
    case SQL_BIGINT:                    return SimpleSqlTypes::SQLDataType::INT_64;
    case SQL_GUID:                      return SimpleSqlTypes::SQLDataType::GUID;
    case SQL_TYPE_TIMESTAMP:            return SimpleSqlTypes::SQLDataType::DATETIME;
    case SQL_TYPE_DATE:                 return SimpleSqlTypes::SQLDataType::DATE;
    case SQL_TYPE_TIME:                 return SimpleSqlTypes::SQLDataType::TIME;
    case SQL_BINARY:                    return SimpleSqlTypes::SQLDataType::BLOB;
    case SQL_VARBINARY:                 return SimpleSqlTypes::SQLDataType::BLOB;
    case SQL_LONGVARBINARY:             return SimpleSqlTypes::SQLDataType::LONG_BLOB;
    default:                            return SimpleSqlTypes::SQLDataType::UNKNOWN;
    }
}

static SimpleSqlTypes::NullRuleType ODBC_NULL_to_NullRuleType(const SQLSMALLINT& null_type) {
    switch (null_type) {
    case SQL_NO_NULLS:                  return SimpleSqlTypes::NullRuleType::NOT_ALLOWED;
    case SQL_NULLABLE:                  return SimpleSqlTypes::NullRuleType::ALLOWED;
    case SQL_NULLABLE_UNKNOWN:          return SimpleSqlTypes::NullRuleType::UNKNOWN;
    default:                            return SimpleSqlTypes::NullRuleType::UNKNOWN;
    }
}

static bool infer_parameter_metadata(void* handle, const SQLUSMALLINT& parameter_index, SQLULEN& size, SQLSMALLINT& precision) {
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

template<string_like T, typename SQL_CHARACTER_TYPE>
static SQLRETURN bind_string_parameter(void* handle, const SQLUSMALLINT& index, const SQLSMALLINT& io_type, const SQLSMALLINT& odbc_c_type, const SQLSMALLINT& odbc_sql_type, SimpleSqlTypes::SQLBinding& binding, std::vector<SimpleSqlTypes::SQLBoundOutput>& parameter_buffer) {

    T& val = std::get<T>(binding.data());
    SQLULEN parameter_size;
    SQLLEN indicator;
    SQLSMALLINT precision;
    SQLRETURN return_code;
    SQLLEN input_buffer_size;
    switch (io_type) {
    case SQL_PARAM_INPUT_OUTPUT:

        // create the output object and store it
        parameter_buffer.push_back(SimpleSqlTypes::SQLBoundOutput(binding, binding.set_null() ? SQL_NULL_DATA : SQL_NTS));

        // get the value
        val = std::get<T>(parameter_buffer.back().binding().data());

        // infer parameter metadata
        if (!infer_parameter_metadata(handle, index, parameter_size, precision))
            parameter_size = static_cast<SQLULEN>(val.size()) + 1;

        // ensure the value buffer is large enough (plus null-terminator)
        val.resize(parameter_size);

        // define specific parameters
        input_buffer_size = static_cast<SQLLEN>(val.size() * sizeof(SQL_CHARACTER_TYPE)) + 1;

        // finally bind the parameter
        return_code = SQLBindParameter(
            handle,
            index,
            io_type,
            odbc_c_type,
            odbc_sql_type,
            parameter_size,
            0,
            (SQLPOINTER)val.c_str(),
            input_buffer_size,
            &parameter_buffer.back().buffer_size()
        );
        break;
    case SQL_PARAM_INPUT:

        // infer parameter metadata
        if (!infer_parameter_metadata(handle, index, parameter_size, precision))
            parameter_size = static_cast<SQLULEN>(val.size()) + 1;

        // define specific parameters
        indicator = binding.set_null() ? SQL_NULL_DATA : SQL_NTS;
        input_buffer_size = static_cast<SQLLEN>(val.size() * sizeof(SQL_CHARACTER_TYPE)) + 1;

        // finally bind the parameter
        return_code = SQLBindParameter(
            handle,
            index,
            io_type,
            odbc_c_type,
            odbc_sql_type,
            parameter_size,
            0,
            (SQLPOINTER)val.c_str(),
            input_buffer_size,
            &indicator
        );
        break;
    case SQL_PARAM_OUTPUT:

        // create the output object and store it
        parameter_buffer.push_back(SimpleSqlTypes::SQLBoundOutput(binding, 0));

        // get the value
        val = std::get<T>(parameter_buffer.back().binding().data());

        // infer parameter metadata
        if (!infer_parameter_metadata(handle, index, parameter_size, precision))
            parameter_size = static_cast<SQLULEN>(val.size()) + 1;

        // ensure the value buffer is large enough
        val.resize(parameter_size);

        // define specific parameters
        input_buffer_size = static_cast<SQLLEN>(val.size() * sizeof(SQL_CHARACTER_TYPE)) + 1;

        // finally bind the parameter
        return_code = SQLBindParameter(
            handle,
            index,
            io_type,
            odbc_c_type,
            odbc_sql_type,
            parameter_size,
            0,
            (SQLPOINTER)val.c_str(),
            input_buffer_size,
            &parameter_buffer.back().buffer_size()
        );
        break;
    }
    return return_code;
}

template<fixed_size T>
static SQLRETURN bind_fixed_size_parameter(void* handle, const SQLUSMALLINT& index, const SQLSMALLINT& io_type, const SQLSMALLINT& odbc_c_type, const SQLSMALLINT& odbc_sql_type, SimpleSqlTypes::SQLBinding& binding, std::vector<SimpleSqlTypes::SQLBoundOutput>& parameter_buffer) {

    SQLLEN indicator;
    T& val = std::get<T>(binding.data());
    SQLRETURN return_code;
    switch (io_type) {
    case SQL_PARAM_INPUT_OUTPUT:

        // create the output object and store it
        parameter_buffer.push_back(SimpleSqlTypes::SQLBoundOutput(binding, binding.set_null() ? SQL_NULL_DATA : 0));

        // get the value
        val = std::get<T>(parameter_buffer.back().binding().data());

        // finally bind the parameter
        return_code = SQLBindParameter(
            handle,
            index,
            io_type,
            odbc_c_type,
            odbc_sql_type,
            0,
            0,
            &val,
            0,
            &parameter_buffer.back().buffer_size()
        );
        break;
    case SQL_PARAM_INPUT:

        // define specific parameters
        indicator = binding.set_null() ? SQL_NULL_DATA : 0;

        // finally bind the parameter
        return_code = SQLBindParameter(
            handle,
            index,
            io_type,
            odbc_c_type,
            odbc_sql_type,
            0,
            0,
            &val,
            0,
            &indicator
        );
        break;
    case SQL_PARAM_OUTPUT:

        // create the output object and store it
        parameter_buffer.push_back(SimpleSqlTypes::SQLBoundOutput(binding, 0));

        // get the value
        val = std::get<T>(parameter_buffer.back().binding().data());

        // finally bind the parameter
        return_code = SQLBindParameter(
            handle,
            index,
            io_type,
            odbc_c_type,
            odbc_sql_type,
            0,
            0,
            &val,
            0,
            &parameter_buffer.back().buffer_size()
        );
        break;
    }
    return return_code;
}

static SQLRETURN bind_odbc_guid_parameter(void* handle, const SQLUSMALLINT& index, const SQLSMALLINT& io_type, const SQLSMALLINT& odbc_c_type, const SQLSMALLINT& odbc_sql_type, SimpleSqlTypes::SQLBinding& binding, std::vector<SimpleSqlTypes::SQLBoundOutput>& parameter_buffer) {

    SQLLEN indicator;
    SimpleSqlTypes::ODBC_GUID& val = std::get<SimpleSqlTypes::ODBC_GUID>(binding.data());
    SQLRETURN return_code;
    switch (io_type) {
    case SQL_PARAM_INPUT_OUTPUT:

        // create the output object and store it
        parameter_buffer.push_back(SimpleSqlTypes::SQLBoundOutput(binding, binding.set_null() ? SQL_NULL_DATA : 16ULL));

        // get the value
        val = std::get<SimpleSqlTypes::ODBC_GUID>(parameter_buffer.back().binding().data());

        // finally bind the parameter
        return_code = SQLBindParameter(
            handle,
            index,
            io_type,
            odbc_c_type,
            odbc_sql_type,
            sizeof(val),
            0,
            &val,
            sizeof(val),
            &parameter_buffer.back().buffer_size()
        );
        break;
    case SQL_PARAM_INPUT:

        // define specific parameters
        indicator = binding.set_null() ? SQL_NULL_DATA : sizeof(val);

        // finally bind the parameter
        return_code = SQLBindParameter(
            handle,
            index,
            io_type,
            odbc_c_type,
            odbc_sql_type,
            sizeof(val),
            0,
            &val,
            sizeof(val),
            &indicator
        );
        break;
    case SQL_PARAM_OUTPUT:

        // create the output object and store it
        parameter_buffer.push_back(SimpleSqlTypes::SQLBoundOutput(binding, 16ULL));

        // get the value
        val = std::get<SimpleSqlTypes::ODBC_GUID>(parameter_buffer.back().binding().data());

        // finally bind the parameter
        return_code = SQLBindParameter(
            handle,
            index,
            io_type,
            odbc_c_type,
            odbc_sql_type,
            sizeof(val),
            0,
            &val,
            sizeof(val),
            &parameter_buffer.back().buffer_size()
        );
        break;
    }
    return return_code;
}

static SQLRETURN bind_guid_parameter(void* handle, const SQLUSMALLINT& index, const SQLSMALLINT& io_type, const SQLSMALLINT& odbc_c_type, const SQLSMALLINT& odbc_sql_type, SimpleSqlTypes::SQLBinding& binding, std::vector<SimpleSqlTypes::SQLBoundOutput>& parameter_buffer) {

    SQLLEN indicator;
    SimpleSqlTypes::GUID& val = std::get<SimpleSqlTypes::GUID>(binding.data());
    SQLRETURN return_code;
    switch (io_type) {
    case SQL_PARAM_INPUT_OUTPUT:

        // create the output object and store it
        parameter_buffer.push_back(SimpleSqlTypes::SQLBoundOutput(binding, binding.set_null() ? SQL_NULL_DATA : 0));

        // get the value
        val = std::get<SimpleSqlTypes::GUID>(parameter_buffer.back().binding().data());

        // finally bind the parameter
        return_code = SQLBindParameter(
            handle,
            index,
            io_type,
            odbc_c_type,
            odbc_sql_type,
            val.size(),
            0,
            &val,
            val.size(),
            &parameter_buffer.back().buffer_size()
        );
        break;
    case SQL_PARAM_INPUT:

        // define specific parameters
        indicator = binding.set_null() ? SQL_NULL_DATA : 0;

        // finally bind the parameter
        return_code = SQLBindParameter(
            handle,
            index,
            io_type,
            odbc_c_type,
            odbc_sql_type,
            val.size(),
            0,
            &val,
            val.size(),
            &indicator
        );
        break;
    case SQL_PARAM_OUTPUT:

        // create the output object and store it
        parameter_buffer.push_back(SimpleSqlTypes::SQLBoundOutput(binding, 0));

        // get the value
        val = std::get<SimpleSqlTypes::GUID>(parameter_buffer.back().binding().data());

        // finally bind the parameter
        return_code = SQLBindParameter(
            handle,
            index,
            io_type,
            odbc_c_type,
            odbc_sql_type,
            val.size(),
            0,
            &val,
            val.size(),
            &parameter_buffer.back().buffer_size()
        );
        break;
    }
    return return_code;
}

template<SimpleSqlTypes::temporal_types T>
static SQLRETURN bind_datetime_parameter(void* handle, const SQLUSMALLINT& index, const SQLSMALLINT& io_type, const SQLSMALLINT& odbc_c_type, const SQLSMALLINT& odbc_sql_type, SimpleSqlTypes::SQLBinding& binding, std::vector<SimpleSqlTypes::SQLBoundOutput>& parameter_buffer) {

    SQLLEN indicator;
    T& val = std::get<T>(binding.data());
    SQLRETURN return_code;
    switch (io_type) {
    case SQL_PARAM_INPUT_OUTPUT:

        // create the output object and store it
        parameter_buffer.push_back(SimpleSqlTypes::SQLBoundOutput(binding, binding.set_null() ? SQL_NULL_DATA : 0));

        // get the value
        val = std::get<T>(parameter_buffer.back().binding().data());

        // finally bind the parameter
        return_code = SQLBindParameter(
            handle,
            index,
            io_type,
            odbc_c_type,
            odbc_sql_type,
            0,
            0,
            &val.temporal(),
            0,
            &parameter_buffer.back().buffer_size()
        );
        break;
    case SQL_PARAM_INPUT:

        // define specific parameters
        indicator = binding.set_null() ? SQL_NULL_DATA : 0;

        // finally bind the parameter
        return_code = SQLBindParameter(
            handle,
            index,
            io_type,
            odbc_c_type,
            odbc_sql_type,
            0,
            0,
            &val.temporal(),
            0,
            &indicator
        );
        break;
    case SQL_PARAM_OUTPUT:

        // create the output object and store it
        parameter_buffer.push_back(SimpleSqlTypes::SQLBoundOutput(binding, 0));

        // get the value
        val = std::get<T>(parameter_buffer.back().binding().data());

        // finally bind the parameter
        return_code = SQLBindParameter(
            handle,
            index,
            io_type,
            odbc_c_type,
            odbc_sql_type,
            0,
            0,
            &val.temporal(),
            0,
            &parameter_buffer.back().buffer_size()
        );
        break;
    }
    return return_code;
}

static SQLRETURN bind_blob_parameter(void* handle, const SQLUSMALLINT& index, const SQLSMALLINT& io_type, const SQLSMALLINT& odbc_c_type, const SQLSMALLINT& odbc_sql_type, SimpleSqlTypes::SQLBinding& binding, std::vector<SimpleSqlTypes::SQLBoundOutput>& parameter_buffer) {

    SQLLEN indicator;
    std::vector<std::uint8_t>& val = std::get<std::vector<std::uint8_t>>(binding.data());
    SQLULEN parameter_size;
    SQLSMALLINT precision;
    SQLRETURN return_code;
    switch (io_type) {
    case SQL_PARAM_INPUT_OUTPUT:

        // create the output object and store it
        parameter_buffer.push_back(SimpleSqlTypes::SQLBoundOutput(binding, binding.set_null() ? SQL_NULL_DATA : 0));

        // get the value
        val = std::get<std::vector<std::uint8_t>>(parameter_buffer.back().binding().data());

        // infer parameter metadata
        if (infer_parameter_metadata(handle, index, parameter_size, precision))
            val.resize(static_cast<size_t>(parameter_size));

        // finally bind the parameter
        return_code = SQLBindParameter(
            handle,
            index,
            io_type,
            odbc_c_type,
            odbc_sql_type,
            val.size(),
            0,
            val.data(),
            val.size(),
            &parameter_buffer.back().buffer_size()
        );
        break;
    case SQL_PARAM_INPUT:

        // infer parameter metadata
        if (infer_parameter_metadata(handle, index, parameter_size, precision))
            val.resize(static_cast<size_t>(parameter_size));

        // define specific parameters
        indicator = binding.set_null() ? SQL_NULL_DATA : static_cast<SQLLEN>(val.size());

        // finally bind the parameter
        return_code = SQLBindParameter(
            handle,
            index,
            io_type,
            odbc_c_type,
            odbc_sql_type,
            val.size(),
            0,
            val.data(),
            val.size(),
            &indicator
        );
        break;
    case SQL_PARAM_OUTPUT:

        // create the output object and store it
        parameter_buffer.push_back(SimpleSqlTypes::SQLBoundOutput(binding, 0));

        // get the value
        val = std::get<std::vector<std::uint8_t>>(parameter_buffer.back().binding().data());

        // infer parameter metadata
        if (infer_parameter_metadata(handle, index, parameter_size, precision))
            val.resize(static_cast<size_t>(parameter_size));

        // finally bind the parameter
        return_code = SQLBindParameter(
            handle,
            index,
            io_type,
            odbc_c_type,
            odbc_sql_type,
            val.size(),
            0,
            val.data(),
            val.size(),
            &parameter_buffer.back().buffer_size()
        );
        break;
    }
    return return_code;
}

template<string_like T, typename SQL_CHARACTER_TYPE>
static SQLRETURN bind_string_column(void* handle, const SQLUSMALLINT& index, const SQLSMALLINT& odbc_c_type, SimpleSqlTypes::ColumnMetadata& meta, std::vector<SimpleSqlTypes::SQLBoundOutput>& column_buffer) {

    // add to the column buffer
    SimpleSqlTypes::SQLVariant data;
    SimpleSqlTypes::SQLBinding binding(meta.name(), data, SimpleSqlTypes::BindingType::OUTPUT, meta.sim_data_type());
    SimpleSqlTypes::SQLBoundOutput output(binding);
    column_buffer.push_back(output);

    // get a reference to the value
    T& val = std::get<T>(column_buffer.back().binding().data());

    // calculate the input buffer size
    SQLLEN input_buffer_size = static_cast<SQLLEN>(val.size() * sizeof(SQL_CHARACTER_TYPE)) + 1;

    // bind the column
    return SQLBindCol(
        handle,
        index,
        odbc_c_type,
        (SQLPOINTER)val.c_str(),
        input_buffer_size,
        &column_buffer.back().buffer_size()
    );
}

template<fixed_size T>
static SQLRETURN bind_fixed_size_column(void* handle, const SQLUSMALLINT& index, const SQLSMALLINT& odbc_c_type, SimpleSqlTypes::ColumnMetadata& meta, std::vector<SimpleSqlTypes::SQLBoundOutput>& column_buffer) {

    // add to the column buffer
    SimpleSqlTypes::SQLVariant data;
    SimpleSqlTypes::SQLBinding binding(meta.name(), data, SimpleSqlTypes::BindingType::OUTPUT, meta.sim_data_type());
    SimpleSqlTypes::SQLBoundOutput output(binding);
    column_buffer.push_back(output);

    // get a reference to the value
    T& val = std::get<T>(column_buffer.back().binding().data());

    // bind the column
    return SQLBindCol(
        handle,
        index,
        odbc_c_type,
        &val,
        sizeof(val),
        &column_buffer.back().buffer_size()
    );
}

static SQLRETURN bind_odbc_guid_column(void* handle, const SQLUSMALLINT& index, const SQLSMALLINT& odbc_c_type, SimpleSqlTypes::ColumnMetadata& meta, std::vector<SimpleSqlTypes::SQLBoundOutput>& column_buffer) {

    // add to the column buffer
    SimpleSqlTypes::SQLVariant data;
    SimpleSqlTypes::SQLBinding binding(meta.name(), data, SimpleSqlTypes::BindingType::OUTPUT, meta.sim_data_type());
    SimpleSqlTypes::SQLBoundOutput output(binding, 16ULL);
    column_buffer.push_back(output);

    // get a reference to the value
    SimpleSqlTypes::ODBC_GUID& val = std::get<SimpleSqlTypes::ODBC_GUID>(column_buffer.back().binding().data());

    // bind the column
    return SQLBindCol(
        handle,
        index,
        odbc_c_type,
        &val,
        sizeof(val),
        &column_buffer.back().buffer_size()
    );
}

static SQLRETURN bind_guid_column(void* handle, const SQLUSMALLINT& index, const SQLSMALLINT& odbc_c_type, SimpleSqlTypes::ColumnMetadata& meta, std::vector<SimpleSqlTypes::SQLBoundOutput>& column_buffer) {

    // add to the column buffer
    SimpleSqlTypes::SQLVariant data;
    SimpleSqlTypes::SQLBinding binding(meta.name(), data, SimpleSqlTypes::BindingType::OUTPUT, meta.sim_data_type());
    SimpleSqlTypes::SQLBoundOutput output(binding);
    column_buffer.push_back(output);

    // get a reference to the value
    SimpleSqlTypes::GUID& val = std::get<SimpleSqlTypes::GUID>(column_buffer.back().binding().data());

    // bind the column
    return SQLBindCol(
        handle,
        index,
        odbc_c_type,
        val.data(),
        val.size(),
        &column_buffer.back().buffer_size()
    );
}

template<SimpleSqlTypes::temporal_types T>
static SQLRETURN bind_datetime_column(void* handle, const SQLUSMALLINT& index, const SQLSMALLINT& odbc_c_type, SimpleSqlTypes::ColumnMetadata& meta, std::vector<SimpleSqlTypes::SQLBoundOutput>& column_buffer) {

    // add to the column buffer
    SimpleSqlTypes::SQLVariant data;
    SimpleSqlTypes::SQLBinding binding(meta.name(), data, SimpleSqlTypes::BindingType::OUTPUT, meta.sim_data_type());
    SimpleSqlTypes::SQLBoundOutput output(binding);
    column_buffer.push_back(output);

    // get a reference to the value
    T& val = std::get<T>(column_buffer.back().binding().data());

    // bind the column
    return SQLBindCol(
        handle,
        index,
        odbc_c_type,
        &val,
        sizeof(T),
        &column_buffer.back().buffer_size()
    );
}

static SQLRETURN bind_blob_column(void* handle, const SQLUSMALLINT& index, const SQLSMALLINT& odbc_c_type, SimpleSqlTypes::ColumnMetadata& meta, std::vector<SimpleSqlTypes::SQLBoundOutput>& column_buffer) {

    // add to the column buffer
    SimpleSqlTypes::SQLVariant data;
    SimpleSqlTypes::SQLBinding binding(meta.name(), data, SimpleSqlTypes::BindingType::OUTPUT, meta.sim_data_type());
    SimpleSqlTypes::SQLBoundOutput output(binding);
    column_buffer.push_back(output);

    // get a reference to the value
    std::vector<std::uint8_t>& val = std::get<std::vector<std::uint8_t>>(column_buffer.back().binding().data());
    val.resize(meta.data_size());

    // bind the column
    return SQLBindCol(
        handle,
        index,
        odbc_c_type,
        val.data(),
        val.size(),
        &column_buffer.back().buffer_size()
    );
}

// class definition

std::uint8_t SimpleSql::SimQuery::define_columns() {

    auto get_columns = [&](SQLSMALLINT &column_count) -> std::uint8_t {
        for (SQLUSMALLINT i = 0; i < column_count; ++i) {
            std::vector<SQLCHAR> column_name_buffer(SimpleSqlConstants::Limits::max_sql_column_name_size);
            SQLSMALLINT column_name_length;
            SQLSMALLINT odbc_sql_type;
            SQLULEN data_size;
            SQLSMALLINT precision;
            SQLSMALLINT null_id;
            SQLRETURN sr = SQLDescribeCol(h_stmt->get(), i + 1, column_name_buffer.data(), SimpleSqlConstants::Limits::max_sql_column_name_size, &column_name_length, &odbc_sql_type, &data_size, &precision, &null_id);
            if (sr != SQL_SUCCESS && sr != SQL_SUCCESS_WITH_INFO) {
                update_diagnostics();
                return _RC_UNDEFINED_COLUMNS;
            }

            // build column metadata
            std::string column_name(reinterpret_cast<const char*>(column_name_buffer.data()), column_name_length);
            auto it = m_column_map.find(column_name);
            if (it != m_column_map.end())
                return _RC_DUPLICATE_COLUMNS;

            m_column_map.emplace(column_name, static_cast<std::uint8_t>(i));
            m_columns.push_back(SimpleSqlTypes::ColumnMetadata(
                    static_cast<std::uint16_t>(i),
                    column_name,
                    ODBC_SQL_to_SQLDataType(odbc_sql_type),
                    static_cast<std::uint64_t>(data_size),
                    static_cast<std::uint16_t>(precision),
                    ODBC_NULL_to_NullRuleType(null_id)
                )
            );

            SQLSMALLINT odbc_c_type;
            if (!OBDC_SQL_to_ODBC_C(odbc_sql_type, odbc_c_type))
                return _RC_BIND_COLUMN;

            BindingFamily family;
            if (!get_binding_family(m_columns.back().sim_data_type(), family))
                return _RC_UNKNOWN_BINDING_FAMILY;

            switch (family) {
            case BindingFamily::STRING_UTF8:
                sr = bind_string_column<std::string, SQLCHAR>(h_stmt->get(), i + 1, odbc_c_type, m_columns.back(), m_column_buffer);
                break;
            case BindingFamily::STRING_UTF16:
                sr = bind_string_column<std::wstring, SQLWCHAR>(h_stmt->get(), i + 1, odbc_c_type, m_columns.back(), m_column_buffer);
                break;
            case BindingFamily::NUMERIC:
                switch (m_columns.back().sim_data_type()) {
                case SimpleSqlTypes::SQLDataType::FLOAT:
                    sr = bind_fixed_size_column<float>(h_stmt->get(), i + 1, odbc_c_type, m_columns.back(), m_column_buffer);
                    break;
                case SimpleSqlTypes::SQLDataType::DOUBLE:
                    sr = bind_fixed_size_column<double>(h_stmt->get(), i + 1, odbc_c_type, m_columns.back(), m_column_buffer);
                    break;
                default:
                    return _RC_NUMERIC_BIND;
                }
                break;
            case BindingFamily::BOOL_INT:
                switch (m_columns.back().sim_data_type()) {
                case SimpleSqlTypes::SQLDataType::BOOLEAN:
                    sr = bind_fixed_size_column<bool>(h_stmt->get(), i + 1, odbc_c_type, m_columns.back(), m_column_buffer);
                    break;
                case SimpleSqlTypes::SQLDataType::INT_8:
                    sr = bind_fixed_size_column<std::int8_t>(h_stmt->get(), i + 1, odbc_c_type, m_columns.back(), m_column_buffer);
                    break;
                case SimpleSqlTypes::SQLDataType::INT_16:
                    sr = bind_fixed_size_column<std::int16_t>(h_stmt->get(), i + 1, odbc_c_type, m_columns.back(), m_column_buffer);
                    break;
                case SimpleSqlTypes::SQLDataType::INT_32:
                    sr = bind_fixed_size_column<std::int32_t>(h_stmt->get(), i + 1, odbc_c_type, m_columns.back(), m_column_buffer);
                    break;
                case SimpleSqlTypes::SQLDataType::INT_64:
                    sr = bind_fixed_size_column<std::int64_t>(h_stmt->get(), i + 1, odbc_c_type, m_columns.back(), m_column_buffer);
                    break;
                default:
                    return _RC_BOOL_INT_BIND;
                }
                break;
            case BindingFamily::GUID:
                switch (m_columns.back().sim_data_type()) {
                case SimpleSqlTypes::SQLDataType::ODBC_GUID:
                    sr = bind_odbc_guid_column(h_stmt->get(), i + 1, odbc_c_type, m_columns.back(), m_column_buffer);
                    break;
                case SimpleSqlTypes::SQLDataType::GUID:
                    sr = bind_guid_column(h_stmt->get(), i + 1, odbc_c_type, m_columns.back(), m_column_buffer);
                    break;
                default:
                    return _RC_GUID_BIND;
                }
                break;
            case BindingFamily::DATETIME:
                switch (m_columns.back().sim_data_type()) {
                case SimpleSqlTypes::SQLDataType::DATETIME:
                    sr = bind_datetime_column<SimpleSqlTypes::Datetime>(h_stmt->get(), i + 1, odbc_c_type, m_columns.back(), m_column_buffer);
                    break;
                case SimpleSqlTypes::SQLDataType::DATE:
                    sr = bind_datetime_column<SimpleSqlTypes::Date>(h_stmt->get(), i + 1, odbc_c_type, m_columns.back(), m_column_buffer);
                    break;
                case SimpleSqlTypes::SQLDataType::TIME:
                    sr = bind_datetime_column<SimpleSqlTypes::Time>(h_stmt->get(), i + 1, odbc_c_type, m_columns.back(), m_column_buffer);
                    break;
                default:
                    return _RC_DATETIME_BIND;
                }
                break;
            case BindingFamily::BLOB:
                sr = bind_blob_column(h_stmt->get(), i + 1, odbc_c_type, m_columns.back(), m_column_buffer);
                break;
            }
        }
        return _RC_SUCCESS;
    };

    SQLSMALLINT column_count = 0;
    SQLRETURN sr = SQLNumResultCols(h_stmt->get(), &column_count);
    if (sr != SQL_SUCCESS && sr != SQL_SUCCESS_WITH_INFO) {
        update_diagnostics();
        return _RC_CALC_COLUMNS;
    }

    if (column_count > 0) {
        m_is_select = true;
        std::uint8_t return_code = get_columns(column_count);
        if (return_code > 0)
            return return_code;

        m_matrix.make_valid(column_count);
    } else {
        m_is_select = false;
    }
    return _RC_SUCCESS;
}

void SimpleSql::SimQuery::update_diagnostics() {
    SQLSMALLINT current_record_number = static_cast<SQLSMALLINT>(m_diagnostic_record_number);
    std::vector<SQLCHAR> sql_state(6);
    SQLINTEGER native_error;
    std::vector<SQLCHAR> message(SQL_MAX_MESSAGE_LENGTH);
    SQLSMALLINT message_length;
    while (true) {
        bool exit_condition = false;
        switch (SQLGetDiagRec(SQL_HANDLE_STMT, h_stmt->get(), current_record_number, sql_state.data(), &native_error, message.data(), sizeof(message), &message_length)) {
        case SQL_SUCCESS: break;
        case SQL_SUCCESS_WITH_INFO: break;
        case SQL_NO_DATA: exit_condition = true; break;
        default: exit_condition = true; break;
        }
        if (exit_condition)
            break;

        m_diagnostics.push_back(SimpleSqlTypes::DiagnosticRecord(
                static_cast<std::int16_t>(current_record_number),
                std::string(reinterpret_cast<const char*>(sql_state.data()), 6),
                static_cast<std::int32_t>(native_error),
                std::string(reinterpret_cast<const char*>(message.data()), message_length)
            )
        );
        current_record_number++;
    }
    m_diagnostic_record_number = current_record_number;
}

// bool SimpleSql::SimQuery::claim_handle(SimpleSqlTypes::STMT_HANDLE&& stmt_handle) {
//     if (!stmt_handle) {
//         m_is_valid = false;
//         return false;
//     }

//     h_stmt = std::move(stmt_handle);
//     return true;
// }

// SimpleSqlTypes::STMT_HANDLE&& SimpleSql::SimQuery::return_handle() {
//     return std::move(h_stmt);
// }

void SimpleSql::SimQuery::set_handle(SimpleSqlTypes::STMT_HANDLE* handle) {
    if (handle) {
        h_stmt = handle;
        m_is_valid = true;
    } else {
        m_is_valid = false;
    }
}

void SimpleSql::SimQuery::set_batch_size(const std::uint64_t& batch_size) {
    if (m_batch_size != batch_size)
        m_batch_size = batch_size;
}

std::uint8_t SimpleSql::SimQuery::set_sql(const std::string& sql) {
    if (sql.empty())
        return _RC_EMPTY_SQL;

    m_sql = sql;
    return _RC_SUCCESS;
}

std::uint8_t SimpleSql::SimQuery::prepare() {

    SQLULEN array_size = static_cast<SQLULEN>(m_batch_size);
    SQLRETURN sr = SQLSetStmtAttr(h_stmt->get(), SQL_ATTR_ROW_ARRAY_SIZE, (SQLPOINTER)array_size, 0);
    if (sr != SQL_SUCCESS && sr != SQL_SUCCESS_WITH_INFO) {
        update_diagnostics();
        return _RC_ROW_SIZE;
    }

    SQLCHAR* sql = reinterpret_cast<SQLCHAR*>(const_cast<char*>(m_sql.c_str()));
    sr = SQLPrepare(h_stmt->get(), sql, SQL_NTS);
    if (sr != SQL_SUCCESS && sr != SQL_SUCCESS_WITH_INFO) {
        update_diagnostics();
        return _RC_PREPARE;
    }

    return define_columns();
}

std::uint8_t SimpleSql::SimQuery::bind_parameter(SimpleSqlTypes::SQLBinding& binding) {

    SQLSMALLINT parameter_count;
    SQLRETURN sr = SQLNumParams(h_stmt->get(), &parameter_count);
    if (sr != SQL_SUCCESS && sr != SQL_SUCCESS_WITH_INFO)
        return _RC_PARAMETER_CALC;

    if (parameter_count < 1)
        return _RC_NO_PARAMETERS;

    SQLUSMALLINT parameter_index = static_cast<SQLUSMALLINT>(m_binding_index);

    SQLSMALLINT io_type;
    if (!get_io_type(binding.type(), io_type))
        return _RC_UNKNOWN_IO_TYPE;

    BindingFamily family;
    if (!get_binding_family(binding.data_type(), family))
        return _RC_UNKNOWN_BINDING_FAMILY;

    SQLSMALLINT c_data_type, sql_data_type;
    if (!get_odbc_data_types(binding.data_type(), c_data_type, sql_data_type))
        return _RC_UNKNOWN_SQL_C_TYPE;

    switch (family) {
    case BindingFamily::STRING_UTF8:
        sr = bind_string_parameter<std::string, SQLCHAR>(h_stmt->get(), parameter_index, io_type, c_data_type, sql_data_type, binding, m_parameter_buffer);
        break;
    case BindingFamily::STRING_UTF16:
        sr = bind_string_parameter<std::wstring, SQLWCHAR>(h_stmt->get(), parameter_index, io_type, c_data_type, sql_data_type, binding, m_parameter_buffer);
        break;
    case BindingFamily::NUMERIC:
        switch (binding.data_type()) {
        case SimpleSqlTypes::SQLDataType::FLOAT:
            sr = bind_fixed_size_parameter<float>(h_stmt->get(), parameter_index, io_type, c_data_type, sql_data_type, binding, m_parameter_buffer);
            break;
        case SimpleSqlTypes::SQLDataType::DOUBLE:
            sr = bind_fixed_size_parameter<double>(h_stmt->get(), parameter_index, io_type, c_data_type, sql_data_type, binding, m_parameter_buffer);
            break;
        default:
            return _RC_NUMERIC_BIND;
        }
        break;
    case BindingFamily::BOOL_INT:
        switch (binding.data_type()) {
        case SimpleSqlTypes::SQLDataType::BOOLEAN:
            sr = bind_fixed_size_parameter<bool>(h_stmt->get(), parameter_index, io_type, c_data_type, sql_data_type, binding, m_parameter_buffer);
            break;
        case SimpleSqlTypes::SQLDataType::INT_8:
            sr = bind_fixed_size_parameter<std::int8_t>(h_stmt->get(), parameter_index, io_type, c_data_type, sql_data_type, binding, m_parameter_buffer);
            break;
        case SimpleSqlTypes::SQLDataType::INT_16:
            sr = bind_fixed_size_parameter<std::int16_t>(h_stmt->get(), parameter_index, io_type, c_data_type, sql_data_type, binding, m_parameter_buffer);
            break;
        case SimpleSqlTypes::SQLDataType::INT_32:
            sr = bind_fixed_size_parameter<std::int32_t>(h_stmt->get(), parameter_index, io_type, c_data_type, sql_data_type, binding, m_parameter_buffer);
            break;
        case SimpleSqlTypes::SQLDataType::INT_64:
            sr = bind_fixed_size_parameter<std::int64_t>(h_stmt->get(), parameter_index, io_type, c_data_type, sql_data_type, binding, m_parameter_buffer);
            break;
        default:
            return _RC_BOOL_INT_BIND;
        }
        break;
    case BindingFamily::GUID:
        switch (binding.data_type()) {
        case SimpleSqlTypes::SQLDataType::ODBC_GUID:
            sr = bind_odbc_guid_parameter(h_stmt->get(), parameter_index, io_type, c_data_type, sql_data_type, binding, m_parameter_buffer);
            break;
        case SimpleSqlTypes::SQLDataType::GUID:
            sr = bind_guid_parameter(h_stmt->get(), parameter_index, io_type, c_data_type, sql_data_type, binding, m_parameter_buffer);
            break;
        default:
            return _RC_GUID_BIND;
        }
        break;
    case BindingFamily::DATETIME:
        switch (binding.data_type()) {
        case SimpleSqlTypes::SQLDataType::DATETIME:
            sr = bind_datetime_parameter<SimpleSqlTypes::Datetime>(h_stmt->get(), parameter_index, io_type, c_data_type, sql_data_type, binding, m_parameter_buffer);
            break;
        case SimpleSqlTypes::SQLDataType::DATE:
            sr = bind_datetime_parameter<SimpleSqlTypes::Date>(h_stmt->get(), parameter_index, io_type, c_data_type, sql_data_type, binding, m_parameter_buffer);
            break;
        case SimpleSqlTypes::SQLDataType::TIME:
            sr = bind_datetime_parameter<SimpleSqlTypes::Time>(h_stmt->get(), parameter_index, io_type, c_data_type, sql_data_type, binding, m_parameter_buffer);
            break;
        default:
            return _RC_DATETIME_BIND;
        }
        break;
    case BindingFamily::BLOB:
        sr = bind_blob_parameter(h_stmt->get(), parameter_index, io_type, c_data_type, sql_data_type, binding, m_parameter_buffer);
        break;
    }

    if (sr != SQL_SUCCESS && sr != SQL_SUCCESS_WITH_INFO) {
        update_diagnostics();
        return _RC_BINDING;
    }
    m_binding_index++;
    return _RC_SUCCESS;
}

void SimpleSql::SimQuery::add_parameter(const SimpleSqlTypes::SQLBinding& binding) {
    m_parameters.push_back(binding);
}

void SimpleSql::SimQuery::bind_parameters() {
    for (SimpleSqlTypes::SQLBinding& binding : m_parameters) {
        bind_parameter(binding);
    }
}

bool SimpleSql::SimQuery::execute(const bool& autofinish) {

    // run SQLExecute
    switch (SQLExecute(h_stmt->get())) {
    case SQL_SUCCESS:
        break;
    case SQL_SUCCESS_WITH_INFO:
        update_diagnostics();
        break;
    default:
        update_diagnostics();
        return false;
    }

    // get select results
    while (true) {
        bool exit_condition = false;
        switch (SQLFetch(h_stmt->get())) {
        case SQL_SUCCESS:
            break;
        case SQL_SUCCESS_WITH_INFO:
            update_diagnostics();
            break;
        case SQL_NO_DATA:
            exit_condition = true;
            break;
        default:
            update_diagnostics();
            return false;
        }
        if (exit_condition)
            break;

        if (!m_column_buffer.empty()) {
            std::vector<SimpleSqlTypes::SQLCell> row;
            for (SimpleSqlTypes::SQLBoundOutput& output : m_column_buffer) {
                row.push_back(SimpleSqlTypes::SQLCell(
                    output.binding().data(),
                    output.binding().data_type(),
                    output.buffer_size() == SQL_NULL_DATA
                ));
            }
            m_matrix.add_row(std::move(row));
        }
    }

    // get output parameters
    switch (SQLMoreResults(h_stmt->get())) {
    case SQL_SUCCESS:
        for (SimpleSqlTypes::SQLBoundOutput& output : m_parameter_buffer) {
            m_key_data.emplace(output.binding().name(), SimpleSqlTypes::SQLCell(
                output.binding().data(),
                output.binding().data_type(),
                output.buffer_size() == SQL_NULL_DATA
            ));
        }
        break;
    case SQL_SUCCESS_WITH_INFO:
        for (SimpleSqlTypes::SQLBoundOutput& output : m_parameter_buffer) {
            m_key_data.emplace(output.binding().name(), SimpleSqlTypes::SQLCell(
                output.binding().data(),
                output.binding().data_type(),
                output.buffer_size() == SQL_NULL_DATA
            ));
        }
        update_diagnostics();
        break;
    case SQL_NO_DATA:
        break;
    default:
        update_diagnostics();
        return false;
    }

    if (autofinish)
        finish();

    return true;
}

void SimpleSql::SimQuery::finish() {

    // free statement: close
    switch (SQLFreeStmt(h_stmt->get(), SQL_CLOSE)) {
    case SQL_SUCCESS:
        break;
    case SQL_SUCCESS_WITH_INFO:
        update_diagnostics();
        break;
    default:
        update_diagnostics();
        break;
    }

    // free statement: reset params
    switch (SQLFreeStmt(h_stmt->get(), SQL_RESET_PARAMS)) {
    case SQL_SUCCESS:
        break;
    case SQL_SUCCESS_WITH_INFO:
        update_diagnostics();
        break;
    default:
        update_diagnostics();
        break;
    }

    // free statement: unbind
    switch (SQLFreeStmt(h_stmt->get(), SQL_UNBIND)) {
    case SQL_SUCCESS:
        break;
    case SQL_SUCCESS_WITH_INFO:
        update_diagnostics();
        break;
    default:
        update_diagnostics();
        break;
    }
}

void SimpleSql::SimQuery::destroy() {

}