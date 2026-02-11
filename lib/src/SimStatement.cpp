// SimQL stuff
#include <SimStatement.hpp>
#include <SimConnection.hpp>
#include <SimQL_ReturnCodes.hpp>
#include <SimQL_Strings.hpp>

// STL stuff
#include <cstdint>
#include <memory>
#include <vector>
#include <map>
#include <variant>

// Windows stuff
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

namespace SimpleSql {

    extern void* get_dbc_handle(Connection& dbc) noexcept;

    struct Statement::handle {
        SQLHSTMT h_stmt;
        SimQL_ReturnCodes::Code return_code;
        SQLUSMALLINT pbind_index;

        using BoundValue = std::variant<
            SQLCHAR*,
            SQLWCHAR*,
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
            SQL_TIME_STRUCT
        >;
        struct BoundParameter {
            BoundValue value;
            SQLLEN indicator;
        };
        std::map<std::string, BoundParameter> bound_parameters;

        explicit handle(Connection& dbc, const Statement::Options& options) {

            SQLHDBC h_dbc = static_cast<SQLHDBC>(get_dbc_handle(dbc));
            h_stmt = SQL_NULL_HSTMT;
            return_code = SimQL_ReturnCodes::Code::SUCCESS;
            pbind_index = 1;

            // allocate the handle
            switch (SQLAllocHandle(SQL_HANDLE_STMT, h_dbc, &h_stmt)) {
            case SQL_SUCCESS:
                break;
            case SQL_SUCCESS_WITH_INFO:
                return_code = SimQL_ReturnCodes::Code::SUCCESS_INFO;
                break;
            default:
                return_code = SimQL_ReturnCodes::Code::ERROR_ALLOC_HANDLE;
                return;
            }

            // set attribute (cursor type)
            SQLPOINTER p_cursor_type;
            switch (options.cursor_type) {
            case CursorType::CX_FORWARD_ONLY:
                p_cursor_type = reinterpret_cast<SQLPOINTER>(SQL_CURSOR_FORWARD_ONLY);
                break;
            case CursorType::CX_STATIC:
                p_cursor_type = reinterpret_cast<SQLPOINTER>(SQL_CURSOR_STATIC);
                break;
            case CursorType::CX_DYNAMIC:
                p_cursor_type = reinterpret_cast<SQLPOINTER>(SQL_CURSOR_DYNAMIC);
                break;
            case CursorType::CX_KEYSET_DRIVEN:
                p_cursor_type = reinterpret_cast<SQLPOINTER>(SQL_CURSOR_KEYSET_DRIVEN);
                break;
            }
            switch (SQLSetStmtAttrW(h_stmt, SQL_ATTR_CURSOR_TYPE, p_cursor_type, 0)) {
            case SQL_SUCCESS:
                break;
            case SQL_SUCCESS_WITH_INFO:
                return_code = SimQL_ReturnCodes::Code::SUCCESS_INFO;
                break;
            default:
                return_code = SimQL_ReturnCodes::Code::ERROR_SET_CURSOR_TYPE;
                return;
            }

            // set attribute (query timeout)
            SQLPOINTER p_query_timeout = reinterpret_cast<SQLPOINTER>(static_cast<SQLULEN>(options.query_timeout));
            switch (SQLSetStmtAttrW(h_stmt, SQL_ATTR_QUERY_TIMEOUT, p_query_timeout, 0)) {
            case SQL_SUCCESS:
                break;
            case SQL_SUCCESS_WITH_INFO:
                return_code = SimQL_ReturnCodes::Code::SUCCESS_INFO;
                break;
            default:
                return_code = SimQL_ReturnCodes::Code::ERROR_SET_QUERY_TIMEOUT;
                return;
            }
            
            // set attribute (max rows)
            SQLPOINTER p_max_rows = reinterpret_cast<SQLPOINTER>(static_cast<SQLULEN>(options.max_rows));
            switch (SQLSetStmtAttrW(h_stmt, SQL_ATTR_MAX_ROWS, p_max_rows, 0)) {
            case SQL_SUCCESS:
                break;
            case SQL_SUCCESS_WITH_INFO:
                return_code = SimQL_ReturnCodes::Code::SUCCESS_INFO;
                break;
            default:
                return_code = SimQL_ReturnCodes::Code::ERROR_SET_MAX_ROWS;
                return;
            }
        }

