#ifndef SimQuery_header_h
#define SimQuery_header_h

// SimQL stuff
#include <SimQL_Types.hpp>

// STL stuff
#include <unordered_map>
#include <vector>
#include <string>
#include <string_view>
#include <cstdint>
#include <memory>

namespace SimpleSql {
    class SimQuery {
    private:

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
        static constexpr std::uint8_t _RC_NUMERIC_BIND              = 11;
        static constexpr std::uint8_t _RC_BOOL_INT_BIND             = 12;
        static constexpr std::uint8_t _RC_GUID_BIND                 = 13;
        static constexpr std::uint8_t _RC_DATETIME_BIND             = 14;
        static constexpr std::uint8_t _RC_BINDING_NOT_SET           = 15;
        static constexpr std::uint8_t _RC_BINDING                   = 16;
        static constexpr std::uint8_t _RC_ROW_SIZE                  = 17;
        static constexpr std::uint8_t _RC_SQL_C                     = 18;
        static constexpr std::uint8_t _RC_BIND_COLUMN               = 19;
        const std::unordered_map<std::uint8_t, std::string_view> m_rc_def {
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
            {_RC_NUMERIC_BIND,              std::string_view("cannot bind an unknown floating point type")},
            {_RC_BOOL_INT_BIND,             std::string_view("cannot bind an unknown boolean or integer type")},
            {_RC_GUID_BIND,                 std::string_view("cannot bind an unknown GUID type")},
            {_RC_DATETIME_BIND,             std::string_view("cannot bind an unknown date/time type")},
            {_RC_BINDING_NOT_SET,           std::string_view("the binding family is undefined")},
            {_RC_BINDING,                   std::string_view("could not bind the provided parameter")},
            {_RC_ROW_SIZE,                  std::string_view("could not set the row size attribute")},
            {_RC_SQL_C,                     std::string_view("could not find the corresponding C data type to the SQL data type")},
            {_RC_BIND_COLUMN,               std::string_view("could not bind the current column")}
        };

        // handles
        SimpleSqlTypes::STMT_HANDLE h_stmt;

        // data storage
        SimpleSqlTypes::SQLMatrix m_matrix;
        std::unordered_map<std::string, SimpleSqlTypes::SQLCell> m_key_data;
        std::vector<SimpleSqlTypes::DiagnosticRecord> m_diagnostics;
        std::vector<SimpleSqlTypes::SQLBoundOutput> m_parameter_buffer;
        std::vector<SimpleSqlTypes::SQLBoundOutput> m_column_buffer;

        // utility storage
        std::vector<SimpleSqlTypes::ColumnMetadata> m_columns;
        std::unordered_map<std::string, size_t> m_column_map;
        mutable std::vector<SimpleSqlTypes::SQLCell> m_column;
        mutable std::vector<SimpleSqlTypes::SQLCell> m_row;

        // index trackers
        std::int32_t m_diagnostic_record_number;
        std::uint32_t m_binding_index;

        // idk just basic stuff
        std::uint64_t m_batch_size;
        bool m_is_valid;
        bool m_is_select;
        std::string m_sql;

        // invalid returns
        SimpleSqlTypes::SQLCell m_invalid_cell;
        size_t m_invalid_count;
        std::vector<SimpleSqlTypes::SQLCell> m_invalid_matrix;

        // get internal buffers
        std::uint8_t define_columns();
        void define_diagnostics();

    public:
        SimQuery(SimQuery&&) {}
        SimQuery() : m_matrix(SimpleSqlTypes::SQLMatrix()), m_diagnostic_record_number(1), m_binding_index(1), m_invalid_cell(SimpleSqlTypes::SQLCell()), m_invalid_count(0), m_invalid_matrix(std::vector<SimpleSqlTypes::SQLCell>{}) {}
        ~SimQuery() { destroy(); }
        SimQuery& operator=(SimQuery&&) noexcept {
            return *this;
        }

        // control ownership of statement handle
        bool has_handle() const { return h_stmt != nullptr; }
        bool claim_handle(SimpleSqlTypes::STMT_HANDLE&& stmt_handle);
        SimpleSqlTypes::STMT_HANDLE&& return_handle();

        // setting up the sql statement for execution
        void set_batch_size(const std::uint64_t& batch_size);
        std::uint8_t set_sql(const std::string& sql);
        std::uint8_t prepare();
        std::uint8_t bind_parameter(SimpleSqlTypes::SQLBinding& binding);

        // execution
        bool execute_non_select();
        bool execute_select();
        bool execute();

        // property getters
        bool is_valid() const { return m_is_valid; }
        bool is_select() const { return m_is_select; }
        const size_t& get_row_count(bool& invalid) const;
        const size_t& get_column_count(bool& invalid) const;

        // data getters
        const SimpleSqlTypes::SQLCell& get_cell(const std::string& key) const;
        const SimpleSqlTypes::SQLCell& get_cell(const std::string& column, const size_t& row) const;
        const SimpleSqlTypes::SQLCell& get_cell(const size_t& column, const size_t& row) const;
        const std::vector<SimpleSqlTypes::SQLCell>& get_column(const std::string& column) const;
        const std::vector<SimpleSqlTypes::SQLCell>& get_column(const size_t& column) const;
        const std::vector<SimpleSqlTypes::SQLCell>& get_row(const size_t& row) const;
        const std::vector<SimpleSqlTypes::SQLCell>& get_data() const;
        std::vector<SimpleSqlTypes::SQLCell> claim_data();
        std::string_view return_code_definition(const std::uint8_t& return_code) {
            auto it = m_rc_def.find(return_code);
            if (it != m_rc_def.end())
                return it->second;
            return std::string_view("fallback return code for undefined unsigned 8bit integers");
        }

        // bruh just end me
        void destroy();
    };
}

#endif