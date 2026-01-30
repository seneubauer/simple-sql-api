// SimQL stuff
#include <SimResultSet.hpp>
#include <SimQL_Types.hpp>

// STL stuff
#include <cstdint>
#include <vector>
#include <unordered_map>
#include <string>
#include <utility>

const std::uint8_t& SimpleSql::SimResultSet::add_column(const std::string& name, const std::uint8_t& ordinal) {
    
    auto it = m_column_map.find(name);
    if (it != m_column_map.end())
        return _RC_COLUMN_ALREADY_EXISTS;

    m_column_map.emplace(name, ordinal);
    m_columns.emplace_back(SimpleSqlTypes::SQL_Column {
        name,
        ordinal
    });
}

const std::uint8_t& SimpleSql::SimResultSet::add_row(std::vector<SimpleSqlTypes::SQL_Value>&& row) {
    if (row.size() != m_columns.size())
        return _RC_INCORRECT_DATA_SIZE;

    m_row_count++;
    m_data.resize(m_data.size() + row.size());
    m_data.insert(m_data.end(), std::make_move_iterator(row.begin()), std::make_move_iterator(row.end()));
}

const std::uint8_t& SimpleSql::SimResultSet::set_data(std::vector<SimpleSqlTypes::SQL_Value>&& data) {
    if (data.size() % m_columns.size() != 0)
        return _RC_INCORRECT_DATA_SIZE;

    m_data = std::move(data);
    m_row_count = m_data.size() / m_columns.size();
}

const std::vector<SimpleSqlTypes::SQL_Column>& SimpleSql::SimResultSet::columns() {
    return m_columns;
}

SimpleSqlTypes::SQL_Value* SimpleSql::SimResultSet::value(const std::uint64_t& r, const std::string& c) {
    auto it = m_column_map.find(c);
    if (it == m_column_map.end())
        return nullptr;

    return value(r, it->second);
}

SimpleSqlTypes::SQL_Value* SimpleSql::SimResultSet::value(const std::uint64_t& r, const std::uint8_t& c) {
    if (r >= m_row_count || c >= m_column_count)
        return nullptr;

    std::uint64_t index = r * m_column_count + c;
    if (index >= m_data.size())
        return nullptr;

    return &m_data[index];
}

std::vector<SimpleSqlTypes::SQL_Value> SimpleSql::SimResultSet::row(const std::uint64_t& r) {
    std::vector<SimpleSqlTypes::SQL_Value> v;
    if (r >= m_row_count)
        return v;

    auto it = m_data.begin();
    v.insert(v.begin(), it + r * m_column_count, it + r * m_column_count + m_column_count);
    return v;
}

std::vector<SimpleSqlTypes::SQL_Value> SimpleSql::SimResultSet::column(const std::string& c) {
    std::vector<SimpleSqlTypes::SQL_Value> v;
    auto it = m_column_map.find(c);
    if (it == m_column_map.end())
        return v;

    return column(it->second);
}

std::vector<SimpleSqlTypes::SQL_Value> SimpleSql::SimResultSet::column(const std::uint8_t& c) {
    std::vector<SimpleSqlTypes::SQL_Value> v;
    if (c >= m_column_count)
        return v;

    for (std::uint64_t i = 0; i < m_row_count; ++i) {
        v.emplace_back(m_data[i * m_column_count + c]);
    }
    return v;
}

std::string_view SimpleSql::SimResultSet::return_code_def(const std::uint8_t& return_code) {
    auto it = m_return_codes.find(return_code);
    if (it == m_return_codes.end())
        return std::string_view("invalid return code");

    return it->second;
}