        ~handle() {
            reset();
            if (h_stmt)
                SQLFreeHandle(SQL_HANDLE_STMT, h_stmt);
        }

        void reset() {
            SQLCloseCursor(h_stmt);
            SQLFreeStmt(h_stmt, SQL_RESET_PARAMS);
            SQLFreeStmt(h_stmt, SQL_UNBIND);
        }

        SQLSMALLINT param_type(const SimpleSqlTypes::BindingType& binding_type) {
            switch (binding_type) {
            case SimpleSqlTypes::BindingType::INPUT_OUTPUT:
                return SQL_PARAM_INPUT_OUTPUT;
            case SimpleSqlTypes::BindingType::INPUT:
                return SQL_PARAM_INPUT;
            case SimpleSqlTypes::BindingType::OUTPUT:
                return SQL_PARAM_OUTPUT;
            }
        }

        SimQL_ReturnCodes::Code bind_string(const std::string& name, const std::u8string& value, const SimpleSqlTypes::BindingType& binding_type, const bool& set_null) {
            auto it = bound_parameters.find(name);
            if (it != bound_parameters.end())
                return SimQL_ReturnCodes::Code::ERROR_SET_PARAM_DUPLICATE;

            SQLSMALLINT sql_type;
            SQLULEN definition;
            SQLSMALLINT scale = 0;
            SQLSMALLINT nullable = 0;
            SQLLEN buffer_length;

            if (!SQL_SUCCEEDED(SQLDescribeParam(h_stmt, pbind_index, &sql_type, &definition, &scale, &nullable))) {
                sql_type = SQL_WVARCHAR;
                definition = static_cast<SQLULEN>(value.size() + 1);
            }

            SQLPOINTER p_val;
            SQLSMALLINT c_type;
            if (sql_type == SQL_VARCHAR || sql_type == SQL_CHAR) {
                c_type = SQL_C_CHAR;
                buffer_length = definition * sizeof(SQLCHAR);
                bound_parameters.emplace(name, BoundParameter { BoundValue(SimpleSqlStrings::to_odbc_n(value).data()), 0 });
                p_val = reinterpret_cast<SQLPOINTER>(std::get<SQLCHAR*>(bound_parameters.at(name).value));
            } else if (sql_type == SQL_WVARCHAR || sql_type == SQL_WCHAR) {
                c_type = SQL_C_WCHAR;
                buffer_length = definition * sizeof(SQLWCHAR);
                bound_parameters.emplace(name, BoundParameter { BoundValue(SimpleSqlStrings::to_odbc_w(value).data()), 0 });
                p_val = reinterpret_cast<SQLPOINTER>(std::get<SQLWCHAR*>(bound_parameters.at(name).value));
            } else {
                return SimQL_ReturnCodes::Code::ERROR_SET_PARAM_INVALID_DTYPE;
            }

            switch (binding_type) {
            case SimpleSqlTypes::BindingType::INPUT_OUTPUT:
                bound_parameters.at(name).indicator = set_null ? SQL_NULL_DATA : SQL_NTS;
                break;
            case SimpleSqlTypes::BindingType::INPUT:
                bound_parameters.at(name).indicator = set_null ? SQL_NULL_DATA : SQL_NTS;
                break;
            case SimpleSqlTypes::BindingType::OUTPUT:
                bound_parameters.at(name).indicator = 0;
                break;
            }

            switch (SQLBindParameter(h_stmt, pbind_index, param_type(binding_type), c_type, sql_type, definition, scale, p_val, buffer_length, &bound_parameters.at(name).indicator)) {
            case SQL_SUCCESS:
                pbind_index++;
                return SimQL_ReturnCodes::Code::SUCCESS;
            case SQL_SUCCESS_WITH_INFO:
                pbind_index++;
                return SimQL_ReturnCodes::Code::SUCCESS_INFO;
            default:
                return SimQL_ReturnCodes::Code::ERROR_SET_PARAM_BINDING;
            }
        }

