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

        /* constructor/destructor */
        SimResultSet() {}
        ~SimResultSet() {}

        /* functions */
        const std::uint8_t& add_column(const std::string& name, const std::uint8_t& ordinal);
        const std::uint8_t& add_row(std::vector<SimpleSqlTypes::SQL_Value>&& r);
        const std::uint8_t& set_data(std::vector<SimpleSqlTypes::SQL_Value>&& data);
        const std::vector<SimpleSqlTypes::SQL_Column>& columns();
        SimpleSqlTypes::SQL_Value* value(const std::uint64_t& r, const std::string& c);
        SimpleSqlTypes::SQL_Value* value(const std::uint64_t& r, const std::uint8_t& c);
        std::vector<SimpleSqlTypes::SQL_Value> row(const std::uint64_t& r);
        std::vector<SimpleSqlTypes::SQL_Value> column(const std::string& c);
        std::vector<SimpleSqlTypes::SQL_Value> column(const std::uint8_t& c);
        std::string_view return_code_def(const std::uint8_t& return_code);

    private:

        /* members */
        std::uint64_t m_row_count;
        std::uint8_t m_column_count;
        std::vector<SimpleSqlTypes::SQL_Value> m_data;
        std::vector<SimpleSqlTypes::SQL_Column> m_columns;
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