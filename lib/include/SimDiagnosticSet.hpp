#ifndef SimDiagnosticSet_header_h
#define SimDiagnosticSet_header_h

// SimQL stuff
#include <SimQL_Types.hpp>

// STL stuff
#include <string>
#include <cstdint>
#include <vector>
#include <unordered_map>
#include <optional>
#include <ranges>

namespace SimpleSql {
    class SimDiagnosticSet {
    public:

        /* structs */
        struct Diagnostic {
            std::int16_t record_number;
            std::string sql_state;
            std::int32_t native_error;
            std::string message;
        };

        /* filters */
        struct FilterPredicate {
            std::optional<std::string> sql_state;
            std::optional<std::int32_t> native_error;
            bool operator() (const Diagnostic& diag) const noexcept {
                if (sql_state.has_value())
                    if (diag.sql_state != *sql_state)
                        return false;
                if (native_error.has_value())
                    if (diag.native_error != *native_error)
                        return false;
                return true;
            }
        };
        using diagnostic_filter_view = std::ranges::filter_view<std::ranges::ref_view<std::vector<Diagnostic>>, FilterPredicate>;

        // constructor/destructor
        SimDiagnosticSet() : m_diagnostic_index(1) {}
        ~SimDiagnosticSet() {}

        // functions
        diagnostic_filter_view view_diagnostics(std::optional<std::string> sql_state = std::nullopt, std::optional<std::int32_t> native_error = std::nullopt);
        void flush();
        void update_diagnostics(SimpleSqlTypes::DBC_HANDLE* handle);
        void update_diagnostics(SimpleSqlTypes::ENV_HANDLE* handle);
        void update_diagnostics(SimpleSqlTypes::STMT_HANDLE* handle);

    private:

        /* functions */
        void update_diagnostics(const std::int16_t& type, void* handle);

        /* members */
        std::int16_t m_diagnostic_index;
        std::vector<Diagnostic> m_diagnostics;