        SimQL_ReturnCodes::Code bind_floating_point(const std::string& name, const double& value, const SimpleSqlTypes::BindingType& binding_type, const bool& set_null) {
            auto it = bound_parameters.find(name);
            if (it != bound_parameters.end())
                return SimQL_ReturnCodes::Code::ERROR_SET_PARAM_DUPLICATE;

            SQLSMALLINT sql_type;
            SQLULEN definition;
            SQLSMALLINT scale = 0;
            SQLSMALLINT nullable = 0;
            SQLLEN buffer_length;

            if (!SQL_SUCCEEDED(SQLDescribeParam(h_stmt, pbind_index, &sql_type, &definition, &scale, &nullable))) {
                sql_type = SQL_DOUBLE;
            }

            SQLPOINTER p_val;
            SQLSMALLINT c_type;
            if (sql_type == SQL_DOUBLE || sql_type == SQL_FLOAT) {
                sql_type = SQL_DOUBLE;
                c_type = SQL_C_DOUBLE;
                buffer_length = sizeof(SQLDOUBLE);
                bound_parameters.emplace(name, BoundParameter { BoundValue(value), 0 });
                auto& param = bound_parameters.at(name);
                auto& val = std::get<SQLDOUBLE>(param.value);
                p_val = reinterpret_cast<SQLPOINTER>(&val);
            } else if (sql_type == SQL_REAL) {
                c_type = SQL_C_FLOAT;
                buffer_length = sizeof(SQLREAL);
                bound_parameters.emplace(name, BoundParameter { BoundValue(value), 0 });
                auto& param = bound_parameters.at(name);
                auto& val = std::get<SQLREAL>(param.value);
                p_val = reinterpret_cast<SQLPOINTER>(&val);
            } else {
                return SimQL_ReturnCodes::Code::ERROR_SET_PARAM_INVALID_DTYPE;
            }
            definition = buffer_length * 2;

            switch (binding_type) {
            case SimpleSqlTypes::BindingType::INPUT_OUTPUT:
                bound_parameters.at(name).indicator = set_null ? SQL_NULL_DATA : 0;
                break;
            case SimpleSqlTypes::BindingType::INPUT:
                bound_parameters.at(name).indicator = set_null ? SQL_NULL_DATA : 0;
                break;
            case SimpleSqlTypes::BindingType::OUTPUT:
                bound_parameters.at(name).indicator = 0;
                break;
            }

            switch (SQLBindParameter(h_stmt, pbind_index, param_type(binding_type), c_type, sql_type, definition, scale, p_val, buffer_length, &bound_parameters.at(name).indicator)) {
            case SQL_SUCCESS:
                pbind_index++;
                return SimQL_ReturnCodes::Code::SUCCESS;
            case SQL_SUCCESS_WITH_INFO:
                pbind_index++;
                return SimQL_ReturnCodes::Code::SUCCESS_INFO;
            default:
                return SimQL_ReturnCodes::Code::ERROR_SET_PARAM_BINDING;
            }
        }

        SimQL_ReturnCodes::Code bind_boolean(const std::string& name, const bool& value, const SimpleSqlTypes::BindingType& binding_type, const bool& set_null) {
            auto it = bound_parameters.find(name);
            if (it != bound_parameters.end())
                return SimQL_ReturnCodes::Code::ERROR_SET_PARAM_DUPLICATE;

            SQLSMALLINT sql_type;
            SQLULEN definition;
            SQLSMALLINT scale;
            SQLSMALLINT nullable;
            SQLLEN buffer_length = 0;

            if (!SQL_SUCCEEDED(SQLDescribeParam(h_stmt, pbind_index, &sql_type, &definition, &scale, &nullable))) {
                sql_type = SQL_BIT;
                definition = 1;
            }

            SQLPOINTER p_val;
            SQLSMALLINT c_type;
            if (sql_type == SQL_BIT) {
                c_type = SQL_C_BIT;
                bound_parameters.emplace(name, BoundParameter { BoundValue(static_cast<SQLCHAR>(value)), 0 });
                auto& param = bound_parameters.at(name);
                auto& val = std::get<SQLCHAR>(param.value);
                p_val = reinterpret_cast<SQLPOINTER>(&val);
            } else {
                return SimQL_ReturnCodes::Code::ERROR_SET_PARAM_INVALID_DTYPE;
            }

            switch (binding_type) {
            case SimpleSqlTypes::BindingType::INPUT_OUTPUT:
                bound_parameters.at(name).indicator = set_null ? SQL_NULL_DATA : 0;
                break;
            case SimpleSqlTypes::BindingType::INPUT:
                bound_parameters.at(name).indicator = set_null ? SQL_NULL_DATA : 0;
                break;
            case SimpleSqlTypes::BindingType::OUTPUT:
                bound_parameters.at(name).indicator = 0;
                break;
            }

            switch (SQLBindParameter(h_stmt, pbind_index, param_type(binding_type), c_type, sql_type, definition, scale, p_val, buffer_length, &bound_parameters.at(name).indicator)) {
            case SQL_SUCCESS:
                pbind_index++;
                return SimQL_ReturnCodes::Code::SUCCESS;
            case SQL_SUCCESS_WITH_INFO:
                pbind_index++;
                return SimQL_ReturnCodes::Code::SUCCESS_INFO;
            default:
                return SimQL_ReturnCodes::Code::ERROR_SET_PARAM_BINDING;
            }
        }

