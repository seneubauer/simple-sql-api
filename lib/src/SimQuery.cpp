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
    NOT_SET         = 0,
    STRING_UTF8     = 1,
    STRING_UTF16    = 2,
    NUMERIC         = 3,
    BOOL_INT        = 4,
    GUID            = 5,
    DATETIME        = 6,
    BLOB            = 7
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

static bool get_odbc_data_types(const SimpleSqlTypes::SimDataType& data_type, SQLSMALLINT& odbc_c_type, SQLSMALLINT& odbc_sql_type) {

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
    case SimpleSqlTypes::SimDataType::ODBC_GUID:
        odbc_c_type = SQL_C_BINARY;
        odbc_sql_type = SQL_BINARY;
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

static bool get_binding_family(const SimpleSqlTypes::SimDataType& data_type, BindingFamily& family) {

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
    family = data_type ^ SimpleSqlTypes::SimDataType::ODBC_GUID &&
             data_type ^ SimpleSqlTypes::SimDataType::GUID ? BindingFamily::GUID : family;

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

static SimpleSqlTypes::SimDataType ODBC_SQL_to_SQLDataType(const SQLSMALLINT& data_type) {
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
    SimpleSqlTypes::SQLData data;
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
    SimpleSqlTypes::SQLData data;
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
    SimpleSqlTypes::SQLData data;
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
    SimpleSqlTypes::SQLData data;
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
    SimpleSqlTypes::SQLData data;
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
    SimpleSqlTypes::SQLData data;
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
        for (SQLUSMALLINT i = 1; i < column_count; ++i) {
            std::vector<SQLCHAR> column_name_buffer(SimpleSqlConstants::Limits::max_sql_column_name_size);
            SQLSMALLINT column_name_length;
            SQLSMALLINT odbc_sql_type;
            SQLULEN data_size;
            SQLSMALLINT precision;
            SQLSMALLINT null_id;
            SQLRETURN sr = SQLDescribeCol(h_stmt.get(), i, column_name_buffer.data(), SimpleSqlConstants::Limits::max_sql_column_name_size, &column_name_length, &odbc_sql_type, &data_size, &precision, &null_id);
            if (sr != SQL_SUCCESS && sr != SQL_SUCCESS_WITH_INFO) {
                define_diagnostics();
                return _RC_UNDEFINED_COLUMNS;
            }

            // build column metadata
            std::string column_name(reinterpret_cast<const char*>(column_name_buffer.data()), column_name_length);
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
                sr = bind_string_column<std::string, SQLCHAR>(h_stmt.get(), i, odbc_c_type, m_columns.back(), m_column_buffer);
                break;
            case BindingFamily::STRING_UTF16:
                sr = bind_string_column<std::wstring, SQLWCHAR>(h_stmt.get(), i, odbc_c_type, m_columns.back(), m_column_buffer);
                break;
            case BindingFamily::NUMERIC:
                switch (m_columns.back().sim_data_type()) {
                case SimpleSqlTypes::SimDataType::FLOAT:
                    sr = bind_fixed_size_column<float>(h_stmt.get(), i, odbc_c_type, m_columns.back(), m_column_buffer);
                    break;
                case SimpleSqlTypes::SimDataType::DOUBLE:
                    sr = bind_fixed_size_column<double>(h_stmt.get(), i, odbc_c_type, m_columns.back(), m_column_buffer);
                    break;
                default:
                    return _RC_NUMERIC_BIND;
                }
                break;
            case BindingFamily::BOOL_INT:
                switch (m_columns.back().sim_data_type()) {
                case SimpleSqlTypes::SimDataType::BOOLEAN:
                    sr = bind_fixed_size_column<bool>(h_stmt.get(), i, odbc_c_type, m_columns.back(), m_column_buffer);
                    break;
                case SimpleSqlTypes::SimDataType::INT_8:
                    sr = bind_fixed_size_column<std::int8_t>(h_stmt.get(), i, odbc_c_type, m_columns.back(), m_column_buffer);
                    break;
                case SimpleSqlTypes::SimDataType::INT_16:
                    sr = bind_fixed_size_column<std::int16_t>(h_stmt.get(), i, odbc_c_type, m_columns.back(), m_column_buffer);
                    break;
                case SimpleSqlTypes::SimDataType::INT_32:
                    sr = bind_fixed_size_column<std::int32_t>(h_stmt.get(), i, odbc_c_type, m_columns.back(), m_column_buffer);
                    break;
                case SimpleSqlTypes::SimDataType::INT_64:
                    sr = bind_fixed_size_column<std::int64_t>(h_stmt.get(), i, odbc_c_type, m_columns.back(), m_column_buffer);
                    break;
                default:
                    return _RC_BOOL_INT_BIND;
                }
                break;
            case BindingFamily::GUID:
                switch (m_columns.back().sim_data_type()) {
                case SimpleSqlTypes::SimDataType::ODBC_GUID:
                    sr = bind_odbc_guid_column(h_stmt.get(), i, odbc_c_type, m_columns.back(), m_column_buffer);
                    break;
                case SimpleSqlTypes::SimDataType::GUID:
                    sr = bind_guid_column(h_stmt.get(), i, odbc_c_type, m_columns.back(), m_column_buffer);
                    break;
                default:
                    return _RC_GUID_BIND;
                }
                break;
            case BindingFamily::DATETIME:
                switch (m_columns.back().sim_data_type()) {
                case SimpleSqlTypes::SimDataType::DATETIME:
                    sr = bind_datetime_column<SimpleSqlTypes::_Datetime>(h_stmt.get(), i, odbc_c_type, m_columns.back(), m_column_buffer);
                    break;
                case SimpleSqlTypes::SimDataType::DATE:
                    sr = bind_datetime_column<SimpleSqlTypes::_Date>(h_stmt.get(), i, odbc_c_type, m_columns.back(), m_column_buffer);
                    break;
                case SimpleSqlTypes::SimDataType::TIME:
                    sr = bind_datetime_column<SimpleSqlTypes::_Time>(h_stmt.get(), i, odbc_c_type, m_columns.back(), m_column_buffer);
                    break;
                default:
                    return _RC_DATETIME_BIND;
                }
                break;
            case BindingFamily::BLOB:
                sr = bind_blob_column(h_stmt.get(), i, odbc_c_type, m_columns.back(), m_column_buffer);
                break;
            case BindingFamily::NOT_SET:
                return _RC_BINDING_NOT_SET;
            }
        }
        return _RC_SUCCESS;
    };

    SQLSMALLINT column_count = 0;
    SQLRETURN sr = SQLNumResultCols(h_stmt.get(), &column_count);
    if (sr != SQL_SUCCESS && sr != SQL_SUCCESS_WITH_INFO) {
        define_diagnostics();
        return _RC_CALC_COLUMNS;
    }

    if (column_count > 0) {
        m_is_select = true;
        std::uint8_t return_code = get_columns(column_count);
        if (return_code > 0)
            return return_code;

        if (column_count != static_cast<SQLSMALLINT>(m_column_map.size()))
            return _RC_DUPLICATE_COLUMNS;

        m_matrix.make_valid(column_count);
    } else {
        m_is_select = false;
    }
    return _RC_SUCCESS;
}

