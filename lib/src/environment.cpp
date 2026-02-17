// SimQL stuff
#include "environment.hpp"
#include "diagnostic_set.hpp"

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
        SQLHENV h_env = SQL_NULL_HENV;
        diagnostic_set diag;
        bool is_valid{true};

        explicit handle(const environment::alloc_options& options) {

            // allocate the handle
            switch (SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &h_env)) {
            case SQL_SUCCESS:
                break;
            case SQL_SUCCESS_WITH_INFO:
                diag.update(h_env, diagnostic_set::handle_type::env);
                break;
            default:
                is_valid = false;
                return;
            }

            // set to ODBC 3.x
            switch (SQLSetEnvAttr(h_env, SQL_ATTR_ODBC_VERSION, reinterpret_cast<SQLPOINTER>(SQL_OV_ODBC3), 0)) {
            case SQL_SUCCESS:
                break;
            case SQL_SUCCESS_WITH_INFO:
                diag.update(h_env, diagnostic_set::handle_type::env);
                break;
            default:
                diag.update(h_env, diagnostic_set::handle_type::env);
                is_valid = false;
                return;
            }

            // set attribute (connection pooling type)
            SQLPOINTER p_pool_type;
            switch (options.pool_type) {
            case environment::pooling_type::off:
                p_pool_type = reinterpret_cast<SQLPOINTER>(SQL_CP_OFF);
                break;
            case environment::pooling_type::one_per_driver:
                p_pool_type = reinterpret_cast<SQLPOINTER>(SQL_CP_ONE_PER_DRIVER);
                break;
            case environment::pooling_type::one_per_env:
                p_pool_type = reinterpret_cast<SQLPOINTER>(SQL_CP_ONE_PER_HENV);
                break;
            }
            switch (SQLSetEnvAttr(h_env, SQL_ATTR_CONNECTION_POOLING, p_pool_type, 0)) {
            case SQL_SUCCESS:
                break;
            case SQL_SUCCESS_WITH_INFO:
                diag.update(h_env, diagnostic_set::handle_type::env);
                break;
            default:
                diag.update(h_env, diagnostic_set::handle_type::env);
                is_valid = false;
                return;
            }

            // set attribute (connection pool match type)
            SQLPOINTER p_match_type;
            switch (options.match_type) {
            case environment::pooling_match_type::strict_match:
                p_match_type = reinterpret_cast<SQLPOINTER>(SQL_CP_STRICT_MATCH);
                break;
            case environment::pooling_match_type::relaxed_match:
                p_match_type = reinterpret_cast<SQLPOINTER>(SQL_CP_RELAXED_MATCH);
                break;
            }
            switch (SQLSetEnvAttr(h_env, SQL_ATTR_CP_MATCH, p_match_type, 0)) {
            case SQL_SUCCESS:
                break;
            case SQL_SUCCESS_WITH_INFO:
                diag.update(h_env, diagnostic_set::handle_type::env);
                break;
            default:
                diag.update(h_env, diagnostic_set::handle_type::env);
                is_valid = false;
                break;
            }
        }

        ~handle() {
            if (h_env) SQLFreeHandle(SQL_HANDLE_ENV, h_env);
        }
    };

    // environment definition
    environment::environment(const environment::alloc_options& options) : sp_handle(std::make_unique<handle>(options)) {}
    environment::~environment() = default;
    environment::environment(environment&&) noexcept = default;
    environment& environment::operator=(environment&&) noexcept = default;

    bool environment::is_valid() {
        return !sp_handle ? false : sp_handle->is_valid;
    }

    diagnostic_set* environment::diagnostics() {
        return !sp_handle ? nullptr : &sp_handle->diag;
    }

    void* get_env_handle(environment& env) noexcept {
        return env.sp_handle ? reinterpret_cast<void*>(env.sp_handle->h_env) : nullptr;
    }
}