        SimQL_ReturnCodes::Code bind_integer(const std::string& name, const int& value, const SimpleSqlTypes::BindingType& binding_type, const bool& set_null) {
            auto it = bound_parameters.find(name);
            if (it != bound_parameters.end())
                return SimQL_ReturnCodes::Code::ERROR_SET_PARAM_DUPLICATE;

            SQLSMALLINT sql_type;
            SQLULEN definition = 0;
            SQLSMALLINT scale = 0;
            SQLSMALLINT nullable = 0;
            SQLLEN buffer_length = 0;

            if (!SQL_SUCCEEDED(SQLDescribeParam(h_stmt, pbind_index, &sql_type, &definition, &scale, &nullable))) {
                sql_type = SQL_INTEGER;
            }

            SQLPOINTER p_val;
            SQLSMALLINT c_type;
            if (sql_type == SQL_TINYINT) {
                c_type = SQL_C_STINYINT;
                bound_parameters.emplace(name, BoundParameter { BoundValue(static_cast<SQLCHAR>(value)), 0 });
                auto& param = bound_parameters.at(name);
                auto& val = std::get<SQLCHAR>(param.value);
                p_val = reinterpret_cast<SQLPOINTER>(&val);
            } else if (sql_type == SQL_SMALLINT) {
                c_type = SQL_C_SSHORT;
                bound_parameters.emplace(name, BoundParameter { BoundValue(static_cast<SQLSMALLINT>(value)), 0 });
                auto& param = bound_parameters.at(name);
                auto& val = std::get<SQLSMALLINT>(param.value);
                p_val = reinterpret_cast<SQLPOINTER>(&val);
            } else if (sql_type == SQL_INTEGER) {
                c_type = SQL_C_SLONG;
                bound_parameters.emplace(name, BoundParameter { BoundValue(static_cast<SQLINTEGER>(value)), 0 });
                auto& param = bound_parameters.at(name);
                auto& val = std::get<SQLINTEGER>(param.value);
                p_val = reinterpret_cast<SQLPOINTER>(&val);
            } else if (sql_type == SQL_BIGINT) {
                c_type = SQL_C_SBIGINT;
                bound_parameters.emplace(name, BoundParameter { BoundValue(static_cast<SQLBIGINT>(value)), 0 });
                auto& param = bound_parameters.at(name);
                auto& val = std::get<SQLBIGINT>(param.value);
                p_val = reinterpret_cast<SQLPOINTER>(&val);
            } else {
                return SimQL_ReturnCodes::Code::ERROR_SET_PARAM_INVALID_DTYPE;
            }

            switch (binding_type) {
            case SimpleSqlTypes::BindingType::INPUT_OUTPUT:
                bound_parameters.at(name).indicator = set_null ? SQL_NULL_DATA : 0;
                break;
            case SimpleSqlTypes::BindingType::INPUT:
                bound_parameters.at(name).indicator = set_null ? SQL_NULL_DATA : 0;
                break;
            case SimpleSqlTypes::BindingType::OUTPUT:
                bound_parameters.at(name).indicator = 0;
                break;
            }

            switch (SQLBindParameter(h_stmt, pbind_index, param_type(binding_type), c_type, sql_type, definition, scale, p_val, buffer_length, &bound_parameters.at(name).indicator)) {
            case SQL_SUCCESS:
                pbind_index++;
                return SimQL_ReturnCodes::Code::SUCCESS;
            case SQL_SUCCESS_WITH_INFO:
                pbind_index++;
                return SimQL_ReturnCodes::Code::SUCCESS_INFO;
            default:
                return SimQL_ReturnCodes::Code::ERROR_SET_PARAM_BINDING;
            }
        }