void SimpleSql::SimQuery::define_diagnostics() {
    SQLSMALLINT current_record_number = static_cast<SQLSMALLINT>(m_diagnostic_record_number);
    std::vector<SQLCHAR> sql_state(6);
    SQLINTEGER native_error;
    std::vector<SQLCHAR> message(SQL_MAX_MESSAGE_LENGTH);
    SQLSMALLINT message_length;
    SQLRETURN rc = SQLGetDiagRec(SQL_HANDLE_STMT, h_stmt.get(), current_record_number, sql_state.data(), &native_error, message.data(), sizeof(message), &message_length);
    while (rc != SQL_NO_DATA || rc != SQL_ERROR || rc != SQL_INVALID_HANDLE) {
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

bool SimpleSql::SimQuery::claim_handle(SimpleSqlTypes::STMT_HANDLE&& stmt_handle) {
    if (!stmt_handle) {
        m_is_valid = false;
        return false;
    }

    h_stmt = std::move(stmt_handle);
    return true;
}

SimpleSqlTypes::STMT_HANDLE&& SimpleSql::SimQuery::return_handle() {
    return std::move(h_stmt);
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
    SQLRETURN sr = SQLSetStmtAttr(h_stmt.get(), SQL_ATTR_ROW_ARRAY_SIZE, (SQLPOINTER)array_size, 0);
    if (sr != SQL_SUCCESS && sr != SQL_SUCCESS_WITH_INFO)
        return _RC_ROW_SIZE;

    SQLCHAR* sql = reinterpret_cast<SQLCHAR*>(const_cast<char*>(m_sql.c_str()));
    sr = SQLPrepare(h_stmt.get(), sql, SQL_NTS);
    if (sr != SQL_SUCCESS && sr != SQL_SUCCESS_WITH_INFO)
        return _RC_PREPARE;

    return define_columns();
}

std::uint8_t SimpleSql::SimQuery::bind_parameter(SimpleSqlTypes::SQLBinding& binding) {

    SQLSMALLINT parameter_count;
    SQLRETURN sr = SQLNumParams(h_stmt.get(), &parameter_count);
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
        sr = bind_string_parameter<std::string, SQLCHAR>(h_stmt.get(), parameter_index, io_type, c_data_type, sql_data_type, binding, m_parameter_buffer);
        break;
    case BindingFamily::STRING_UTF16:
        sr = bind_string_parameter<std::wstring, SQLWCHAR>(h_stmt.get(), parameter_index, io_type, c_data_type, sql_data_type, binding, m_parameter_buffer);
        break;
    case BindingFamily::NUMERIC:
        switch (binding.data_type()) {
        case SimpleSqlTypes::SimDataType::FLOAT:
            sr = bind_fixed_size_parameter<float>(h_stmt.get(), parameter_index, io_type, c_data_type, sql_data_type, binding, m_parameter_buffer);
            break;
        case SimpleSqlTypes::SimDataType::DOUBLE:
            sr = bind_fixed_size_parameter<double>(h_stmt.get(), parameter_index, io_type, c_data_type, sql_data_type, binding, m_parameter_buffer);
            break;
        default:
            return _RC_NUMERIC_BIND;
        }
        break;
    case BindingFamily::BOOL_INT:
        switch (binding.data_type()) {
        case SimpleSqlTypes::SimDataType::BOOLEAN:
            sr = bind_fixed_size_parameter<bool>(h_stmt.get(), parameter_index, io_type, c_data_type, sql_data_type, binding, m_parameter_buffer);
            break;
        case SimpleSqlTypes::SimDataType::INT_8:
            sr = bind_fixed_size_parameter<std::int8_t>(h_stmt.get(), parameter_index, io_type, c_data_type, sql_data_type, binding, m_parameter_buffer);
            break;
        case SimpleSqlTypes::SimDataType::INT_16:
            sr = bind_fixed_size_parameter<std::int16_t>(h_stmt.get(), parameter_index, io_type, c_data_type, sql_data_type, binding, m_parameter_buffer);
            break;
        case SimpleSqlTypes::SimDataType::INT_32:
            sr = bind_fixed_size_parameter<std::int32_t>(h_stmt.get(), parameter_index, io_type, c_data_type, sql_data_type, binding, m_parameter_buffer);
            break;
        case SimpleSqlTypes::SimDataType::INT_64:
            sr = bind_fixed_size_parameter<std::int64_t>(h_stmt.get(), parameter_index, io_type, c_data_type, sql_data_type, binding, m_parameter_buffer);
            break;
        default:
            return _RC_BOOL_INT_BIND;
        }
        break;
    case BindingFamily::GUID:
        switch (binding.data_type()) {
        case SimpleSqlTypes::SimDataType::ODBC_GUID:
            sr = bind_odbc_guid_parameter(h_stmt.get(), parameter_index, io_type, c_data_type, sql_data_type, binding, m_parameter_buffer);
            break;
        case SimpleSqlTypes::SimDataType::GUID:
            sr = bind_guid_parameter(h_stmt.get(), parameter_index, io_type, c_data_type, sql_data_type, binding, m_parameter_buffer);
            break;
        default:
            return _RC_GUID_BIND;
        }
        break;
    case BindingFamily::DATETIME:
        switch (binding.data_type()) {
        case SimpleSqlTypes::SimDataType::DATETIME:
            sr = bind_datetime_parameter<SimpleSqlTypes::_Datetime>(h_stmt.get(), parameter_index, io_type, c_data_type, sql_data_type, binding, m_parameter_buffer);
            break;
        case SimpleSqlTypes::SimDataType::DATE:
            sr = bind_datetime_parameter<SimpleSqlTypes::_Date>(h_stmt.get(), parameter_index, io_type, c_data_type, sql_data_type, binding, m_parameter_buffer);
            break;
        case SimpleSqlTypes::SimDataType::TIME:
            sr = bind_datetime_parameter<SimpleSqlTypes::_Time>(h_stmt.get(), parameter_index, io_type, c_data_type, sql_data_type, binding, m_parameter_buffer);
            break;
        default:
            return _RC_DATETIME_BIND;
        }
        break;
    case BindingFamily::BLOB:
        sr = bind_blob_parameter(h_stmt.get(), parameter_index, io_type, c_data_type, sql_data_type, binding, m_parameter_buffer);
        break;
    case BindingFamily::NOT_SET:
        return _RC_BINDING_NOT_SET;
    }

    if (sr != SQL_SUCCESS && sr != SQL_SUCCESS_WITH_INFO) {
        define_diagnostics();
        return _RC_BINDING;
    }
    m_binding_index++;
    return _RC_SUCCESS;
}

bool SimpleSql::SimQuery::execute_non_select() {

    return true;
}

bool SimpleSql::SimQuery::execute_select() {

    // run SQLExecute

    // iterate calls with SQLFetch until it returns SQL_NO_DATA, the bound values will update so copy those to the SQLMatrix
    while (SQLFetch(h_stmt.get())) {


    }

    return true;
}

bool SimpleSql::SimQuery::execute() {

    // bind columns 
    SQLRETURN sr = SQLExecute(h_stmt.get());
    if (sr != SQL_SUCCESS && sr != SQL_SUCCESS_WITH_INFO)
        return false;

    // populate m_matrix if it was a select query

    // populate key data if there are output parameters

    return true;
}

const size_t& SimpleSql::SimQuery::get_row_count(bool& invalid) const {
    if (!m_matrix.is_valid()) {
        invalid = true;
        return m_invalid_count;
    }

    invalid = false;
    return m_matrix.rows();
}

const size_t& SimpleSql::SimQuery::get_column_count(bool& invalid) const {
    if (!m_matrix.is_valid()) {
        invalid = true;
        return m_invalid_count;
    }

    invalid = false;
    return m_matrix.columns();
}

const SimpleSqlTypes::SQLCell& SimpleSql::SimQuery::get_cell(const std::string& key) const {
    auto iterator = m_key_data.find(key);
    if (iterator != m_key_data.end()) {
        return iterator->second;
    } else {
        return m_invalid_cell;
    }
}

const SimpleSqlTypes::SQLCell& SimpleSql::SimQuery::get_cell(const std::string& column, const size_t& row) const {
    if (!m_matrix.is_valid())
        return m_invalid_cell;

    auto iterator = m_column_map.find(column);
    if (iterator == m_column_map.end())
        return m_invalid_cell;

    size_t column_index = iterator->second;
    return get_cell(column_index, row);
}

const SimpleSqlTypes::SQLCell& SimpleSql::SimQuery::get_cell(const size_t& column, const size_t& row) const {
    if (column >= m_matrix.columns() || row >= m_matrix.rows() || !m_matrix.is_valid())
        return m_invalid_cell;

    return m_matrix.cells()[column + row * m_matrix.columns()];
}

const std::vector<SimpleSqlTypes::SQLCell>& SimpleSql::SimQuery::get_column(const std::string& column) const {
    auto iterator = m_column_map.find(column);
    if (iterator == m_column_map.end())
        return m_invalid_matrix;

    size_t column_index = iterator->second;
    return get_column(column_index);
}

const std::vector<SimpleSqlTypes::SQLCell>& SimpleSql::SimQuery::get_column(const size_t& column) const {
    if (column >= m_matrix.columns() || !m_matrix.is_valid())
        return m_invalid_matrix;

    auto column_count = m_matrix.columns();
    auto cells = m_matrix.cells();
    m_column.clear();
    for (size_t i = 0; i < m_matrix.rows(); ++i) {
        m_column.push_back(cells[i * column_count + column]);
    }
    return m_column;
}

const std::vector<SimpleSqlTypes::SQLCell>& SimpleSql::SimQuery::get_row(const size_t& row) const {
    if (row >= m_matrix.rows() || !m_matrix.is_valid())
        return m_invalid_matrix;

    auto it = m_matrix.cells().begin();
    size_t column_count = m_matrix.columns();

    m_row.clear();
    m_row.insert(m_row.begin(), it + row * column_count, it + row * column_count + column_count);

    return m_row;
}

const std::vector<SimpleSqlTypes::SQLCell>& SimpleSql::SimQuery::get_data() const {
    if (!m_matrix.is_valid())
        return m_invalid_matrix;

    return m_matrix.cells();
}

std::vector<SimpleSqlTypes::SQLCell> SimpleSql::SimQuery::claim_data() {
    return m_matrix.claim_cells();
}

void SimpleSql::SimQuery::destroy() {

}