// SimQL stuff
#include <SimValueSet.hpp>

// STL stuff
#include <cstdint>
#include <unordered_map>
#include <string>
#include <string_view>
#include <utility>

namespace simql {
    const std::uint8_t& value_set::add_value(const std::string& name, simql_types::sql_value&& value) {
        auto it = m_values.find(name);
        if (it != m_values.end())
            return _RC_DUPLICATE;

        m_values.emplace(name, std::move(value));
        return _RC_SUCCESS;
    }

    simql_types::sql_value* value_set::value(const std::string_view& name) {
        auto it = m_values.find(name);
        if (it == m_values.end())
            return nullptr;

        return &it->second;
    }

    std::string_view value_set::return_code_def(const std::uint8_t& return_code) {
        auto it = m_return_codes.find(return_code);
        if (it == m_return_codes.end())
            return std::string_view("invalid return code");

        return it->second;
    }
}