        SimQL_ReturnCodes::Code bind_guid(const std::string& name, const SimpleSqlTypes::ODBC_GUID& value, const SimpleSqlTypes::BindingType& binding_type, const bool& set_null) {
            auto it = bound_parameters.find(name);
            if (it != bound_parameters.end())
                return SimQL_ReturnCodes::Code::ERROR_SET_PARAM_DUPLICATE;

            SQLSMALLINT sql_type;
            SQLULEN definition;
            SQLSMALLINT scale = 0;
            SQLSMALLINT nullable = 0;
            SQLLEN buffer_length;

            if (!SQL_SUCCEEDED(SQLDescribeParam(h_stmt, pbind_index, &sql_type, &definition, &scale, &nullable))) {
                sql_type = SQL_GUID;
            }

            SQLGUID guid;
            guid.Data1 = value.Data1;
            guid.Data2 = value.Data2;
            guid.Data3 = value.Data3;
            std::copy(std::begin(value.Data4), std::end(value.Data4), std::begin(guid.Data4));

            SQLPOINTER p_val;
            SQLSMALLINT c_type;
            if (sql_type == SQL_GUID) {
                c_type = SQL_C_GUID;
                definition = sizeof(SQLGUID);
                buffer_length = sizeof(SQLGUID);
                bound_parameters.emplace(name, BoundParameter { BoundValue(guid), 0 });
                auto& param = bound_parameters.at(name);
                auto& val = std::get<SQLGUID>(param.value);
                p_val = reinterpret_cast<SQLPOINTER>(&val);
            } else {
                return SimQL_ReturnCodes::Code::ERROR_SET_PARAM_INVALID_DTYPE;
            }

            switch (binding_type) {
            case SimpleSqlTypes::BindingType::INPUT_OUTPUT:
                bound_parameters.at(name).indicator = set_null ? SQL_NULL_DATA : 0;
                break;
            case SimpleSqlTypes::BindingType::INPUT:
                bound_parameters.at(name).indicator = set_null ? SQL_NULL_DATA : 0;
                break;
            case SimpleSqlTypes::BindingType::OUTPUT:
                bound_parameters.at(name).indicator = 0;
                break;
            }

            switch (SQLBindParameter(h_stmt, pbind_index, param_type(binding_type), c_type, sql_type, definition, scale, p_val, buffer_length, &bound_parameters.at(name).indicator)) {
            case SQL_SUCCESS:
                pbind_index++;
                return SimQL_ReturnCodes::Code::SUCCESS;
            case SQL_SUCCESS_WITH_INFO:
                pbind_index++;
                return SimQL_ReturnCodes::Code::SUCCESS_INFO;
            default:
                return SimQL_ReturnCodes::Code::ERROR_SET_PARAM_BINDING;
            }
        }

