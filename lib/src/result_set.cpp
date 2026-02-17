// SimQL stuff
#include "result_set.hpp"
#include "simql_types.hpp"

// STL stuff
#include <cstdint>
#include <vector>
#include <unordered_map>
#include <string>
#include <utility>

namespace simql {

    bool result_set::add_column(const simql_types::sql_column& column) {
        
        auto it = m_column_map.find(column.name);
        if (it != m_column_map.end())
            return false;

        m_column_map.emplace(column.name, m_columns.size());
        m_columns.emplace_back(column);
        return true;
    }

    bool result_set::add_row(std::vector<simql_types::sql_value>&& row) {
        if (row.size() != m_columns.size())
            return false;

        m_row_count++;
        m_data.resize(m_data.size() + row.size());
        m_data.insert(m_data.end(), std::make_move_iterator(row.begin()), std::make_move_iterator(row.end()));
        return true;
    }

    bool result_set::set_data(std::vector<simql_types::sql_value>&& data) {
        if (data.size() % m_columns.size() != 0)
            return false;

        m_data = std::move(data);
        m_row_count = m_data.size() / m_columns.size();
        return true;
    }

    const std::vector<simql_types::sql_column>& result_set::columns() {
        return m_columns;
    }

    simql_types::sql_value* result_set::value(const std::size_t& r, const std::string& c) {
        auto it = m_column_map.find(c);
        if (it == m_column_map.end())
            return nullptr;

        return value(r, it->second);
    }

    simql_types::sql_value* result_set::value(const std::size_t& r, const std::size_t& c) {
        if (r >= m_row_count || c >= m_columns.size())
            return nullptr;

        std::uint64_t index = r * m_columns.size() + c;
        if (index >= m_data.size())
            return nullptr;

        return &m_data[index];
    }

    std::vector<simql_types::sql_value> result_set::row(const std::size_t& r) {
        std::vector<simql_types::sql_value> v;
        if (r >= m_row_count)
            return v;

        auto csize = m_columns.size();
        auto it = m_data.begin();
        v.insert(v.begin(), it + r * csize, it + r * csize + csize);
        return v;
    }

    std::vector<simql_types::sql_value> result_set::column(const std::string& c) {
        std::vector<simql_types::sql_value> v;
        auto it = m_column_map.find(c);
        if (it == m_column_map.end())
            return v;

        return column(it->second);
    }

    std::vector<simql_types::sql_value> result_set::column(const std::size_t& c) {
        std::vector<simql_types::sql_value> v;
        if (c >= m_columns.size())
            return v;

        for (std::uint64_t i = 0; i < m_row_count; ++i) {
            v.emplace_back(m_data[i * m_columns.size() + c]);
        }
        return v;
    }

    const std::size_t& result_set::row_count() {
        return m_row_count;
    }

    const std::size_t& result_set::column_count() {
        return m_columns.size();
    }

}