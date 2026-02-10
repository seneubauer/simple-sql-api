// SimQL stuff
#include <SimConnection.hpp>
#include <SimEnvironment.hpp>
#include <SimQL_ReturnCodes.hpp>
#include <SimQL_Strings.hpp>

// STL stuff
#include <cstdint>
#include <memory>
#include <vector>

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

    extern void* get_env_handle(Environment& env) noexcept;

    struct Connection::handle {
        SQLHDBC h_dbc;
        SimQL_ReturnCodes::Code return_code;

        explicit handle(Environment& env, const Connection::Options& options) {

            h_dbc = SQL_NULL_HDBC;
            SQLHENV h_env = static_cast<SQLHENV>(get_env_handle(env));
            return_code = SimQL_ReturnCodes::Code::SUCCESS;

            // allocate the handle
            switch (SQLAllocHandle(SQL_HANDLE_DBC, h_env, &h_dbc)) {
            case SQL_SUCCESS:
                break;
            case SQL_SUCCESS_WITH_INFO:
                return_code = SimQL_ReturnCodes::Code::SUCCESS_INFO;
                break;
            default:
                return_code = SimQL_ReturnCodes::Code::ERROR_ALLOC_HANDLE;
                return;
            }

            // set attribute (access mode)
            SQLPOINTER p_access_mode = options.read_only ? reinterpret_cast<SQLPOINTER>(SQL_MODE_READ_ONLY) : reinterpret_cast<SQLPOINTER>(SQL_MODE_READ_WRITE);
            switch (SQLSetConnectAttrW(h_dbc, SQL_ATTR_ACCESS_MODE, p_access_mode, 0)) {
            case SQL_SUCCESS:
                break;
            case SQL_SUCCESS_WITH_INFO:
                return_code = SimQL_ReturnCodes::Code::SUCCESS_INFO;
                break;
            default:
                return_code = SimQL_ReturnCodes::Code::ERROR_SET_ACCESS_MODE;
                return;
            }

            // set attribute (connection timeout)
            SQLPOINTER p_connection_timeout = reinterpret_cast<SQLPOINTER>(static_cast<SQLUINTEGER>(options.connection_timeout));
            switch (SQLSetConnectAttrW(h_dbc, SQL_ATTR_CONNECTION_TIMEOUT, p_connection_timeout, 0)) {
            case SQL_SUCCESS:
                break;
            case SQL_SUCCESS_WITH_INFO:
                return_code = SimQL_ReturnCodes::Code::SUCCESS_INFO;
                break;
            default:
                return_code = SimQL_ReturnCodes::Code::ERROR_SET_CONNECTION_TIMEOUT;
                return;
            }

            // set attribute (login timeout)
            SQLPOINTER p_login_timeout = reinterpret_cast<SQLPOINTER>(static_cast<SQLUINTEGER>(options.login_timeout));
            switch (SQLSetConnectAttrW(h_dbc, SQL_ATTR_LOGIN_TIMEOUT, p_login_timeout, 0)) {
            case SQL_SUCCESS:
                break;
            case SQL_SUCCESS_WITH_INFO:
                return_code = SimQL_ReturnCodes::Code::SUCCESS_INFO;
                break;
            default:
                return_code = SimQL_ReturnCodes::Code::ERROR_SET_LOGIN_TIMEOUT;
                return;
            }

            // set attribute (packet size)
            SQLPOINTER p_packet_size = reinterpret_cast<SQLPOINTER>(static_cast<SQLUINTEGER>(options.packet_size));
            switch (SQLSetConnectAttrW(h_dbc, SQL_ATTR_PACKET_SIZE, p_packet_size, 0)) {
            case SQL_SUCCESS:
                break;
            case SQL_SUCCESS_WITH_INFO:
                return_code = SimQL_ReturnCodes::Code::SUCCESS_INFO;
                break;
            default:
                return_code = SimQL_ReturnCodes::Code::ERROR_SET_PACKET_SIZE;
                return;
            }

            // set attribute (async)
            SQLPOINTER p_async = options.enable_async ? reinterpret_cast<SQLPOINTER>(SQL_ASYNC_ENABLE_ON) : reinterpret_cast<SQLPOINTER>(SQL_ASYNC_ENABLE_OFF);
            switch (SQLSetConnectAttrW(h_dbc, SQL_ATTR_ASYNC_ENABLE, p_async, 0)) {
            case SQL_SUCCESS:
                break;
            case SQL_SUCCESS_WITH_INFO:
                return_code = SimQL_ReturnCodes::Code::SUCCESS_INFO;
                break;
            default:
                return_code = SimQL_ReturnCodes::Code::ERROR_SET_ASYNC;
                return;
            }

            // set attribute (autocommit)
            SQLPOINTER p_autocommit = options.enable_autocommit ? reinterpret_cast<SQLPOINTER>(SQL_AUTOCOMMIT_ON) : reinterpret_cast<SQLPOINTER>(SQL_AUTOCOMMIT_OFF);
            switch (SQLSetConnectAttrW(h_dbc, SQL_ATTR_AUTOCOMMIT, p_autocommit, 0)) {
            case SQL_SUCCESS:
                break;
            case SQL_SUCCESS_WITH_INFO:
                return_code = SimQL_ReturnCodes::Code::SUCCESS_INFO;
                break;
            default:
                return_code = SimQL_ReturnCodes::Code::ERROR_SET_AUTOCOMMIT;
                return;
            }

            // set attribute (tracing)
            SQLPOINTER p_tracing = options.enable_tracing ? reinterpret_cast<SQLPOINTER>(SQL_OPT_TRACE_ON) : reinterpret_cast<SQLPOINTER>(SQL_OPT_TRACE_OFF);
            switch (SQLSetConnectAttrW(h_dbc, SQL_ATTR_TRACE, p_tracing, 0)) {
            case SQL_SUCCESS:
                break;
            case SQL_SUCCESS_WITH_INFO:
                return_code = SimQL_ReturnCodes::Code::SUCCESS_INFO;
                break;
            default:
                return_code = SimQL_ReturnCodes::Code::ERROR_SET_TRACING;
                return;
            }

            // set attribute (tracefile)
            if (options.enable_tracing) {
                auto str =  SimpleSqlStrings::utf8_to_odbc(options.tracefile);
                switch (SQLSetConnectAttrW(h_dbc, SQL_ATTR_TRACEFILE, str.data(), SQL_NTS)) {
                case SQL_SUCCESS:
                    break;
                case SQL_SUCCESS_WITH_INFO:
                    return_code = SimQL_ReturnCodes::Code::SUCCESS_INFO;
                    break;
                default:
                    return_code = SimQL_ReturnCodes::Code::ERROR_SET_TRACEFILE;
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
            std::wstring connection_string_in = SimpleSqlStrings::utf8_to_odbc(connection_string);
            std::vector<SQLWCHAR> connection_string_out(1024);
            SQLSMALLINT connection_string_out_length;
            switch (SQLDriverConnectW(h_dbc, nullptr, connection_string_in.data(), SQL_NTS, connection_string_out.data(), sizeof(connection_string_out), &connection_string_out_length, SQL_DRIVER_NOPROMPT)) {
            case SQL_SUCCESS:
                break;
            case SQL_SUCCESS_WITH_INFO:
                return_code = SimQL_ReturnCodes::Code::SUCCESS_INFO;
                break;
            default:
                return_code = SimQL_ReturnCodes::Code::ERROR_OPEN_CONNECTION;
                return;
            }
        }

        bool is_connected() {
            SQLUINTEGER output;
            switch (SQLGetConnectAttrW(h_dbc, SQL_ATTR_CONNECTION_DEAD, &output, 0, nullptr)) {
            case SQL_SUCCESS:
                return_code = SimQL_ReturnCodes::Code::SUCCESS;
                break;
            case SQL_SUCCESS_WITH_INFO:
                return_code = SimQL_ReturnCodes::Code::SUCCESS_INFO;
                break;
            default:
                return_code = SimQL_ReturnCodes::Code::ERROR_UNKNOWN_CONNECTION_STATE;
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
    Connection::Connection(Environment& env, const Connection::Options& options) : sp_handle(std::make_unique<handle>(env, options)) {}
    Connection::~Connection() = default;
    Connection::Connection(Connection&&) noexcept = default;
    Connection& Connection::operator=(Connection&&) noexcept = default;

    void Connection::connect(std::string_view connection_string) {
        if (sp_handle)
            sp_handle.get()->connect(connection_string);
    }

    bool Connection::is_connected() {
        if (sp_handle) {
            return sp_handle.get()->is_connected();
        } else {
            return false;
        }
    }

    void Connection::disconnect() {
        if (sp_handle)
            sp_handle.get()->disconnect();
    }

    const SimQL_ReturnCodes::Code& Connection::return_code() {
        if (sp_handle)
            return sp_handle.get()->return_code;

        return SimQL_ReturnCodes::IS_NULLPTR;
    }

    void* get_dbc_handle(Connection& dbc) noexcept {
        return dbc.sp_handle ? reinterpret_cast<void*>(dbc.sp_handle.get()->h_dbc) : nullptr;
    }
}