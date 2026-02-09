// SimQL stuff
#include <SimQuery.hpp>
#include <SimValueSet.hpp>
#include <SimResultSet.hpp>
#include <SimDiagnosticSet.hpp>
#include <SimQL_Types.hpp>
#include <SimQL_Constants.hpp>
#include <SimQL_Strings.hpp>

// STL stuff
#include <cstdint>
#include <memory>
#include <utility>
#include <vector>
#include <array>
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

// private definitions

enum class BindingFamily : std::uint8_t {
    STRING          = 0,
    NUMERIC         = 1,
    INTEGER         = 2,
    OBDCGUID        = 3,
    GUID            = 4,
    TEMPORAL        = 5,
    BLOB            = 6
};

std::unordered_map<SimpleSqlTypes::SQLDataType, std::size_t> data_sizes {
    {SimpleSqlTypes::SQLDataType::DOUBLE, sizeof(SQLDOUBLE)},
    {SimpleSqlTypes::SQLDataType::INT, sizeof(INT)},
    {SimpleSqlTypes::SQLDataType::BOOLEAN, sizeof(BOOLEAN)},
    {SimpleSqlTypes::SQLDataType::DATETIME, sizeof(SimpleSqlTypes::_Datetime)},
    {SimpleSqlTypes::SQLDataType::DATE, sizeof(SimpleSqlTypes::_Date)},
    {SimpleSqlTypes::SQLDataType::TIME, sizeof(SimpleSqlTypes::_Time)}
};

// helper functions

