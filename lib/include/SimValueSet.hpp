#ifndef SimValueSet_header_h
#define SimValueSet_header_h

// SimQL stuff
#include <SimQL_Types.hpp>

// STL stuff
#include <cstdint>
#include <unordered_map>
#include <string>
#include <string_view>

namespace SimpleSql {
    class SimValueSet {
    public:
        
        /* constructor/destructor */
        SimValueSet() {}
        ~SimValueSet() {}

        /* functions */
        const std::uint8_t& add_value(const std::string& name, SimpleSqlTypes::SQL_Value&& value);
        SimpleSqlTypes::SQL_Value* value(const std::string_view& name);
        std::string_view return_code_def(const std::uint8_t& return_code);

    private:

        /* members */
        std::unordered_map<std::string_view, SimpleSqlTypes::SQL_Value> m_values;

        /* return codes */
        static constexpr std::uint8_t _RC_SUCCESS       = 0;
        static constexpr std::uint8_t _RC_DUPLICATE     = 1;
        const std::unordered_map<std::uint8_t, std::string_view> m_return_codes {
            {_RC_SUCCESS,           std::string_view("process was successful")},
            {_RC_DUPLICATE,         std::string_view("this value name is already in use")}
        };
    };
}

#endif