#ifndef diagnostic_set_header_h
#define diagnostic_set_header_h

// SimQL stuff
#include <SimQL_Types.hpp>

// STL stuff
#include <string>
#include <cstdint>
#include <vector>
#include <unordered_map>
#include <optional>
#include <ranges>

namespace simql {
    class diagnostic_set {
    public:

        /* enums */
        enum class handle_type {
            dbc,
            env,
            stmt
        };

        /* structs */
        struct diagnostic {
            std::int16_t record_number;
            std::u8string sql_state;
            std::int32_t native_error;
            std::u8string message;
        };

        struct FilterPredicate {
            std::optional<std::string> sql_state;
            std::optional<std::int32_t> native_error;
            bool operator() (const diagnostic& diag) const noexcept {
                if (sql_state.has_value())
                    if (diag.sql_state != *sql_state)
                        return false;
                if (native_error.has_value())
                    if (diag.native_error != *native_error)
                        return false;
                return true;
            }
        };
        using diagnostic_filter_view = std::ranges::filter_view<std::ranges::ref_view<std::vector<diagnostic>>, FilterPredicate>;

        // constructor/destructor
        diagnostic_set() : m_diagnostic_index(1) {}
        ~diagnostic_set() {}

        // functions
        diagnostic_filter_view view_diagnostics(std::optional<std::string> sql_state = std::nullopt, std::optional<std::int32_t> native_error = std::nullopt);
        void flush();
        void update(void* handle, const handle_type& type);

    private:

        /* functions */
        void update_diagnostics(const std::int16_t& type, void* handle);

        /* members */
        std::int16_t m_diagnostic_index;
        std::vector<diagnostic> m_diagnostics;