        SimQL_ReturnCodes::Code bind_datetime(const std::string& name, const SimpleSqlTypes::_Datetime& value, const SimpleSqlTypes::BindingType& binding_type, const bool& set_null) {
            auto it = bound_parameters.find(name);
            if (it != bound_parameters.end())
                return SimQL_ReturnCodes::Code::ERROR_SET_PARAM_DUPLICATE;

            SQLSMALLINT sql_type = SQL_TYPE_TIMESTAMP;
            SQLSMALLINT c_type = SQL_C_TYPE_TIMESTAMP;
            SQLULEN definition = 29;
            SQLSMALLINT scale = 9;
            SQLLEN buffer_length = 0;

            SQL_TIMESTAMP_STRUCT x;
            x.year = value.year;
            x.month = value.month;
            x.day = value.day;
            x.hour = value.hour;
            x.minute = value.minute;
            x.second = value.second;
            x.fraction = value.fraction;

            bound_parameters.emplace(name, BoundParameter { BoundValue(x), 0 });
            auto& param = bound_parameters.at(name);
            auto& val = std::get<SQL_TIMESTAMP_STRUCT>(param.value);
            SQLPOINTER p_val = reinterpret_cast<SQLPOINTER>(&val);

            switch (binding_type) {
            case SimpleSqlTypes::BindingType::INPUT_OUTPUT:
                bound_parameters.at(name).indicator = set_null ? SQL_NULL_DATA : 0;
                break;
            case SimpleSqlTypes::BindingType::INPUT:
                bound_parameters.at(name).indicator = set_null ? SQL_NULL_DATA : 0;
                break;
            case SimpleSqlTypes::BindingType::OUTPUT:
                bound_parameters.at(name).indicator = 0;
                break;
            }

            switch (SQLBindParameter(h_stmt, pbind_index, param_type(binding_type), c_type, sql_type, definition, scale, p_val, buffer_length, &bound_parameters.at(name).indicator)) {
            case SQL_SUCCESS:
                pbind_index++;
                return SimQL_ReturnCodes::Code::SUCCESS;
            case SQL_SUCCESS_WITH_INFO:
                pbind_index++;
                return SimQL_ReturnCodes::Code::SUCCESS_INFO;
            default:
                return SimQL_ReturnCodes::Code::ERROR_SET_PARAM_BINDING;
            }
        }

        SimQL_ReturnCodes::Code bind_date(const std::string& name, const SimpleSqlTypes::_Date& value, const SimpleSqlTypes::BindingType& binding_type, const bool& set_null) {
            auto it = bound_parameters.find(name);
            if (it != bound_parameters.end())
                return SimQL_ReturnCodes::Code::ERROR_SET_PARAM_DUPLICATE;

            SQLSMALLINT sql_type = SQL_TYPE_DATE;
            SQLSMALLINT c_type = SQL_C_TYPE_DATE;
            SQLULEN definition = 10;
            SQLSMALLINT scale = 0;
            SQLLEN buffer_length = 0;

            SQL_DATE_STRUCT x;
            x.year = value.year;
            x.month = value.month;
            x.day = value.day;

            bound_parameters.emplace(name, BoundParameter { BoundValue(x), 0 });
            auto& param = bound_parameters.at(name);
            auto& val = std::get<SQL_DATE_STRUCT>(param.value);
            SQLPOINTER p_val = reinterpret_cast<SQLPOINTER>(&val);

            switch (binding_type) {
            case SimpleSqlTypes::BindingType::INPUT_OUTPUT:
                bound_parameters.at(name).indicator = set_null ? SQL_NULL_DATA : 0;
                break;
            case SimpleSqlTypes::BindingType::INPUT:
                bound_parameters.at(name).indicator = set_null ? SQL_NULL_DATA : 0;
                break;
            case SimpleSqlTypes::BindingType::OUTPUT:
                bound_parameters.at(name).indicator = 0;
                break;
            }

            switch (SQLBindParameter(h_stmt, pbind_index, param_type(binding_type), c_type, sql_type, definition, scale, p_val, buffer_length, &bound_parameters.at(name).indicator)) {
            case SQL_SUCCESS:
                pbind_index++;
                return SimQL_ReturnCodes::Code::SUCCESS;
            case SQL_SUCCESS_WITH_INFO:
                pbind_index++;
                return SimQL_ReturnCodes::Code::SUCCESS_INFO;
            default:
                return SimQL_ReturnCodes::Code::ERROR_SET_PARAM_BINDING;
            }
        }

