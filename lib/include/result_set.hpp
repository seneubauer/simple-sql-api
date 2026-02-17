#ifndef result_set_header_h
#define result_set_header_h

// SimQL stuff
#include "simql_types.hpp"

// STL stuff
#include <cstdint>
#include <vector>
#include <unordered_map>
#include <string>
#include <string_view>

namespace simql {
    class result_set {
    public:

        /* constructor/destructor */
        result_set() {}
        ~result_set() {}

        /* functions */
        bool add_column(const simql_types::sql_column& column);
        bool add_row(std::vector<simql_types::sql_value>&& r);
        bool set_data(std::vector<simql_types::sql_value>&& data);
        const std::vector<simql_types::sql_column>& columns();
        simql_types::sql_value* value(const std::size_t& r, const std::string& c);
        simql_types::sql_value* value(const std::size_t& r, const std::size_t& c);
        std::vector<simql_types::sql_value> row(const std::size_t& r);
        std::vector<simql_types::sql_value> column(const std::string& c);
        std::vector<simql_types::sql_value> column(const std::size_t& c);
        const std::size_t& row_count();
        const std::size_t& column_count();

    private:

        /* members */
        std::size_t m_row_count;
        std::vector<simql_types::sql_value> m_data;
        std::vector<simql_types::sql_column> m_columns;
        std::unordered_map<std::string, std::uint8_t> m_column_map;
    };
}

#endif