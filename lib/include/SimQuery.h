#ifndef SimQuery_header_h
#define SimQuery_header_h

// SimQL stuff
#include <SimQL_Types.h>
#include <SimQL_Utility.h>

// STL stuff
#include <unordered_map>
#include <vector>
#include <string>
#include <cstdint>
#include <memory>

namespace SimpleSql {
    class SimQuery {
    private:

        // handles
        std::unique_ptr<void, SimpleSqlUtility::HandleDeleter> mp_stmt_handle;

        // data storage
        SimpleSqlTypes::SQLMatrix m_matrix;
        std::unordered_map<std::string, SimpleSqlTypes::SQLCell> m_key_data;
        std::vector<SimpleSqlTypes::DiagnosticRecord> m_diagnostics;
        std::vector<SimpleSqlTypes::SQLBoundOutput> m_output_buffer;

        // utility storage
        std::vector<SimpleSqlTypes::ColumnMetadata> m_columns;
        std::unordered_map<std::string, size_t> m_column_map;
        mutable std::vector<SimpleSqlTypes::SQLCell> m_column;
        mutable std::vector<SimpleSqlTypes::SQLCell> m_row;

        // index trackers
        std::int32_t m_diagnostic_record_number;
        std::uint32_t m_binding_index;

        // idk just basic stuff
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
        SimQuery() : m_matrix(SimpleSqlTypes::SQLMatrix()), m_diagnostic_record_number(1), m_binding_index(1), m_invalid_cell(SimpleSqlTypes::SQLCell()), m_invalid_count(0), m_invalid_matrix(std::vector<SimpleSqlTypes::SQLCell>{}) {}
        ~SimQuery() { destroy(); }
        SimQuery& operator=(SimQuery&&) = default;

        // control ownership of statement handle
        bool has_handle() const { return mp_stmt_handle != nullptr; }
        bool claim_handle(std::unique_ptr<void, SimpleSqlUtility::HandleDeleter>&& stmt_handle);
        std::unique_ptr<void, SimpleSqlUtility::HandleDeleter> return_handle();

        // setting up the sql statement for execution
        std::uint8_t set_sql(const std::string& sql);
        std::uint8_t prepare();
        std::uint8_t bind_parameter(SimpleSqlTypes::SQLBinding& binding);

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

        // bruh just end me
        void destroy();
    };
}

#endif