        /* dictionaries */
        const std::unordered_map<std::string, std::string_view> m_states {
            {std::string("UNSET"), std::string_view("UNSET")},
            {std::string("01000"), std::string_view("General warning")},
            {std::string("01001"), std::string_view("Cursor operation conflict")},
            {std::string("01002"), std::string_view("Disconnect error")},
            {std::string("01003"), std::string_view("NULL value eliminated in set function")},
            {std::string("01004"), std::string_view("String data, right-truncated")},
            {std::string("01006"), std::string_view("Privilege not revoked")},
            {std::string("01007"), std::string_view("Privilege not granted")},
            {std::string("01S00"), std::string_view("Invalid connection string attribute")},
            {std::string("01S01"), std::string_view("Error in row")},
            {std::string("01S02"), std::string_view("Option value changed")},
            {std::string("01S06"), std::string_view("Attempt to fetch before the result set returned the first rowset")},
            {std::string("01S07"), std::string_view("Fractional truncation")},
            {std::string("01S08"), std::string_view("Error saving File DSN")},
            {std::string("01S09"), std::string_view("Invalid keyword")},
            {std::string("07001"), std::string_view("Wrong number of parameters")},
            {std::string("07002"), std::string_view("COUNT field incorrect")},
            {std::string("07005"), std::string_view("Prepared statement not a cursor-specification")},
            {std::string("07006"), std::string_view("Restricted data type attribute violation")},
            {std::string("07009"), std::string_view("Invalid descriptor index")},
            {std::string("07S01"), std::string_view("Invalid use of default parameter")},
            {std::string("08001"), std::string_view("Client unable to establish connection")},
            {std::string("08002"), std::string_view("Connection name in use")},
            {std::string("08003"), std::string_view("Connection not open")},
            {std::string("08004"), std::string_view("Server rejected the connection")},
            {std::string("08007"), std::string_view("Connection failure during transaction")},
            {std::string("08S01"), std::string_view("Communication link failure")},
            {std::string("21S01"), std::string_view("Insert value list does not match column list")},
            {std::string("21S02"), std::string_view("Degree of derived table does not match column list")},
            {std::string("22001"), std::string_view("String data, right-truncated")},
            {std::string("22002"), std::string_view("Indicator variable required but not supplied")},
            {std::string("22003"), std::string_view("Numeric value out of range")},
            {std::string("22007"), std::string_view("Invalid datetime format")},
            {std::string("22008"), std::string_view("Datetime field overflow")},
            {std::string("22012"), std::string_view("Division by zero")},
            {std::string("22015"), std::string_view("Interval field overflow")},
            {std::string("22018"), std::string_view("Invalid character value for cast specification")},
            {std::string("22019"), std::string_view("Invalid escape character")},
            {std::string("22025"), std::string_view("Invalid escape sequence")},
            {std::string("22026"), std::string_view("String data, length mismatch")},
            {std::string("23000"), std::string_view("Integrity constraint violation")},
            {std::string("24000"), std::string_view("Invalid cursor state")},
            {std::string("25000"), std::string_view("Invalid transaction state")},
            {std::string("25S01"), std::string_view("Transaction state")},
            {std::string("25S02"), std::string_view("Transaction is still active")},
            {std::string("25S03"), std::string_view("Transaction is rolled back")},
            {std::string("28000"), std::string_view("Invalid authorization specification")},
            {std::string("34000"), std::string_view("Invalid cursor name")},
            {std::string("3C000"), std::string_view("Duplicate cursor name")},
            {std::string("3D000"), std::string_view("Invalid catalog name")},
            {std::string("3F000"), std::string_view("Invalid schema name")},
            {std::string("40001"), std::string_view("Serialization failure")},
            {std::string("40002"), std::string_view("Integrity constraint violation")},
            {std::string("40003"), std::string_view("Statement completion unknown")},
            {std::string("42000"), std::string_view("Syntax error or access violation")},
            {std::string("42S01"), std::string_view("Base table or view already exists")},
            {std::string("42S02"), std::string_view("Base table or view not found")},
            {std::string("42S11"), std::string_view("Index already exists")},
            {std::string("42S12"), std::string_view("Index not found")},
            {std::string("42S21"), std::string_view("Column already exists")},
            {std::string("42S22"), std::string_view("Column not found")},
            {std::string("44000"), std::string_view("WITH CHECK OPTION violation")},
            {std::string("HY000"), std::string_view("General error")},
            {std::string("HY001"), std::string_view("Memory allocation error")},
            {std::string("HY003"), std::string_view("Invalid application buffer type")},
            {std::string("HY004"), std::string_view("Invalid SQL data type")},
            {std::string("HY007"), std::string_view("Associated statement is not prepared")},
            {std::string("HY008"), std::string_view("Operation canceled")},
            {std::string("HY009"), std::string_view("Invalid use of null pointer")},
            {std::string("HY010"), std::string_view("Function sequence error")},
            {std::string("HY011"), std::string_view("Attribute cannot be set now")},
            {std::string("HY012"), std::string_view("Invalid transaction operation code")},
            {std::string("HY013"), std::string_view("Memory management error")},
            {std::string("HY014"), std::string_view("Limit on the number of handles exceeded")},
            {std::string("HY015"), std::string_view("No cursor name available")},
            {std::string("HY016"), std::string_view("Cannot modify an implementation row descriptor")},
            {std::string("HY017"), std::string_view("Invalid use of an automatically allocated descriptor handle")},
            {std::string("HY018"), std::string_view("Server declined cancel request")},
            {std::string("HY019"), std::string_view("Non-character and non-binary data sent in pieces")},
            {std::string("HY020"), std::string_view("Attempt to concatenate a null value")},
            {std::string("HY021"), std::string_view("Inconsistent descriptor information")},
            {std::string("HY024"), std::string_view("Invalid attribute value")},
            {std::string("HY090"), std::string_view("Invalid string or buffer length")},
            {std::string("HY091"), std::string_view("Invalid descriptor field identifier")},
            {std::string("HY092"), std::string_view("Invalid attribute/option identifier")},
            {std::string("HY095"), std::string_view("Function type out of range")},
            {std::string("HY096"), std::string_view("Invalid information type")},
            {std::string("HY097"), std::string_view("Column type out of range")},
            {std::string("HY098"), std::string_view("Scope type out of range")},
            {std::string("HY099"), std::string_view("Nullable type out of range")},
            {std::string("HY100"), std::string_view("Uniqueness option type out of range")},
            {std::string("HY101"), std::string_view("Accuracy option type out of range")},
            {std::string("HY103"), std::string_view("Invalid retrieval code")},
            {std::string("HY104"), std::string_view("Invalid precision or scale value")},
            {std::string("HY105"), std::string_view("Invalid parameter type")},
            {std::string("HY106"), std::string_view("Fetch type out of range")},
            {std::string("HY107"), std::string_view("Row value out of range")},
            {std::string("HY109"), std::string_view("Invalid cursor position")},
            {std::string("HY110"), std::string_view("Invalid driver completion")},
            {std::string("HY111"), std::string_view("Invalid bookmark value")},
            {std::string("HYC00"), std::string_view("Optional feature not implemented")},
            {std::string("HYT00"), std::string_view("Timeout expired")},
            {std::string("HYT01"), std::string_view("Connection timeout expired")},
            {std::string("IM001"), std::string_view("Driver does not support this function")},
            {std::string("IM002"), std::string_view("Data source name not found and no default driver specified")},
            {std::string("IM003"), std::string_view("Specified driver could not be loaded")},
            {std::string("IM004"), std::string_view("Driver's SQLAllocHandle on SQL_HANDLE_ENV failed")},
            {std::string("IM005"), std::string_view("Driver's SQLAllocHandle on SQL_HANDLE_DBC failed")},
            {std::string("IM006"), std::string_view("Driver's SQLSetConnectAttr failed")},
            {std::string("IM007"), std::string_view("No data source or driver specified; dialog prohibited")},
            {std::string("IM008"), std::string_view("Dialog failed")},
            {std::string("IM009"), std::string_view("Unable to load translation DLL")},
            {std::string("IM010"), std::string_view("Data source name too long")},
            {std::string("IM011"), std::string_view("Driver name too long")},
            {std::string("IM012"), std::string_view("DRIVER keyword syntax error")},
            {std::string("IM013"), std::string_view("Trace file error")},
            {std::string("IM014"), std::string_view("Invalid name of File DSN")},
            {std::string("IM015"), std::string_view("Corrupt file data source")}
        };
    };
}

#endif