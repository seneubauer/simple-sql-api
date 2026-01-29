#ifndef SimResultSet_header_h
#define SimResultSet_header_h

// SimQL stuff
#include <SimQL_Types.hpp>

// STL stuff
#include <cstdint>
#include <vector>
#include <unordered_map>
#include <string>

namespace SimpleSql {
    class SimResultSet {
    public:

        /* data structures */
        struct SQLValue {
            SimpleSqlTypes::SQLVariant data;
            SimpleSqlTypes::SQLDataType data_type;
            bool is_null;
        };

        struct SQLColumn {
            std::string name;
            std::uint8_t ordinal;
        };

        /* constructor/destructor */
        SimResultSet() {}
        ~SimResultSet() {}

        /* functions */
        const std::uint8_t& add_column(const std::string& name, const std::uint8_t& ordinal);
        const std::uint8_t& add_row(std::vector<SQLValue>&& row);
        const std::uint8_t& set_data(std::vector<SQLValue>&& data);
        SQLValue* value(const std::uint64_t& row, const std::uint8_t& column);
        std::vector<SQLValue> row(const std::uint64_t& row);
        std::vector<SQLValue> colummn(const std::uint8_t& column);

    private:

        /* members */
        std::uint64_t m_row_count;
        std::uint8_t m_column_count;
        std::vector<SQLValue> m_data;
        std::vector<SQLColumn> m_columns;
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