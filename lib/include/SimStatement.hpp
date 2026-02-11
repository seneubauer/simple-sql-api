#ifndef SimStatement_header_h
#define SimStatement_header_h

// SimQL stuff
#include <SimConnection.hpp>
#include <SimQL_ReturnCodes.hpp>
#include <SimQL_Types.hpp>

// STL stuff
#include <cstdint>
#include <memory>
#include <unordered_map>

namespace SimpleSql {
    class Statement {
    public:

        /* enums */
        enum class CursorType : std::uint8_t {
            CX_FORWARD_ONLY,
            CX_STATIC,
            CX_DYNAMIC,
            CX_KEYSET_DRIVEN
        };

        /* structs */
        struct Options {
            CursorType cursor_type = CursorType::CX_FORWARD_ONLY;
            std::uint32_t query_timeout = 0;
            std::uint64_t max_rows = 0;
        };

        struct ValuePair {
            std::string name;
            SimpleSqlTypes::SQL_Value value;
        };

        /* constructor/destructor */
        explicit Statement(Connection& dbc, const Options& options);
        ~Statement();

        /* set move assignment */
        Statement(Statement&&) noexcept;
        Statement& operator=(Statement&&) noexcept;

        /* set copy assignment */
        Statement(const Statement&) = delete;
        Statement& operator=(const Statement&) = delete;

        /* execution */
        SimQL_ReturnCodes::Code prepare(std::u8string_view sql);
        SimQL_ReturnCodes::Code execute();
        SimQL_ReturnCodes::Code execute_direct(std::u8string_view sql);

        /* data retrieval */
        SimQL_ReturnCodes::Code get_result_set(std::vector<SimpleSqlTypes::SQL_Value>& results, std::vector<SimpleSqlTypes::SQL_Column_>& columns, std::uint64_t& row_count, std::uint8_t& skipped_columns);
        SimQL_ReturnCodes::Code get_value_set(std::vector<ValuePair>& value_pairs);
        bool next_result_set();
        bool next_value_set();

        /* parameter binding */
        SimQL_ReturnCodes::Code bind_string(std::string name, std::u8string value, SimpleSqlTypes::BindingType binding_type, bool set_null);
        SimQL_ReturnCodes::Code bind_floating_point(std::string name, double value, SimpleSqlTypes::BindingType binding_type, bool set_null);
        SimQL_ReturnCodes::Code bind_boolean(std::string name, bool value, SimpleSqlTypes::BindingType binding_type, bool set_null);
        SimQL_ReturnCodes::Code bind_integer(std::string name, int value, SimpleSqlTypes::BindingType binding_type, bool set_null);
        SimQL_ReturnCodes::Code bind_guid(std::string name, SimpleSqlTypes::ODBC_GUID value, SimpleSqlTypes::BindingType binding_type, bool set_null);
        SimQL_ReturnCodes::Code bind_datetime(std::string name, SimpleSqlTypes::_Datetime value, SimpleSqlTypes::BindingType binding_type, bool set_null);
        SimQL_ReturnCodes::Code bind_date(std::string name, SimpleSqlTypes::_Date value, SimpleSqlTypes::BindingType binding_type, bool set_null);
        SimQL_ReturnCodes::Code bind_time(std::string name, SimpleSqlTypes::_Time value, SimpleSqlTypes::BindingType binding_type, bool set_null);
        SimQL_ReturnCodes::Code bind_blob(std::string name, std::vector<std::uint8_t> value, SimpleSqlTypes::BindingType binding_type, bool set_null);

        /* process transparency */
        const SimQL_ReturnCodes::Code& return_code();

    private:
        struct handle;
        std::unique_ptr<handle> sp_handle;
    };
}

#endif