#ifndef result_set_header_h
#define result_set_header_h

// SimQL stuff
#include <SimQL_Types.hpp>

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
        const std::uint8_t& add_column(const simql_types::sql_column& column);
        const std::uint8_t& add_row(std::vector<simql_types::sql_value>&& r);
        const std::uint8_t& set_data(std::vector<simql_types::sql_value>&& data);
        const std::vector<simql_types::sql_column>& columns();
        simql_types::sql_value* value(const std::uint64_t& r, const std::string& c);
        simql_types::sql_value* value(const std::uint64_t& r, const std::uint8_t& c);
        std::vector<simql_types::sql_value> row(const std::uint64_t& r);
        std::vector<simql_types::sql_value> column(const std::string& c);
        std::vector<simql_types::sql_value> column(const std::uint8_t& c);
        const std::uint64_t& row_count();
        const std::uint8_t& column_count();
        std::string_view return_code_def(const std::uint8_t& return_code);

    private:

        /* members */
        std::uint64_t m_row_count;
        std::uint8_t m_column_count;
        std::vector<simql_types::sql_value> m_data;
        std::vector<simql_types::sql_column> m_columns;
        std::unordered_map<std::string, std::uint8_t> m_column_map;

        /* return codes */
        static constexpr std::uint8_t _RC_SUCCESS               = 0;
        static constexpr std::uint8_t _RC_INCORRECT_DATA_SIZE   = 1;
        static constexpr std::uint8_t _RC_COLUMNS_NOT_SET       = 2;
        static constexpr std::uint8_t _RC_COLUMN_ALREADY_EXISTS = 3;
        const std::unordered_map<std::uint8_t, std::string_view> m_return_codes {
            {_RC_SUCCESS,                   std::string_view("process was successful")},
            {_RC_INCORRECT_DATA_SIZE,       std::string_view("data does not match the provided column size")},
            {_RC_COLUMNS_NOT_SET,           std::string_view("columns must be defined")},
            {_RC_COLUMN_ALREADY_EXISTS,     std::string_view("column already exists")}
        };
    };
}

#endif