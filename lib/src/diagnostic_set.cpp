// SimQL stuff
#include "diagnostic_set.hpp"
#include "simql_strings.hpp"
#include "simql_types.hpp"

// STL stuff
#include <cstdint>
#include <ranges>
#include <vector>
#include <array>
#include <string>
#include <unordered_map>

// OS stuff
#include "os_inclusions.hpp"

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

    void diagnostic_set::update(void* handle, const handle_type& type, std::string library_message) {
        switch (type) {
        case handle_type::dbc:
            update_diagnostics(static_cast<std::int16_t>(SQL_HANDLE_DBC), handle, library_message);
            break;
        case handle_type::env:
            update_diagnostics(static_cast<std::int16_t>(SQL_HANDLE_ENV), handle, library_message);
            break;
        case handle_type::stmt:
            update_diagnostics(static_cast<std::int16_t>(SQL_HANDLE_STMT), handle, library_message);
            break;
        }
    }

    void diagnostic_set::update_diagnostics(const std::int16_t& type, void* handle, std::string library_message) {

        SQLSMALLINT handle_type = static_cast<SQLSMALLINT>(type);
        SQLSMALLINT current_record_number{1};
        std::array<SQLWCHAR, 6> sql_state_buffer;
        SQLINTEGER native_error;
        std::array<SQLWCHAR, SQL_MAX_MESSAGE_LENGTH> message_buffer;
        SQLSMALLINT message_length;

        bool exit_condition = false;
        while (true) {
            switch (SQLGetDiagRecW(handle_type, handle, current_record_number, sql_state_buffer.data(), &native_error, message_buffer.data(), message_buffer.size(), &message_length)) {
            case SQL_SUCCESS:
                break;
            case SQL_SUCCESS_WITH_INFO:
                break;
            case SQL_NO_DATA:
                exit_condition = true;
                break;
            default:
                return;
            }

            m_diagnostics.push_back(diagnostic{
                static_cast<std::int16_t>(current_record_number),
                simql_strings::from_odbc(std::basic_string_view<SQLWCHAR>(sql_state_buffer.data(), sql_state_buffer.size())),
                static_cast<std::int32_t>(native_error),
                simql_strings::from_odbc(std::basic_string_view<SQLWCHAR>(message_buffer.data(), message_length)),
                library_message
            });
            if (exit_condition)
                break;

            current_record_number++;
        }
    }

    std::string_view diagnostic_set::state_description(const std::string& sql_state) {
        auto it = m_states.find(sql_state);
        if (it == m_states.end())
            return m_states.at("UNSET");

        return it->second;
    }

}