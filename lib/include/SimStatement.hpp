#ifndef SimStatement_header_h
#define SimStatement_header_h

// SimQL stuff
#include <SimConnection.hpp>
#include <SimQL_ReturnCodes.hpp>
#include <SimQL_Types.hpp>

// STL stuff
#include <cstdint>
#include <memory>
#include <deque>
#include <map>

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

        /* handle options */
        struct Options {
            CursorType cursor_type = CursorType::CX_FORWARD_ONLY;
            std::uint32_t query_timeout = 0;
            std::uint64_t max_rows = 0;
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

        /* functions */
        void prepare(std::string_view sql);

        SimQL_ReturnCodes::Code bind_string(std::string name, std::u8string value, SimpleSqlTypes::BindingType binding_type);
        SimQL_ReturnCodes::Code bind_floating_point(std::string name, double value, SimpleSqlTypes::BindingType binding_type);
        SimQL_ReturnCodes::Code bind_boolean(std::string name, bool value, SimpleSqlTypes::BindingType binding_type);
        SimQL_ReturnCodes::Code bind_integer(std::string name, int value, SimpleSqlTypes::BindingType binding_type);
        SimQL_ReturnCodes::Code bind_guid(std::string name, SimpleSqlTypes::ODBC_GUID value, SimpleSqlTypes::BindingType binding_type);
        SimQL_ReturnCodes::Code bind_datetime(std::string name, SimpleSqlTypes::_Datetime value, SimpleSqlTypes::BindingType binding_type);
        SimQL_ReturnCodes::Code bind_date(std::string name, SimpleSqlTypes::_Date value, SimpleSqlTypes::BindingType binding_type);
        SimQL_ReturnCodes::Code bind_time(std::string name, SimpleSqlTypes::_Time value, SimpleSqlTypes::BindingType binding_type);
        SimQL_ReturnCodes::Code bind_blob(std::string name, std::vector<std::uint8_t> value, SimpleSqlTypes::BindingType binding_type);

        void execute();
        void execute_direct(std::string_view sql);
        const SimQL_ReturnCodes::Code& return_code();

    private:

        struct BoundParameter {
            std::string name;
            SimpleSqlTypes::SQL_Variant value;
            SimpleSqlTypes::SQL_DataType data_type;
            SimpleSqlTypes::BindingType binding_type;
            std::int64_t indicator;
        };

        struct handle;
        std::unique_ptr<handle> sp_handle;
        std::map<std::string, BoundParameter> m_parameters;
    };
}

#endif