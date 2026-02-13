// SimQL stuff
#include <diagnostic_set.hpp>
#include <simql_strings.hpp>
#include <simql_types.hpp>

// STL stuff
#include <cstdint>
#include <ranges>
#include <vector>
#include <array>
#include <string>

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

namespace simql {

    diagnostic_set::diagnostic_filter_view diagnostic_set::view_diagnostics(std::optional<std::string> sql_state, std::optional<std::int32_t> native_error) {
        return diagnostic_set::diagnostic_filter_view {
            std::ranges::ref_view{m_diagnostics},
            diagnostic_set::FilterPredicate{
                std::move(sql_state),
                std::move(native_error)
            }
        };
    }

    void diagnostic_set::flush() {
        m_diagnostics.clear();
    }

    void diagnostic_set::update(void* handle, const handle_type& type) {
        switch (type) {
        case handle_type::dbc:
            update_diagnostics(static_cast<std::int16_t>(SQL_HANDLE_DBC), handle);
            break;
        case handle_type::env:
            update_diagnostics(static_cast<std::int16_t>(SQL_HANDLE_ENV), handle);
            break;
        case handle_type::stmt:
            update_diagnostics(static_cast<std::int16_t>(SQL_HANDLE_STMT), handle);
            break;
        }
        
    }

    void diagnostic_set::update_diagnostics(const std::int16_t& type, void* handle) {

        SQLSMALLINT handle_type = static_cast<SQLSMALLINT>(type);
        SQLSMALLINT current_record_number = static_cast<SQLSMALLINT>(m_diagnostic_index);
        std::array<SQLWCHAR, 6> sql_state_buffer;
        SQLINTEGER native_error;
        std::array<SQLWCHAR, SQL_MAX_MESSAGE_LENGTH> message_buffer;
        SQLSMALLINT message_length;

        bool exit_condition = false;
        while (true) {
            switch (SQLGetDiagRecW(handle_type, handle, current_record_number, sql_state_buffer.data(), &native_error, message_buffer.data(), message_buffer.size(), &message_length)) {
            case SQL_SUCCESS: break;
            case SQL_SUCCESS_WITH_INFO: break;
            case SQL_NO_DATA: exit_condition = true; break;
            default: return;
            }
            if (exit_condition)
                break;

            m_diagnostics.push_back(diagnostic{
                static_cast<std::int16_t>(current_record_number),
                simql_strings::from_odbc(std::basic_string_view<SQLWCHAR>(sql_state_buffer.data(), sql_state_buffer.size())),
                static_cast<std::int32_t>(native_error),
                simql_strings::from_odbc(std::basic_string_view<SQLWCHAR>(message_buffer.data(), message_length))
            });
            current_record_number++;
        }
        m_diagnostic_index = static_cast<std::int16_t>(current_record_number);
    }

}