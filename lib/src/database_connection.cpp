// SimQL stuff
#include <database_connection.hpp>
#include <environment.hpp>
#include <simql_returncodes.hpp>
#include <simql_strings.hpp>

// STL stuff
#include <cstdint>
#include <memory>
#include <vector>
#include <string_view>

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

namespace simql {

    extern void* get_env_handle(environment& env) noexcept;

    struct database_connection::handle {
        SQLHDBC h_dbc;
        simql_returncodes::code return_code;

        explicit handle(environment& env, database_connection::Options& options) {

            h_dbc = SQL_NULL_HDBC;
            SQLHENV h_env = static_cast<SQLHENV>(get_env_handle(env));
            return_code = simql_returncodes::code::success;

            // allocate the handle
            switch (SQLAllocHandle(SQL_HANDLE_DBC, h_env, &h_dbc)) {
            case SQL_SUCCESS:
                break;
            case SQL_SUCCESS_WITH_INFO:
                return_code = simql_returncodes::code::success_info;
                break;
            default:
                return_code = simql_returncodes::code::error_alloc_handle;
                return;
            }

            // set attribute (access mode)
            SQLPOINTER p_access_mode = options.read_only ? reinterpret_cast<SQLPOINTER>(SQL_MODE_READ_ONLY) : reinterpret_cast<SQLPOINTER>(SQL_MODE_READ_WRITE);
            switch (SQLSetConnectAttrW(h_dbc, SQL_ATTR_ACCESS_MODE, p_access_mode, 0)) {
            case SQL_SUCCESS:
                break;
            case SQL_SUCCESS_WITH_INFO:
                return_code = simql_returncodes::code::success_info;
                break;
            default:
                return_code = simql_returncodes::code::error_set_access_mode;
                return;
            }

            // set attribute (connection timeout)
            SQLPOINTER p_connection_timeout = reinterpret_cast<SQLPOINTER>(static_cast<SQLUINTEGER>(options.connection_timeout));
            switch (SQLSetConnectAttrW(h_dbc, SQL_ATTR_CONNECTION_TIMEOUT, p_connection_timeout, 0)) {
            case SQL_SUCCESS:
                break;
            case SQL_SUCCESS_WITH_INFO:
                return_code = simql_returncodes::code::success_info;
                break;
            default:
                return_code = simql_returncodes::code::error_set_connection_timeout;
                return;
            }

            // set attribute (login timeout)
            SQLPOINTER p_login_timeout = reinterpret_cast<SQLPOINTER>(static_cast<SQLUINTEGER>(options.login_timeout));
            switch (SQLSetConnectAttrW(h_dbc, SQL_ATTR_LOGIN_TIMEOUT, p_login_timeout, 0)) {
            case SQL_SUCCESS:
                break;
            case SQL_SUCCESS_WITH_INFO:
                return_code = simql_returncodes::code::success_info;
                break;
            default:
                return_code = simql_returncodes::code::error_set_login_timeout;
                return;
            }

            // set attribute (packet size)
            SQLPOINTER p_packet_size = reinterpret_cast<SQLPOINTER>(static_cast<SQLUINTEGER>(options.packet_size));
            switch (SQLSetConnectAttrW(h_dbc, SQL_ATTR_PACKET_SIZE, p_packet_size, 0)) {
            case SQL_SUCCESS:
                break;
            case SQL_SUCCESS_WITH_INFO:
                return_code = simql_returncodes::code::success_info;
                break;
            default:
                return_code = simql_returncodes::code::error_set_packet_size;
                return;
            }

            // set attribute (async)
            SQLPOINTER p_async = options.enable_async ? reinterpret_cast<SQLPOINTER>(SQL_ASYNC_ENABLE_ON) : reinterpret_cast<SQLPOINTER>(SQL_ASYNC_ENABLE_OFF);
            switch (SQLSetConnectAttrW(h_dbc, SQL_ATTR_ASYNC_ENABLE, p_async, 0)) {
            case SQL_SUCCESS:
                break;
            case SQL_SUCCESS_WITH_INFO:
                return_code = simql_returncodes::code::success_info;
                break;
            default:
                return_code = simql_returncodes::code::error_set_async;
                return;
            }

            // set attribute (autocommit)
            SQLPOINTER p_autocommit = options.enable_autocommit ? reinterpret_cast<SQLPOINTER>(SQL_AUTOCOMMIT_ON) : reinterpret_cast<SQLPOINTER>(SQL_AUTOCOMMIT_OFF);
            switch (SQLSetConnectAttrW(h_dbc, SQL_ATTR_AUTOCOMMIT, p_autocommit, 0)) {
            case SQL_SUCCESS:
                break;
            case SQL_SUCCESS_WITH_INFO:
                return_code = simql_returncodes::code::success_info;
                break;
            default:
                return_code = simql_returncodes::code::error_set_autocommit;
                return;
            }

            // set attribute (tracing)
            SQLPOINTER p_tracing = options.enable_tracing ? reinterpret_cast<SQLPOINTER>(SQL_OPT_TRACE_ON) : reinterpret_cast<SQLPOINTER>(SQL_OPT_TRACE_OFF);
            switch (SQLSetConnectAttrW(h_dbc, SQL_ATTR_TRACE, p_tracing, 0)) {
            case SQL_SUCCESS:
                break;
            case SQL_SUCCESS_WITH_INFO:
                return_code = simql_returncodes::code::success_info;
                break;
            default:
                return_code = simql_returncodes::code::error_set_tracing;
                return;
            }

            // set attribute (tracefile)
            if (options.enable_tracing) {
                auto str = simql_strings::to_odbc_w(std::string_view(reinterpret_cast<char*>(options.tracefile.data()), options.tracefile.size()));
                switch (SQLSetConnectAttrW(h_dbc, SQL_ATTR_TRACEFILE, str.data(), SQL_NTS)) {
                case SQL_SUCCESS:
                    break;
                case SQL_SUCCESS_WITH_INFO:
                    return_code = simql_returncodes::code::success_info;
                    break;
                default:
                    return_code = simql_returncodes::code::error_set_tracefile;
                    return;
                }
            }
        }