static bool get_odbc_data_types(const SimpleSqlTypes::SQLDataType& data_type, SQLSMALLINT& odbc_c_type, SQLSMALLINT& odbc_sql_type) {

    switch (data_type) {
    case SimpleSqlTypes::SQLDataType::STRING:
        odbc_c_type = SQL_C_WCHAR;
        odbc_sql_type = SQL_WVARCHAR;
        break;
    case SimpleSqlTypes::SQLDataType::LONG_STRING:
        odbc_c_type = SQL_C_WCHAR;
        odbc_sql_type = SQL_WLONGVARCHAR;
        break;
    case SimpleSqlTypes::SQLDataType::DOUBLE:
        odbc_c_type = SQL_C_DOUBLE;
        odbc_sql_type = SQL_DOUBLE;
        break;
    case SimpleSqlTypes::SQLDataType::BOOLEAN:
        odbc_c_type = SQL_C_BIT;
        odbc_sql_type = SQL_BIT;
        break;
    case SimpleSqlTypes::SQLDataType::INT:
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

    // for strings
    if (data_type == SimpleSqlTypes::SQLDataType::STRING ||
        data_type == SimpleSqlTypes::SQLDataType::LONG_STRING) {

        family = BindingFamily::STRING;
        return true;
    }

    // for numeric types
    if (data_type == SimpleSqlTypes::SQLDataType::DOUBLE) {

        family = BindingFamily::NUMERIC;
        return true;
    }

    // for integer / boolean types
    if (data_type == SimpleSqlTypes::SQLDataType::BOOLEAN ||
        data_type == SimpleSqlTypes::SQLDataType::INT) {

        family = BindingFamily::INTEGER;
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

        family = BindingFamily::TEMPORAL;
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
        odbc_c_type = SQL_C_WCHAR;
        break;
    case SQL_LONGVARCHAR:
        odbc_c_type = SQL_C_WCHAR;
        break;
    case SQL_WVARCHAR:
        odbc_c_type = SQL_C_WCHAR;
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
    case SQL_CHAR:                      return SimpleSqlTypes::SQLDataType::STRING;
    case SQL_VARCHAR:                   return SimpleSqlTypes::SQLDataType::STRING;
    case SQL_WCHAR:                     return SimpleSqlTypes::SQLDataType::STRING;
    case SQL_WVARCHAR:                  return SimpleSqlTypes::SQLDataType::STRING;
    case SQL_WLONGVARCHAR:              return SimpleSqlTypes::SQLDataType::LONG_STRING;
    case SQL_FLOAT:                     return SimpleSqlTypes::SQLDataType::DOUBLE;
    case SQL_DOUBLE:                    return SimpleSqlTypes::SQLDataType::DOUBLE;
    case SQL_BIT:                       return SimpleSqlTypes::SQLDataType::BOOLEAN;
    case SQL_TINYINT:                   return SimpleSqlTypes::SQLDataType::INT;
    case SQL_SMALLINT:                  return SimpleSqlTypes::SQLDataType::INT;
    case SQL_INTEGER:                   return SimpleSqlTypes::SQLDataType::INT;
    case SQL_BIGINT:                    return SimpleSqlTypes::SQLDataType::INT;
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

static SQLRETURN bindparam(void* handle, const SQLUSMALLINT& index, const SQLSMALLINT& odbc_binding_type, const SQLSMALLINT& odbc_c_type, const SQLSMALLINT& odbc_sql_type, const SimpleSqlTypes::SQL_Binding& binding, std::vector<SimpleSqlTypes::SQL_Binding>& parameters) {
    BindingFamily family;
    if (!get_binding_family(binding.data_type, family))
        return SQL_ERROR;

    SimpleSqlTypes::SQL_Binding pbind;
    pbind.name = binding.name;
    pbind.data = binding.data;
    pbind.type = binding.type;
    pbind.data_type = binding.data_type;
    pbind.set_null = binding.set_null;

    SQLULEN column_size;
    SQLSMALLINT decimal_digits;
    SQLPOINTER p_val;
    SQLLEN buffer_length;

    switch (family) {
    case BindingFamily::STRING:
        {
            std::wstring str_val = std::get<std::wstring>(binding.data);
            if (!infer_parameter_metadata(handle, index, column_size, decimal_digits))
                column_size = static_cast<SQLULEN>(str_val.size() * sizeof(wchar_t)) + 1;

            decimal_digits = 0;
            buffer_length = column_size * sizeof(wchar_t);
            pbind.data = str_val;
            p_val = std::get<std::wstring>(pbind.data).data();
        }
        break;
    case BindingFamily::NUMERIC:
        column_size = data_sizes[pbind.data_type];
        decimal_digits = 0;
        buffer_length = data_sizes[pbind.data_type];
        p_val = &pbind.data;
        break;
    case BindingFamily::INTEGER:
        column_size = data_sizes[pbind.data_type];
        decimal_digits = 0;
        buffer_length = data_sizes[pbind.data_type];
        p_val = &pbind.data;
        break;
    case BindingFamily::OBDCGUID:
        column_size = 16;
        decimal_digits = 0;
        buffer_length = 16;
        p_val = &pbind.data;
        break;
    case BindingFamily::GUID:
        {
            SimpleSqlTypes::GUID& val = std::get<SimpleSqlTypes::GUID>(pbind.data);
            column_size = 16;
            decimal_digits = 0;
            buffer_length = 16;
            p_val = val.data();
        }
        break;
    case BindingFamily::TEMPORAL:
        column_size = 0;
        decimal_digits = 0;
        buffer_length = 0;
        if (pbind.data_type == SimpleSqlTypes::SQLDataType::DATETIME) {
            p_val = &std::get<SimpleSqlTypes::Datetime>(pbind.data).temporal;
        } else if (pbind.data_type == SimpleSqlTypes::SQLDataType::DATE) {
            p_val = &std::get<SimpleSqlTypes::Date>(pbind.data).temporal;
        } else if (pbind.data_type == SimpleSqlTypes::SQLDataType::TIME) {
            p_val = &std::get<SimpleSqlTypes::Time>(pbind.data).temporal;
        } else {
            return SQL_ERROR;
        }
        break;
    case BindingFamily::BLOB:
        {
            std::vector<std::uint8_t>& arr_val = std::get<std::vector<std::uint8_t>>(pbind.data);
            if (!infer_parameter_metadata(handle, index, column_size, decimal_digits))
                column_size = static_cast<SQLULEN>(arr_val.size());

            decimal_digits = 0;
            buffer_length = arr_val.size();
            p_val = arr_val.data();
        }
        break;
    }

    switch (odbc_binding_type) {
    case SQL_PARAM_INPUT_OUTPUT:
        if (family == BindingFamily::STRING) {
            pbind.indicator = pbind.set_null ? SQL_NULL_DATA : SQL_NTS;
        } else {
            pbind.indicator = pbind.set_null ? SQL_NULL_DATA : 0;
        }
        parameters.push_back(pbind);
        break;
    case SQL_PARAM_INPUT:
        if (family == BindingFamily::STRING) {
            pbind.indicator = pbind.set_null ? SQL_NULL_DATA : SQL_NTS;
        } else {
            pbind.indicator = pbind.set_null ? SQL_NULL_DATA : 0;
        }
        break;
    case SQL_PARAM_OUTPUT:
        pbind.indicator = 0;
        parameters.push_back(pbind);
        break;
    default:
        return SQL_ERROR;
    }

    return SQLBindParameter(handle, index, odbc_binding_type, odbc_c_type, odbc_sql_type, column_size, decimal_digits, p_val, buffer_length, &pbind.indicator);
}

static SQLRETURN bindcol(void* handle, const SQLUSMALLINT& index, const SQLSMALLINT& odbc_c_type, const SimpleSqlTypes::SQL_Column& column, std::vector<SimpleSqlTypes::SQL_Binding>& columns) {
    BindingFamily family;
    if (!get_binding_family(column.data_type, family))
        return SQL_ERROR;

    SQLLEN buffer_length;
    if (family == BindingFamily::STRING) {
        columns.emplace_back(SimpleSqlTypes::SQL_Binding {
            column.name,
            std::wstring(),
            SimpleSqlTypes::BindingType::OUTPUT,
            column.data_type,
            false,
            0
        });
        auto& col = columns.back();
        buffer_length = static_cast<SQLLEN>((column.size + 1) * sizeof(SQLWCHAR));
        auto& val = std::get<std::wstring>(col.data);
        val.resize(column.size + 1);
        SQLWCHAR* p_val = reinterpret_cast<SQLWCHAR*>(val.data());
        return SQLBindCol(handle, index, odbc_c_type, p_val, buffer_length, &col.indicator);
    } else if (family == BindingFamily::NUMERIC) {
        columns.emplace_back(SimpleSqlTypes::SQL_Binding {
            column.name,
            0.0,
            SimpleSqlTypes::BindingType::OUTPUT,
            column.data_type,
            false,
            0
        });
        auto& col = columns.back();
        buffer_length = sizeof(double);
        auto& val = std::get<double>(col.data);
        SQLPOINTER p_val = reinterpret_cast<SQLPOINTER>(&val);
        return SQLBindCol(handle, index, odbc_c_type, p_val, buffer_length, &col.indicator);
    } else if (family == BindingFamily::INTEGER) {
        columns.emplace_back(SimpleSqlTypes::SQL_Binding {
            column.name,
            0,
            SimpleSqlTypes::BindingType::OUTPUT,
            column.data_type,
            false,
            0
        });
        auto& col = columns.back();
        buffer_length = sizeof(int);
        auto& val = std::get<int>(col.data);
        SQLPOINTER p_val = reinterpret_cast<SQLPOINTER>(&val);
        return SQLBindCol(handle, index, odbc_c_type, p_val, buffer_length, &col.indicator);
    } else if (family == BindingFamily::OBDCGUID) {
        columns.emplace_back(SimpleSqlTypes::SQL_Binding {
            column.name,
            SimpleSqlTypes::ODBC_GUID(),
            SimpleSqlTypes::BindingType::OUTPUT,
            column.data_type,
            false,
            0
        });
        auto& col = columns.back();
        buffer_length = sizeof(SimpleSqlTypes::ODBC_GUID);
        auto& val = std::get<SimpleSqlTypes::ODBC_GUID>(col.data);
        SQLPOINTER p_val = reinterpret_cast<SQLPOINTER>(&val);
        return SQLBindCol(handle, index, odbc_c_type, p_val, buffer_length, &col.indicator);
    } else if (family == BindingFamily::GUID) {
        columns.emplace_back(SimpleSqlTypes::SQL_Binding {
            column.name,
            SimpleSqlTypes::GUID(),
            SimpleSqlTypes::BindingType::OUTPUT,
            column.data_type,
            false,
            0
        });
        auto& col = columns.back();
        buffer_length = sizeof(SimpleSqlTypes::GUID);
        auto& val = std::get<SimpleSqlTypes::GUID>(col.data);
        SQLPOINTER p_val = reinterpret_cast<SQLPOINTER>(val.data());
        return SQLBindCol(handle, index, odbc_c_type, p_val, buffer_length, &col.indicator);
    } else if (family == BindingFamily::TEMPORAL && column.data_type == SimpleSqlTypes::SQLDataType::DATETIME) {
        SimpleSqlTypes::_Datetime temporal_value;
        SimpleSqlTypes::BaseTemporal<SimpleSqlTypes::_Datetime> t(std::move(temporal_value), false);
        columns.emplace_back(SimpleSqlTypes::SQL_Binding {
            column.name,
            SimpleSqlTypes::Datetime(t),
            SimpleSqlTypes::BindingType::OUTPUT,
            column.data_type,
            false,
            0
        });
        auto& col = columns.back();
        buffer_length = sizeof(SimpleSqlTypes::BaseTemporal<SimpleSqlTypes::_Datetime>);
        auto& val = std::get<SimpleSqlTypes::Datetime>(col.data);
        SQLPOINTER p_val = reinterpret_cast<SQLPOINTER>(&val.temporal);
        return SQLBindCol(handle, index, odbc_c_type, p_val, buffer_length, &col.indicator);
    } else if (family == BindingFamily::TEMPORAL && column.data_type == SimpleSqlTypes::SQLDataType::DATE) {
        SimpleSqlTypes::_Date temporal_value;
        SimpleSqlTypes::BaseTemporal<SimpleSqlTypes::_Date> t(std::move(temporal_value), false);
        columns.emplace_back(SimpleSqlTypes::SQL_Binding {
            column.name,
            SimpleSqlTypes::Date(t),
            SimpleSqlTypes::BindingType::OUTPUT,
            column.data_type,
            false,
            0
        });
        auto& col = columns.back();
        buffer_length = sizeof(SimpleSqlTypes::BaseTemporal<SimpleSqlTypes::_Date>);
        auto& val = std::get<SimpleSqlTypes::Date>(col.data);
        SQLPOINTER p_val = reinterpret_cast<SQLPOINTER>(&val.temporal);
        return SQLBindCol(handle, index, odbc_c_type, p_val, buffer_length, &col.indicator);
    } else if (family == BindingFamily::TEMPORAL && column.data_type == SimpleSqlTypes::SQLDataType::TIME) {
        SimpleSqlTypes::_Time temporal_value;
        SimpleSqlTypes::BaseTemporal<SimpleSqlTypes::_Time> t(std::move(temporal_value), false);
        columns.emplace_back(SimpleSqlTypes::SQL_Binding {
            column.name,
            SimpleSqlTypes::Time(t),
            SimpleSqlTypes::BindingType::OUTPUT,
            column.data_type,
            false,
            0
        });
        auto& col = columns.back();
        buffer_length = sizeof(SimpleSqlTypes::BaseTemporal<SimpleSqlTypes::_Time>);
        auto& val = std::get<SimpleSqlTypes::Time>(col.data);
        SQLPOINTER p_val = reinterpret_cast<SQLPOINTER>(&val.temporal);
        return SQLBindCol(handle, index, odbc_c_type, p_val, buffer_length, &col.indicator);
    } else if (family == BindingFamily::BLOB) {
        columns.emplace_back(SimpleSqlTypes::SQL_Binding {
            column.name,
            std::vector<std::uint8_t>(),
            SimpleSqlTypes::BindingType::OUTPUT,
            column.data_type,
            false,
            0
        });
        auto& col = columns.back();
        buffer_length = static_cast<SQLLEN>(column.size);
        auto& val = std::get<std::vector<std::uint8_t>>(col.data);
        val.resize(column.size);
        return SQLBindCol(handle, index, odbc_c_type, val.data(), buffer_length, &col.indicator);
    } else {
        return SQL_ERROR;
    }
}

// public class definition

void SimpleSql::SimQuery::set_handle(SimpleSqlTypes::STMT_HANDLE* handle) {
    if (handle) {
        h_stmt = handle;
        m_is_valid = true;
    } else {
        m_is_valid = false;
    }
}

std::uint8_t SimpleSql::SimQuery::set_sql(const std::string& sql) {
    if (sql.empty())
        return _RC_EMPTY_SQL;

    m_sql = sql;
    return _RC_SUCCESS;
}

std::uint8_t SimpleSql::SimQuery::prepare() {
    std::wstring sql = SimpleSqlStrings::utf8_to_odbc(m_sql);
    switch (SQLPrepareW(h_stmt->get(), sql.data(), SQL_NTS)) {
    case SQL_SUCCESS:
        break;
    case SQL_SUCCESS_WITH_INFO:
        p_diagnostics->update(h_stmt->get(), SimpleSql::SimDiagnosticSet::HandleType::STMT);
        break;
    default:
        p_diagnostics->update(h_stmt->get(), SimpleSql::SimDiagnosticSet::HandleType::STMT);
        return _RC_PREPARE;
    }

    return define_columns();
}

std::uint8_t SimpleSql::SimQuery::bind_parameter(const SimpleSqlTypes::SQL_Binding& binding) {

    SQLSMALLINT count;
    switch (SQLNumParams(h_stmt->get(), &count)) {
    case SQL_SUCCESS:
        break;
    case SQL_SUCCESS_WITH_INFO:
        p_diagnostics->update(h_stmt->get(), SimpleSql::SimDiagnosticSet::HandleType::STMT);
        break;
    default:
        p_diagnostics->update(h_stmt->get(), SimpleSql::SimDiagnosticSet::HandleType::STMT);
        return _RC_PARAMETER_CALC;
    }

    if (count < 1)
        return _RC_NO_PARAMETERS;

    SQLUSMALLINT index = static_cast<SQLUSMALLINT>(m_binding_index);

    SQLSMALLINT io_type;
    if (!get_io_type(binding.type, io_type))
        return _RC_UNKNOWN_IO_TYPE;

    BindingFamily family;
    if (!get_binding_family(binding.data_type, family))
        return _RC_UNKNOWN_BINDING_FAMILY;

    SQLSMALLINT c_data_type, sql_data_type;
    if (!get_odbc_data_types(binding.data_type, c_data_type, sql_data_type))
        return _RC_UNKNOWN_SQL_C_TYPE;

    switch (bindparam(h_stmt->get(), index, io_type, c_data_type, sql_data_type, binding, m_bound_parameters)) {
    case SQL_SUCCESS:
        break;
    case SQL_SUCCESS_WITH_INFO:
        p_diagnostics->update(h_stmt->get(), SimpleSql::SimDiagnosticSet::HandleType::STMT);
        break;
    default:
        p_diagnostics->update(h_stmt->get(), SimpleSql::SimDiagnosticSet::HandleType::STMT);
        return _RC_BINDING;
    }

    m_binding_index++;
    return _RC_SUCCESS;
}

void SimpleSql::SimQuery::add_parameter(const SimpleSqlTypes::SQL_Binding& binding) {
    m_bound_parameters.push_back(binding);
}

void SimpleSql::SimQuery::bind_parameters() {
    for (SimpleSqlTypes::SQL_Binding& binding : m_bound_parameters) {
        bind_parameter(binding);
    }
}

bool SimpleSql::SimQuery::execute() {

    // run SQLExecute
    switch (SQLExecute(h_stmt->get())) {
    case SQL_SUCCESS:
        break;
    case SQL_SUCCESS_WITH_INFO:
        p_diagnostics->update(h_stmt->get(), SimpleSql::SimDiagnosticSet::HandleType::STMT);
        break;
    default:
        p_diagnostics->update(h_stmt->get(), SimpleSql::SimDiagnosticSet::HandleType::STMT);
        return false;
    }

    // get select results
    std::vector<SimpleSqlTypes::SQL_Value> values;
    while (true) {
        bool exit_condition = false;
        switch (SQLFetch(h_stmt->get())) {
        case SQL_SUCCESS:
            break;
        case SQL_SUCCESS_WITH_INFO:
            p_diagnostics->update(h_stmt->get(), SimpleSql::SimDiagnosticSet::HandleType::STMT);
            break;
        case SQL_NO_DATA:
            exit_condition = true;
            break;
        default:
            p_diagnostics->update(h_stmt->get(), SimpleSql::SimDiagnosticSet::HandleType::STMT);
            return false;
        }
        if (exit_condition)
            break;

        for (SimpleSqlTypes::SQL_Binding& column : m_bound_columns) {
            values.emplace_back(SimpleSqlTypes::SQL_Value {
                column.data,
                column.data_type,
                column.indicator == SQL_NULL_DATA
            });
        }
    }
    if (values.size() > 0)
        p_results->set_data(std::move(values));

    // get output parameters
    auto get_param = [&]() -> std::uint8_t {
        std::uint8_t output = 0;
        for (SimpleSqlTypes::SQL_Binding& parameter : m_bound_parameters) {
            std::uint8_t rc = p_values->add_value(
                parameter.name,
                std::move(SimpleSqlTypes::SQL_Value {
                    parameter.data,
                    parameter.data_type,
                    parameter.indicator == SQL_NULL_DATA
                })
            );
            if (rc > output)
                output = rc;
        }
        return output;
    };

    switch (SQLMoreResults(h_stmt->get())) {
    case SQL_SUCCESS:
        get_param();
        break;
    case SQL_SUCCESS_WITH_INFO:
        get_param();
        p_diagnostics->update(h_stmt->get(), SimpleSql::SimDiagnosticSet::HandleType::STMT);
        break;
    case SQL_NO_DATA:
        break;
    default:
        p_diagnostics->update(h_stmt->get(), SimpleSql::SimDiagnosticSet::HandleType::STMT);
        return false;
    }

    return true;
}

void SimpleSql::SimQuery::finish() {

    // free statement: close
    switch (SQLCloseCursor(h_stmt->get())) {
    case SQL_SUCCESS:
        break;
    case SQL_SUCCESS_WITH_INFO:
        p_diagnostics->update(h_stmt->get(), SimpleSql::SimDiagnosticSet::HandleType::STMT);
        break;
    default:
        p_diagnostics->update(h_stmt->get(), SimpleSql::SimDiagnosticSet::HandleType::STMT);
        break;
    }

    // free statement: reset params
    switch (SQLFreeStmt(h_stmt->get(), SQL_RESET_PARAMS)) {
    case SQL_SUCCESS:
        break;
    case SQL_SUCCESS_WITH_INFO:
        p_diagnostics->update(h_stmt->get(), SimpleSql::SimDiagnosticSet::HandleType::STMT);
        break;
    default:
        p_diagnostics->update(h_stmt->get(), SimpleSql::SimDiagnosticSet::HandleType::STMT);
        break;
    }

    // free statement: unbind
    switch (SQLFreeStmt(h_stmt->get(), SQL_UNBIND)) {
    case SQL_SUCCESS:
        break;
    case SQL_SUCCESS_WITH_INFO:
        p_diagnostics->update(h_stmt->get(), SimpleSql::SimDiagnosticSet::HandleType::STMT);
        break;
    default:
        p_diagnostics->update(h_stmt->get(), SimpleSql::SimDiagnosticSet::HandleType::STMT);
        break;
    }
    m_is_finished = true;
}

bool SimpleSql::SimQuery::is_valid() const {
    return m_is_valid;
}

bool SimpleSql::SimQuery::is_select() const {
    return m_is_select;
}

SimpleSql::SimResultSet* SimpleSql::SimQuery::results() {
    if (p_results)
        return p_results.get();

    return nullptr;
}

SimpleSql::SimDiagnosticSet* SimpleSql::SimQuery::diagnostics() {
    if (p_diagnostics)
        return p_diagnostics.get();

    return nullptr;
}

std::string_view SimpleSql::SimQuery::return_code_def(const std::uint8_t& return_code) {
    auto it = m_return_codes.find(return_code);
    if (it == m_return_codes.end())
        return std::string_view("invalid return code");

    return it->second;
}

// private class definition

std::uint8_t SimpleSql::SimQuery::define_columns() {

    auto get_columns = [&](SQLSMALLINT &column_count) -> std::uint8_t {
        for (SQLUSMALLINT i = 0; i < column_count; ++i) {
            std::vector<SQLWCHAR> column_name_buffer(SimpleSqlConstants::Limits::max_sql_column_name_size);
            SQLSMALLINT column_name_length;
            SQLSMALLINT odbc_sql_type;
            SQLULEN data_size;
            SQLSMALLINT precision;
            SQLSMALLINT null_id;
            switch(SQLDescribeColW(h_stmt->get(), i + 1, column_name_buffer.data(), SimpleSqlConstants::Limits::max_sql_column_name_size, &column_name_length, &odbc_sql_type, &data_size, &precision, &null_id)) {
            case SQL_SUCCESS:
                break;
            case SQL_SUCCESS_WITH_INFO:
                p_diagnostics->update(h_stmt->get(), SimpleSql::SimDiagnosticSet::HandleType::STMT);
                break;
            default:
                p_diagnostics->update(h_stmt->get(), SimpleSql::SimDiagnosticSet::HandleType::STMT);
                return _RC_UNDEFINED_COLUMNS;
            }

            // build column metadata
            SimpleSqlTypes::SQL_Column col {
                SimpleSqlStrings::odbc_to_utf8(std::wstring(column_name_buffer.data(), column_name_length)),
                static_cast<std::uint8_t>(i),
                ODBC_SQL_to_SQLDataType(odbc_sql_type),
                static_cast<std::uint64_t>(data_size),
                static_cast<std::int16_t>(precision),
                ODBC_NULL_to_NullRuleType(null_id)
            };
            if (p_results->add_column(col) > 0)
                return _RC_DUPLICATE_COLUMNS;

            SQLSMALLINT odbc_c_type;
            if (!OBDC_SQL_to_ODBC_C(odbc_sql_type, odbc_c_type))
                return _RC_BIND_COLUMN_DTYPE;

            BindingFamily family;
            if (!get_binding_family(col.data_type, family))
                return _RC_UNKNOWN_BINDING_FAMILY;

            switch (bindcol(h_stmt->get(), i + 1, odbc_c_type, col, m_bound_columns)) {
            case SQL_SUCCESS:
                break;
            case SQL_SUCCESS_WITH_INFO:
                p_diagnostics->update(h_stmt->get(), SimpleSql::SimDiagnosticSet::HandleType::STMT);
                break;
            default:
                p_diagnostics->update(h_stmt->get(), SimpleSql::SimDiagnosticSet::HandleType::STMT);
                return _RC_BIND_COLUMN;
            }
        }
        return _RC_SUCCESS;
    };

    SQLSMALLINT column_count = 0;
    switch (SQLNumResultCols(h_stmt->get(), &column_count)) {
    case SQL_SUCCESS:
        break;
    case SQL_SUCCESS_WITH_INFO:
        p_diagnostics->update(h_stmt->get(), SimpleSql::SimDiagnosticSet::HandleType::STMT);
        break;
    default:
        p_diagnostics->update(h_stmt->get(), SimpleSql::SimDiagnosticSet::HandleType::STMT);
        return _RC_CALC_COLUMNS;
    }

    if (column_count > 0) {
        m_is_select = true;
        m_bound_columns.reserve(column_count);
        std::uint8_t return_code = get_columns(column_count);
        if (return_code > 0)
            return return_code;

    } else {
        m_is_select = false;
    }
    return _RC_SUCCESS;
}