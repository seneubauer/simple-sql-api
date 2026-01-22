#ifndef SimQuery_header_h
#define SimQuery_header_h

// SimQL stuff
#include <SimQL_Types.h>

// STL stuff
#include <unordered_map>
#include <vector>
#include <string>
#include <cstdint>
#include <memory>

namespace SimpleSql {
    class SimQuery {
    private:

        // constants
        static constexpr uint16_t cm_colname_size = 256U;

        // handles
        std::unique_ptr<void> mp_stmt_handle;

        // data storage
        SimpleSqlTypes::SQLMatrix m_matrix;
        std::unordered_map<std::string, SimpleSqlTypes::SQLCell> m_key_data;
        std::vector<SimpleSqlTypes::DiagnosticRecord> m_diagnostics;
        std::vector<SimpleSqlTypes::SQLBoundOutput> m_output_buffer;

        // utility storage
        std::vector<SimpleSqlTypes::ColumnMetadata> m_columns;
        std::unordered_map<std::string, size_t> m_column_map;

        // index trackers
        int32_t m_diagnostic_record_number;
        uint32_t m_binding_index;

        // idk just basic stuff
        bool m_is_select;
        std::string m_sql;
        SimpleSqlTypes::SQLCell m_invalid_cell;

        // get internal buffers
        uint8_t define_columns();
        void define_diagnostics();

    public:
        SimQuery() : m_diagnostic_record_number(1), m_binding_index(1), m_matrix(SimpleSqlTypes::SQLMatrix()), m_invalid_cell(SimpleSqlTypes::SQLCell()) {}
        ~SimQuery() { destroy(); }
        SimQuery& operator=(SimQuery&&) = default;

        // control ownership of statement handle
        const bool has_handle() const { return mp_stmt_handle != nullptr; }
        bool claim_handle(std::unique_ptr<void> &&stmt_handle);
        std::unique_ptr<void> return_handle();

        // setting up the sql statement for execution
        uint8_t set_sql(const std::string &sql);
        uint8_t prepare();
        uint8_t bind_parameter(const SimpleSqlTypes::SQLBinding &binding);

        // property getters
        bool is_select() const { return m_is_select; }
        const size_t& get_row_count() const;
        const size_t& get_column_count() const;

        // data getters
        const SimpleSqlTypes::SQLCell& get_cell(const std::string &key) const;
        const SimpleSqlTypes::SQLCell& get_cell(const std::string &column, const size_t &row) const;
        const SimpleSqlTypes::SQLCell& get_cell(const size_t &column, const size_t &row) const;
        const std::vector<SimpleSqlTypes::SQLCell>& get_column(const std::string &column) const;
        const std::vector<SimpleSqlTypes::SQLCell>& get_column(const size_t &column) const;
        const std::vector<SimpleSqlTypes::SQLCell>& get_row(const size_t &row) const;
        const std::vector<SimpleSqlTypes::SQLCell>& get_data() const;
        std::vector<SimpleSqlTypes::SQLCell> claim_data();

        // bruh just end me
        void destroy();
    };
}

#endif