        SimQL_ReturnCodes::Code bind_time(const std::string& name, const SimpleSqlTypes::_Time& value, const SimpleSqlTypes::BindingType& binding_type, const bool& set_null) {
            auto it = bound_parameters.find(name);
            if (it != bound_parameters.end())
                return SimQL_ReturnCodes::Code::ERROR_SET_PARAM_DUPLICATE;

            SQLSMALLINT sql_type = SQL_TYPE_TIME;
            SQLSMALLINT c_type = SQL_C_TYPE_TIME;
            SQLULEN definition = 10;
            SQLSMALLINT scale = 0;
            SQLLEN buffer_length = 0;

            SQL_TIME_STRUCT x;
            x.hour = value.hour;
            x.minute = value.minute;
            x.second = value.second;

            bound_parameters.emplace(name, BoundParameter { BoundValue(x), 0 });
            auto& param = bound_parameters.at(name);
            auto& val = std::get<SQL_TIME_STRUCT>(param.value);
            SQLPOINTER p_val = reinterpret_cast<SQLPOINTER>(&val);

            switch (binding_type) {
            case SimpleSqlTypes::BindingType::INPUT_OUTPUT:
                bound_parameters.at(name).indicator = set_null ? SQL_NULL_DATA : 0;
                break;
            case SimpleSqlTypes::BindingType::INPUT:
                bound_parameters.at(name).indicator = set_null ? SQL_NULL_DATA : 0;
                break;
            case SimpleSqlTypes::BindingType::OUTPUT:
                bound_parameters.at(name).indicator = 0;
                break;
            }

            switch (SQLBindParameter(h_stmt, pbind_index, param_type(binding_type), c_type, sql_type, definition, scale, p_val, buffer_length, &bound_parameters.at(name).indicator)) {
            case SQL_SUCCESS:
                pbind_index++;
                return SimQL_ReturnCodes::Code::SUCCESS;
            case SQL_SUCCESS_WITH_INFO:
                pbind_index++;
                return SimQL_ReturnCodes::Code::SUCCESS_INFO;
            default:
                return SimQL_ReturnCodes::Code::ERROR_SET_PARAM_BINDING;
            }
        }

        SimQL_ReturnCodes::Code bind_blob(const std::string& name, const std::vector<std::uint8_t>& value, const SimpleSqlTypes::BindingType& binding_type, const bool& set_null) {
            auto it = bound_parameters.find(name);
            if (it != bound_parameters.end())
                return SimQL_ReturnCodes::Code::ERROR_SET_PARAM_DUPLICATE;

            SQLSMALLINT sql_type;
            SQLULEN definition;
            SQLSMALLINT scale = 0;
            SQLSMALLINT nullable = 0;
            SQLLEN buffer_length = static_cast<SQLLEN>(value.size());

            if (!SQL_SUCCEEDED(SQLDescribeParam(h_stmt, pbind_index, &sql_type, &definition, &scale, &nullable))) {
                definition = static_cast<SQLULEN>(value.size());
                sql_type = SQL_VARBINARY;
            }

            if (sql_type != SQL_VARBINARY && sql_type != SQL_BINARY)
                return SimQL_ReturnCodes::Code::ERROR_SET_PARAM_INVALID_DTYPE;

            std::vector<unsigned char> arr;
            std::copy(value.begin(), value.end(), arr.begin());
            SQLSMALLINT c_type = SQL_C_BINARY;
            bound_parameters.emplace(name, BoundParameter { BoundValue(arr.data()), 0 });
            auto& param = bound_parameters.at(name);
            auto& val = std::get<SQLCHAR*>(param.value);
            SQLPOINTER p_val = reinterpret_cast<SQLPOINTER>(&val);

            switch (binding_type) {
            case SimpleSqlTypes::BindingType::INPUT_OUTPUT:
                bound_parameters.at(name).indicator = set_null ? SQL_NULL_DATA : buffer_length;
                break;
            case SimpleSqlTypes::BindingType::INPUT:
                bound_parameters.at(name).indicator = set_null ? SQL_NULL_DATA : buffer_length;
                break;
            case SimpleSqlTypes::BindingType::OUTPUT:
                bound_parameters.at(name).indicator = 0;
                break;
            }

            switch (SQLBindParameter(h_stmt, pbind_index, param_type(binding_type), c_type, sql_type, definition, scale, p_val, buffer_length, &bound_parameters.at(name).indicator)) {
            case SQL_SUCCESS:
                pbind_index++;
                return SimQL_ReturnCodes::Code::SUCCESS;
            case SQL_SUCCESS_WITH_INFO:
                pbind_index++;
                return SimQL_ReturnCodes::Code::SUCCESS_INFO;
            default:
                return SimQL_ReturnCodes::Code::ERROR_SET_PARAM_BINDING;
            }
        }

    };

