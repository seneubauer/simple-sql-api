#ifndef SimQuery_header_h
#define SimQuery_header_h

// SimQL stuff
#include <SimQL_Types.hpp>
#include <SimResultSet.hpp>
#include <SimValueSet.hpp>
#include <SimDiagnosticSet.hpp>

// STL stuff
#include <unordered_map>
#include <vector>
#include <string>
#include <string_view>
#include <cstdint>
#include <memory>

namespace SimpleSql {
    class SimQuery {
    public:

        /* constructor/destructor */
        SimQuery(SimQuery&&) {}
        SimQuery() : h_stmt(nullptr), p_values(std::make_unique<SimValueSet>()), p_results(std::make_unique<SimResultSet>()), p_diagnostics(std::make_unique<SimDiagnosticSet>()), m_binding_index(1), m_is_finished(false) {}
        SimQuery(SimpleSqlTypes::STMT_HANDLE* handle) : h_stmt(handle), p_values(std::make_unique<SimValueSet>()), p_results(std::make_unique<SimResultSet>()), p_diagnostics(std::make_unique<SimDiagnosticSet>()), m_binding_index(1), m_is_finished(false) {}
        ~SimQuery() { }
        SimQuery& operator=(SimQuery&&) noexcept { return *this; }

        /* functions */
        void set_handle(SimpleSqlTypes::STMT_HANDLE* handle);
        std::uint8_t set_sql(const std::string& sql);
        std::uint8_t prepare();
        std::uint8_t bind_parameter(const SimpleSqlTypes::SQL_Binding& binding);
        void add_parameter(const SimpleSqlTypes::SQL_Binding& binding);
        void bind_parameters();
        bool execute(const bool& autofinish = true);
        void finish();
        bool is_valid() const;
        bool is_select() const;
        SimResultSet* results();
        SimDiagnosticSet* diagnostics();
        std::string_view return_code_def(const std::uint8_t& return_code);

    private:

        /* internal functions */
        std::uint8_t define_columns();

        /* members */
        SimpleSqlTypes::STMT_HANDLE* h_stmt;
        std::unique_ptr<SimValueSet> p_values;
        std::unique_ptr<SimResultSet> p_results;
        std::unique_ptr<SimDiagnosticSet> p_diagnostics;
        std::vector<SimpleSqlTypes::SQL_Binding> m_bound_parameters;
        std::vector<SimpleSqlTypes::SQL_Binding> m_bound_columns;
        std::uint32_t m_binding_index;
        bool m_is_valid;
        bool m_is_select;
        bool m_is_finished;
        std::string m_sql;

        // return codes
        static constexpr std::uint8_t _RC_SUCCESS                   = 0;
        static constexpr std::uint8_t _RC_UNDEFINED_COLUMNS         = 1;
        static constexpr std::uint8_t _RC_CALC_COLUMNS              = 2;
        static constexpr std::uint8_t _RC_DUPLICATE_COLUMNS         = 3;
        static constexpr std::uint8_t _RC_EMPTY_SQL                 = 4;
        static constexpr std::uint8_t _RC_PREPARE                   = 5;
        static constexpr std::uint8_t _RC_PARAMETER_CALC            = 6;
        static constexpr std::uint8_t _RC_NO_PARAMETERS             = 7;
        static constexpr std::uint8_t _RC_UNKNOWN_IO_TYPE           = 8;
        static constexpr std::uint8_t _RC_UNKNOWN_BINDING_FAMILY    = 9;
        static constexpr std::uint8_t _RC_UNKNOWN_SQL_C_TYPE        = 10;
        static constexpr std::uint8_t _RC_BINDING                   = 11;
        static constexpr std::uint8_t _RC_BIND_COLUMN_DTYPE         = 12;
        static constexpr std::uint8_t _RC_BIND_COLUMN               = 13;
        const std::unordered_map<std::uint8_t, std::string_view> m_return_codes {
            {_RC_SUCCESS,                   std::string_view("process was successful")},
            {_RC_UNDEFINED_COLUMNS,         std::string_view("could not retrieve the column metadata from the database")},
            {_RC_CALC_COLUMNS,              std::string_view("could not calculate the number of columns found in the result set")},
            {_RC_DUPLICATE_COLUMNS,         std::string_view("potential duplicate columns found in the result set")},
            {_RC_EMPTY_SQL,                 std::string_view("the provided SQL statement is empty")},
            {_RC_PREPARE,                   std::string_view("could not prepare the provided SQL statement")},
            {_RC_PARAMETER_CALC,            std::string_view("could not calculate the number of parameters")},
            {_RC_NO_PARAMETERS,             std::string_view("no parameters found in the prepared SQL statement")},
            {_RC_UNKNOWN_IO_TYPE,           std::string_view("could not determine the ODBC Input/Output type")},
            {_RC_UNKNOWN_BINDING_FAMILY,    std::string_view("could not determine the proper binding family")},
            {_RC_UNKNOWN_SQL_C_TYPE,        std::string_view("could not determine the ODBC C/SQL data type")},
            {_RC_BINDING,                   std::string_view("could not bind the provided parameter")},
            {_RC_BIND_COLUMN,               std::string_view("could not bind the current column")},
            {_RC_BIND_COLUMN_DTYPE,         std::string_view("could not find a SQL/C data type match")}
        };
    };
}

#endif