        ~handle() {
            if (is_connected())
                SQLDisconnect(h_dbc);

            if (h_dbc)
                SQLFreeHandle(SQL_HANDLE_DBC, h_dbc);
        }

        void connect(std::string_view connection_string) {
            auto connection_string_in = simql_strings::to_odbc_w(std::string_view(reinterpret_cast<const char*>(connection_string.data()), connection_string.size()));
            std::vector<SQLWCHAR> connection_string_out(1024);
            SQLSMALLINT connection_string_out_length;
            switch (SQLDriverConnectW(h_dbc, nullptr, connection_string_in.data(), SQL_NTS, connection_string_out.data(), sizeof(connection_string_out), &connection_string_out_length, SQL_DRIVER_NOPROMPT)) {
            case SQL_SUCCESS:
                break;
            case SQL_SUCCESS_WITH_INFO:
                return_code = simql_returncodes::code::success_info;
                break;
            default:
                return_code = simql_returncodes::code::error_open_connection;
                return;
            }
        }

        bool is_connected() {
            SQLUINTEGER output;
            switch (SQLGetConnectAttrW(h_dbc, SQL_ATTR_CONNECTION_DEAD, &output, 0, nullptr)) {
            case SQL_SUCCESS:
                return_code = simql_returncodes::code::success;
                break;
            case SQL_SUCCESS_WITH_INFO:
                return_code = simql_returncodes::code::success_info;
                break;
            default:
                return_code = simql_returncodes::code::error_unknown_connection_state;
                return false;
            }

            switch (output) {
            case SQL_CD_TRUE:
                return false;
            case SQL_CD_FALSE:
                return true;
            default:
                return false;
            }
        }

        void disconnect() {
            if (is_connected())
                SQLDisconnect(h_dbc);
        }
    };

    // Connection definition
    database_connection::database_connection(environment& env, database_connection::Options& options) : sp_handle(std::make_unique<handle>(env, options)) {}
    database_connection::~database_connection() = default;
    database_connection::database_connection(database_connection&&) noexcept = default;
    database_connection& database_connection::operator=(database_connection&&) noexcept = default;

    void database_connection::connect(std::string_view connection_string) {
        if (sp_handle)
            sp_handle.get()->connect(connection_string);
    }

    bool database_connection::is_connected() {
        if (sp_handle) {
            return sp_handle.get()->is_connected();
        } else {
            return false;
        }
    }

    void database_connection::disconnect() {
        if (sp_handle)
            sp_handle.get()->disconnect();
    }

    const simql_returncodes::code& database_connection::return_code() {
        if (sp_handle)
            return sp_handle.get()->return_code;

        return simql_returncodes::is_nullptr;
    }

    void* get_dbc_handle(database_connection& dbc) noexcept {
        return dbc.sp_handle ? reinterpret_cast<void*>(dbc.sp_handle.get()->h_dbc) : nullptr;
    }
}