    // Statement definition
    Statement::Statement(Connection& dbc, const Statement::Options& options) : sp_handle(std::make_unique<handle>(dbc, options)) {}
    Statement::~Statement() = default;
    Statement::Statement(Statement&&) noexcept = default;
    Statement& Statement::operator=(Statement&&) noexcept = default;

    void Statement::prepare(std::string_view sql) {
        std::string my_sql(sql);
        my_sql.clear();
    }

    SimQL_ReturnCodes::Code Statement::bind_string(std::string name, std::u8string value, SimpleSqlTypes::BindingType binding_type, bool set_null) {
        if (sp_handle)
            return sp_handle.get()->bind_string(name, value, binding_type, set_null);

        return SimQL_ReturnCodes::Code::NULLPTR;
    }

    SimQL_ReturnCodes::Code Statement::bind_floating_point(std::string name, double value, SimpleSqlTypes::BindingType binding_type, bool set_null) {
        if (sp_handle)
            return sp_handle.get()->bind_floating_point(name, value, binding_type, set_null);

        return SimQL_ReturnCodes::Code::NULLPTR;
    }

    SimQL_ReturnCodes::Code Statement::bind_boolean(std::string name, bool value, SimpleSqlTypes::BindingType binding_type, bool set_null) {
        if (sp_handle)
            return sp_handle.get()->bind_boolean(name, value, binding_type, set_null);

        return SimQL_ReturnCodes::Code::NULLPTR;
    }

    SimQL_ReturnCodes::Code Statement::bind_integer(std::string name, int value, SimpleSqlTypes::BindingType binding_type, bool set_null) {
        if (sp_handle)
            return sp_handle.get()->bind_integer(name, value, binding_type, set_null);

        return SimQL_ReturnCodes::Code::NULLPTR;
    }

    SimQL_ReturnCodes::Code Statement::bind_guid(std::string name, SimpleSqlTypes::ODBC_GUID value, SimpleSqlTypes::BindingType binding_type, bool set_null) {
        if (sp_handle)
            return sp_handle.get()->bind_guid(name, value, binding_type, set_null);

        return SimQL_ReturnCodes::Code::NULLPTR;
    }

    SimQL_ReturnCodes::Code Statement::bind_datetime(std::string name, SimpleSqlTypes::_Datetime value, SimpleSqlTypes::BindingType binding_type, bool set_null) {
        if (sp_handle)
            return sp_handle.get()->bind_datetime(name, value, binding_type, set_null);

        return SimQL_ReturnCodes::Code::NULLPTR;
    }

    SimQL_ReturnCodes::Code Statement::bind_date(std::string name, SimpleSqlTypes::_Date value, SimpleSqlTypes::BindingType binding_type, bool set_null) {
        if (sp_handle)
            return sp_handle.get()->bind_date(name, value, binding_type, set_null);

        return SimQL_ReturnCodes::Code::NULLPTR;
    }

    SimQL_ReturnCodes::Code Statement::bind_time(std::string name, SimpleSqlTypes::_Time value, SimpleSqlTypes::BindingType binding_type, bool set_null) {
        if (sp_handle)
            return sp_handle.get()->bind_time(name, value, binding_type, set_null);

        return SimQL_ReturnCodes::Code::NULLPTR;
    }

    SimQL_ReturnCodes::Code Statement::bind_blob(std::string name, std::vector<std::uint8_t> value, SimpleSqlTypes::BindingType binding_type, bool set_null) {
        if (sp_handle)
            return sp_handle.get()->bind_blob(name, value, binding_type, set_null);

        return SimQL_ReturnCodes::Code::NULLPTR;
    }

    void Statement::execute() {

    }

    void Statement::execute_direct(std::string_view sql) {
        std::string my_sql(sql);
        my_sql.clear();
    }

    const SimQL_ReturnCodes::Code& Statement::return_code() {
        if (sp_handle)
            return sp_handle.get()->return_code;

        return SimQL_ReturnCodes::IS_NULLPTR;
    }
}