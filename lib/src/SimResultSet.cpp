// SimQL stuff
#include <SimResultSet.hpp>
#include <SimQL_Types.hpp>

// STL stuff
#include <cstdint>
#include <vector>
#include <unordered_map>
#include <string>
#include <utility>

const std::uint8_t& simql::result_set::add_column(const simql_types::SQL_Column& column) {
    
    auto it = m_column_map.find(column.name);
    if (it != m_column_map.end())
        return _RC_COLUMN_ALREADY_EXISTS;

    m_column_map.emplace(column.name, column.ordinal);
    m_columns.emplace_back(column);
    return _RC_SUCCESS;
}

const std::uint8_t& simql::result_set::add_row(std::vector<simql_types::SQL_Value>&& row) {
    if (row.size() != m_columns.size())
        return _RC_INCORRECT_DATA_SIZE;

    m_row_count++;
    m_data.resize(m_data.size() + row.size());
    m_data.insert(m_data.end(), std::make_move_iterator(row.begin()), std::make_move_iterator(row.end()));
    return _RC_SUCCESS;
}

const std::uint8_t& simql::result_set::set_data(std::vector<simql_types::SQL_Value>&& data) {
    if (data.size() % m_columns.size() != 0)
        return _RC_INCORRECT_DATA_SIZE;

    m_data = std::move(data);
    m_row_count = m_data.size() / m_columns.size();
    return _RC_SUCCESS;
}

const std::vector<simql_types::SQL_Column>& simql::result_set::columns() {
    return m_columns;
}

simql_types::SQL_Value* simql::result_set::value(const std::uint64_t& r, const std::string& c) {
    auto it = m_column_map.find(c);
    if (it == m_column_map.end())
        return nullptr;

    return value(r, it->second);
}

simql_types::SQL_Value* simql::result_set::value(const std::uint64_t& r, const std::uint8_t& c) {
    if (r >= m_row_count || c >= m_column_count)
        return nullptr;

    std::uint64_t index = r * m_column_count + c;
    if (index >= m_data.size())
        return nullptr;

    return &m_data[index];
}

std::vector<simql_types::SQL_Value> simql::result_set::row(const std::uint64_t& r) {
    std::vector<simql_types::SQL_Value> v;
    if (r >= m_row_count)
        return v;

    auto it = m_data.begin();
    v.insert(v.begin(), it + r * m_column_count, it + r * m_column_count + m_column_count);
    return v;
}

std::vector<simql_types::SQL_Value> simql::result_set::column(const std::string& c) {
    std::vector<simql_types::SQL_Value> v;
    auto it = m_column_map.find(c);
    if (it == m_column_map.end())
        return v;

    return column(it->second);
}

std::vector<simql_types::SQL_Value> simql::result_set::column(const std::uint8_t& c) {
    std::vector<simql_types::SQL_Value> v;
    if (c >= m_column_count)
        return v;

    for (std::uint64_t i = 0; i < m_row_count; ++i) {
        v.emplace_back(m_data[i * m_column_count + c]);
    }
    return v;
}

const std::uint64_t& simql::result_set::row_count() {
    return m_row_count;
}

const std::uint8_t& simql::result_set::column_count() {
    return m_column_count;
}

std::string_view simql::result_set::return_code_def(const std::uint8_t& return_code) {
    auto it = m_return_codes.find(return_code);
    if (it == m_return_codes.end())
        return std::string_view("invalid return code");

    return it->second;
}