        /* dictionaries */
        const std::unordered_map<std::u8string, std::string_view> m_states {
            {std::u8string(u8"UNSET"), std::string_view("UNSET")},
            {std::u8string(u8"01000"), std::string_view("General warning")},
            {std::u8string(u8"01001"), std::string_view("Cursor operation conflict")},
            {std::u8string(u8"01002"), std::string_view("Disconnect error")},
            {std::u8string(u8"01003"), std::string_view("NULL value eliminated in set function")},
            {std::u8string(u8"01004"), std::string_view("String data, right-truncated")},
            {std::u8string(u8"01006"), std::string_view("Privilege not revoked")},
            {std::u8string(u8"01007"), std::string_view("Privilege not granted")},
            {std::u8string(u8"01S00"), std::string_view("Invalid connection string attribute")},
            {std::u8string(u8"01S01"), std::string_view("Error in row")},
            {std::u8string(u8"01S02"), std::string_view("Option value changed")},
            {std::u8string(u8"01S06"), std::string_view("Attempt to fetch before the result set returned the first rowset")},
            {std::u8string(u8"01S07"), std::string_view("Fractional truncation")},
            {std::u8string(u8"01S08"), std::string_view("Error saving File DSN")},
            {std::u8string(u8"01S09"), std::string_view("Invalid keyword")},
            {std::u8string(u8"07001"), std::string_view("Wrong number of parameters")},
            {std::u8string(u8"07002"), std::string_view("COUNT field incorrect")},
            {std::u8string(u8"07005"), std::string_view("Prepared statement not a cursor-specification")},
            {std::u8string(u8"07006"), std::string_view("Restricted data type attribute violation")},
            {std::u8string(u8"07009"), std::string_view("Invalid descriptor index")},
            {std::u8string(u8"07S01"), std::string_view("Invalid use of default parameter")},
            {std::u8string(u8"08001"), std::string_view("Client unable to establish connection")},
            {std::u8string(u8"08002"), std::string_view("Connection name in use")},
            {std::u8string(u8"08003"), std::string_view("Connection not open")},
            {std::u8string(u8"08004"), std::string_view("Server rejected the connection")},
            {std::u8string(u8"08007"), std::string_view("Connection failure during transaction")},
            {std::u8string(u8"08S01"), std::string_view("Communication link failure")},
            {std::u8string(u8"21S01"), std::string_view("Insert value list does not match column list")},
            {std::u8string(u8"21S02"), std::string_view("Degree of derived table does not match column list")},
            {std::u8string(u8"22001"), std::string_view("String data, right-truncated")},
            {std::u8string(u8"22002"), std::string_view("Indicator variable required but not supplied")},
            {std::u8string(u8"22003"), std::string_view("Numeric value out of range")},
            {std::u8string(u8"22007"), std::string_view("Invalid datetime format")},
            {std::u8string(u8"22008"), std::string_view("Datetime field overflow")},
            {std::u8string(u8"22012"), std::string_view("Division by zero")},
            {std::u8string(u8"22015"), std::string_view("Interval field overflow")},
            {std::u8string(u8"22018"), std::string_view("Invalid character value for cast specification")},
            {std::u8string(u8"22019"), std::string_view("Invalid escape character")},
            {std::u8string(u8"22025"), std::string_view("Invalid escape sequence")},
            {std::u8string(u8"22026"), std::string_view("String data, length mismatch")},
            {std::u8string(u8"23000"), std::string_view("Integrity constraint violation")},
            {std::u8string(u8"24000"), std::string_view("Invalid cursor state")},
            {std::u8string(u8"25000"), std::string_view("Invalid transaction state")},
            {std::u8string(u8"25S01"), std::string_view("Transaction state")},
            {std::u8string(u8"25S02"), std::string_view("Transaction is still active")},
            {std::u8string(u8"25S03"), std::string_view("Transaction is rolled back")},
            {std::u8string(u8"28000"), std::string_view("Invalid authorization specification")},
            {std::u8string(u8"34000"), std::string_view("Invalid cursor name")},
            {std::u8string(u8"3C000"), std::string_view("Duplicate cursor name")},
            {std::u8string(u8"3D000"), std::string_view("Invalid catalog name")},
            {std::u8string(u8"3F000"), std::string_view("Invalid schema name")},
            {std::u8string(u8"40001"), std::string_view("Serialization failure")},
            {std::u8string(u8"40002"), std::string_view("Integrity constraint violation")},
            {std::u8string(u8"40003"), std::string_view("Statement completion unknown")},
            {std::u8string(u8"42000"), std::string_view("Syntax error or access violation")},
            {std::u8string(u8"42S01"), std::string_view("Base table or view already exists")},
            {std::u8string(u8"42S02"), std::string_view("Base table or view not found")},
            {std::u8string(u8"42S11"), std::string_view("Index already exists")},
            {std::u8string(u8"42S12"), std::string_view("Index not found")},
            {std::u8string(u8"42S21"), std::string_view("Column already exists")},
            {std::u8string(u8"42S22"), std::string_view("Column not found")},
            {std::u8string(u8"44000"), std::string_view("WITH CHECK OPTION violation")},
            {std::u8string(u8"HY000"), std::string_view("General error")},
            {std::u8string(u8"HY001"), std::string_view("Memory allocation error")},
            {std::u8string(u8"HY003"), std::string_view("Invalid application buffer type")},
            {std::u8string(u8"HY004"), std::string_view("Invalid SQL data type")},
            {std::u8string(u8"HY007"), std::string_view("Associated statement is not prepared")},
            {std::u8string(u8"HY008"), std::string_view("Operation canceled")},
            {std::u8string(u8"HY009"), std::string_view("Invalid use of null pointer")},
            {std::u8string(u8"HY010"), std::string_view("Function sequence error")},
            {std::u8string(u8"HY011"), std::string_view("Attribute cannot be set now")},
            {std::u8string(u8"HY012"), std::string_view("Invalid transaction operation code")},
            {std::u8string(u8"HY013"), std::string_view("Memory management error")},
            {std::u8string(u8"HY014"), std::string_view("Limit on the number of handles exceeded")},
            {std::u8string(u8"HY015"), std::string_view("No cursor name available")},
            {std::u8string(u8"HY016"), std::string_view("Cannot modify an implementation row descriptor")},
            {std::u8string(u8"HY017"), std::string_view("Invalid use of an automatically allocated descriptor handle")},
            {std::u8string(u8"HY018"), std::string_view("Server declined cancel request")},
            {std::u8string(u8"HY019"), std::string_view("Non-character and non-binary data sent in pieces")},
            {std::u8string(u8"HY020"), std::string_view("Attempt to concatenate a null value")},
            {std::u8string(u8"HY021"), std::string_view("Inconsistent descriptor information")},
            {std::u8string(u8"HY024"), std::string_view("Invalid attribute value")},
            {std::u8string(u8"HY090"), std::string_view("Invalid string or buffer length")},
            {std::u8string(u8"HY091"), std::string_view("Invalid descriptor field identifier")},
            {std::u8string(u8"HY092"), std::string_view("Invalid attribute/option identifier")},
            {std::u8string(u8"HY095"), std::string_view("Function type out of range")},
            {std::u8string(u8"HY096"), std::string_view("Invalid information type")},
            {std::u8string(u8"HY097"), std::string_view("Column type out of range")},
            {std::u8string(u8"HY098"), std::string_view("Scope type out of range")},
            {std::u8string(u8"HY099"), std::string_view("Nullable type out of range")},
            {std::u8string(u8"HY100"), std::string_view("Uniqueness option type out of range")},
            {std::u8string(u8"HY101"), std::string_view("Accuracy option type out of range")},
            {std::u8string(u8"HY103"), std::string_view("Invalid retrieval code")},
            {std::u8string(u8"HY104"), std::string_view("Invalid precision or scale value")},
            {std::u8string(u8"HY105"), std::string_view("Invalid parameter type")},
            {std::u8string(u8"HY106"), std::string_view("Fetch type out of range")},
            {std::u8string(u8"HY107"), std::string_view("Row value out of range")},
            {std::u8string(u8"HY109"), std::string_view("Invalid cursor position")},
            {std::u8string(u8"HY110"), std::string_view("Invalid driver completion")},
            {std::u8string(u8"HY111"), std::string_view("Invalid bookmark value")},
            {std::u8string(u8"HYC00"), std::string_view("Optional feature not implemented")},
            {std::u8string(u8"HYT00"), std::string_view("Timeout expired")},
            {std::u8string(u8"HYT01"), std::string_view("Connection timeout expired")},
            {std::u8string(u8"IM001"), std::string_view("Driver does not support this function")},
            {std::u8string(u8"IM002"), std::string_view("Data source name not found and no default driver specified")},
            {std::u8string(u8"IM003"), std::string_view("Specified driver could not be loaded")},
            {std::u8string(u8"IM004"), std::string_view("Driver's SQLAllocHandle on SQL_HANDLE_ENV failed")},
            {std::u8string(u8"IM005"), std::string_view("Driver's SQLAllocHandle on SQL_HANDLE_DBC failed")},
            {std::u8string(u8"IM006"), std::string_view("Driver's SQLSetConnectAttr failed")},
            {std::u8string(u8"IM007"), std::string_view("No data source or driver specified; dialog prohibited")},
            {std::u8string(u8"IM008"), std::string_view("Dialog failed")},
            {std::u8string(u8"IM009"), std::string_view("Unable to load translation DLL")},
            {std::u8string(u8"IM010"), std::string_view("Data source name too long")},
            {std::u8string(u8"IM011"), std::string_view("Driver name too long")},
            {std::u8string(u8"IM012"), std::string_view("DRIVER keyword syntax error")},
            {std::u8string(u8"IM013"), std::string_view("Trace file error")},
            {std::u8string(u8"IM014"), std::string_view("Invalid name of File DSN")},
            {std::u8string(u8"IM015"), std::string_view("Corrupt file data source")}
        };
    };
}

#endif