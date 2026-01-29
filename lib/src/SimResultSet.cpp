// SimQL stuff
#include <SimResultSet.hpp>

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
    m_columns.emplace_back(SQLColumn {
        name,
        ordinal
    });
}

const std::uint8_t& SimpleSql::SimResultSet::add_row(std::vector<SQLValue>&& row) {
    if (row.size() != m_columns.size())
        return _RC_INCORRECT_DATA_SIZE;

    m_row_count++;
    m_data.resize(m_data.size() + row.size());
    m_data.insert(m_data.end(), std::make_move_iterator(row.begin()), std::make_move_iterator(row.end()));
}

const std::uint8_t& SimpleSql::SimResultSet::set_data(std::vector<SQLValue>&& data) {
    if (data.size() % m_columns.size() != 0)
        return _RC_INCORRECT_DATA_SIZE;

    m_data = std::move(data);
    m_row_count = m_data.size() / m_columns.size();
}