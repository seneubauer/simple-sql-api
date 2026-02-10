// SimQL stuff
#include <SimStatement.hpp>
#include <SimConnection.hpp>
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

    extern void* get_dbc_handle(Connection& dbc) noexcept;

    struct Statement::handle {
        SQLHSTMT h_stmt;
        SimQL_ReturnCodes::Code return_code;

        explicit handle(Connection& dbc, const Statement::Options& options) {

            h_stmt = SQL_NULL_HSTMT;
            SQLHDBC h_dbc = static_cast<SQLHDBC>(get_dbc_handle(dbc));
            return_code = SimQL_ReturnCodes::Code::SUCCESS;

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
            if (h_stmt)
                SQLFreeHandle(SQL_HANDLE_STMT, h_stmt);
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