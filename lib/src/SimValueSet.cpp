// SimQL stuff
#include <SimValueSet.hpp>

// STL stuff
#include <cstdint>
#include <unordered_map>
#include <string>
#include <string_view>
#include <utility>

const std::uint8_t& SimpleSql::SimValueSet::add_value(const std::string& name, SimpleSqlTypes::SQL_Value&& value) {
    auto it = m_values.find(name);
    if (it != m_values.end())
        return _RC_DUPLICATE;

    m_values.emplace(name, std::move(value));
    return _RC_SUCCESS;
}

SimpleSqlTypes::SQL_Value* SimpleSql::SimValueSet::value(const std::string_view& name) {
    auto it = m_values.find(name);
    if (it == m_values.end())
        return nullptr;

    return &it->second;
}

std::string_view SimpleSql::SimValueSet::return_code_def(const std::uint8_t& return_code) {
    auto it = m_return_codes.find(return_code);
    if (it == m_return_codes.end())
        return std::string_view("invalid return code");

    return it->second;
}
