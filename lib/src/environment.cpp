// SimQL stuff
#include <environment.hpp>
#include <simql_returncodes.hpp>

// STL stuff
#include <cstdint>
#include <memory>

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

    struct environment::handle {
        SQLHENV h_env;
        simql_returncodes::code return_code;

        explicit handle(const environment::alloc_options& options) {

            h_env = SQL_NULL_HENV;
            return_code = simql_returncodes::code::success;

            // allocate the handle
            switch (SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &h_env)) {
            case SQL_SUCCESS:
                break;
            case SQL_SUCCESS_WITH_INFO:
                return_code = simql_returncodes::code::success_info;
                break;
            default:
                return_code = simql_returncodes::code::error_alloc_handle;
                return;
            }

            // set to ODBC 3.x
            switch (SQLSetEnvAttr(h_env, SQL_ATTR_ODBC_VERSION, reinterpret_cast<SQLPOINTER>(SQL_OV_ODBC3), 0)) {
            case SQL_SUCCESS:
                break;
            case SQL_SUCCESS_WITH_INFO:
                return_code = simql_returncodes::code::success_info;
                break;
            default:
                return_code = simql_returncodes::code::error_set_odbc_version3;
                return;
            }

            // set attribute (connection pooling type)
            switch (options.pool_type) {
            case environment::pooling_type::off:
                switch (SQLSetEnvAttr(h_env, SQL_ATTR_CONNECTION_POOLING, reinterpret_cast<SQLPOINTER>(SQL_CP_OFF), 0)) {
                case SQL_SUCCESS:
                    break;
                case SQL_SUCCESS_WITH_INFO:
                    return_code = simql_returncodes::code::success_info;
                    break;
                default:
                    return_code = simql_returncodes::code::error_set_pooling_type;
                    return;
                }
                break;
            case environment::pooling_type::one_per_driver:
                switch (SQLSetEnvAttr(h_env, SQL_ATTR_CONNECTION_POOLING, reinterpret_cast<SQLPOINTER>(SQL_CP_ONE_PER_DRIVER), 0)) {
                case SQL_SUCCESS:
                    break;
                case SQL_SUCCESS_WITH_INFO:
                    return_code = simql_returncodes::code::success_info;
                    break;
                default:
                    return_code = simql_returncodes::code::error_set_pooling_type;
                    return;
                }
                break;
            case environment::pooling_type::one_per_env:
                switch (SQLSetEnvAttr(h_env, SQL_ATTR_CONNECTION_POOLING, reinterpret_cast<SQLPOINTER>(SQL_CP_ONE_PER_HENV), 0)) {
                case SQL_SUCCESS:
                    break;
                case SQL_SUCCESS_WITH_INFO:
                    return_code = simql_returncodes::code::success_info;
                    break;
                default:
                    return_code = simql_returncodes::code::error_set_pooling_type;
                    return;
                }
                break;
            }

            // set attribute (connection pool match type)
            switch (options.match_type) {
            case environment::pooling_match_type::strict_match:
                switch (SQLSetEnvAttr(h_env, SQL_ATTR_CP_MATCH, reinterpret_cast<SQLPOINTER>(SQL_CP_STRICT_MATCH), 0)) {
                case SQL_SUCCESS:
                    break;
                case SQL_SUCCESS_WITH_INFO:
                    return_code = simql_returncodes::code::success_info;
                    break;
                default:
                    return_code = simql_returncodes::code::error_set_pool_match_type;
                    break;
                }
                break;
            case environment::pooling_match_type::relaxed_match:
                switch (SQLSetEnvAttr(h_env, SQL_ATTR_CP_MATCH, reinterpret_cast<SQLPOINTER>(SQL_CP_RELAXED_MATCH), 0)) {
                case SQL_SUCCESS:
                    break;
                case SQL_SUCCESS_WITH_INFO:
                    return_code = simql_returncodes::code::success_info;
                    break;
                default:
                    return_code = simql_returncodes::code::error_set_pool_match_type;
                    break;
                }
                break;
            }
        }

        ~handle() {
            if (h_env) SQLFreeHandle(SQL_HANDLE_ENV, h_env);
        }
    };

    // Environment definition
    environment::environment(const environment::alloc_options& options) : sp_handle(std::make_unique<handle>(options)) {}
    environment::~environment() = default;
    environment::environment(environment&&) noexcept = default;
    environment& environment::operator=(environment&&) noexcept = default;

    const simql_returncodes::code& environment::return_code() {
        if (sp_handle)
            return sp_handle.get()->return_code;

        return simql_returncodes::is_nullptr;
    }

    void* get_env_handle(environment& env) noexcept {
        return env.sp_handle ? reinterpret_cast<void*>(env.sp_handle.get()->h_env) : nullptr;
    }
}