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
    class query {
    private:

        struct Output {
            SimpleSqlTypes::SQLBinding binding;
            size_t buffer_size;
        };

        // handles
        std::unique_ptr<void> mp_stmt_handle;

        // members for vector-based results
        uint64_t m_row_count;
        uint8_t m_column_count;
        std::vector<SimpleSqlTypes::Cell> m_vector_data;

        // members for key based results
        std::unordered_map<std::string, SimpleSqlTypes::Cell> m_key_data;

        // default members
        uint16_t m_max_column_name_length;
        bool m_is_select;
        bool m_info_pending;
        int32_t m_diagnostic_record_number;
        uint32_t m_binding_index;
        std::string m_sql;
        std::vector<SimpleSqlTypes::DiagnosticRecord> m_diagnostics;
        std::vector<SimpleSqlTypes::ColumnMetadata> m_columns;
        SimpleSqlTypes<ResultStorageType> m_storage_type;
        std::vector<Output> m_output_buffer;

        // get internal buffers
        bool define_columns(std::string &error);
        void define_diagnostics();

    public:
        query(std::unique_ptr<void> &&stmt_handle, const uint16_t &max_column_name_length) : mp_stmt_handle(std::move(stmt_handle)), m_max_column_name_length(max_column_name_length), m_diagnostic_record_number(1), m_binding_index(1) {}
        ~query() { destroy(); }

        // control ownership of statement handle
        bool claim_handle(std::unique_ptr<void> &&stmt_handle, std::string &error);
        std::unique_ptr<void> return_handle();

        // setting up the sql statement for execution
        bool set_sql(const std::string &sql, std::string &error);
        bool prepare(std::string &error);
        bool bind_parameter(const SimpleSqlTypes::SQLBinding &binding, std::string &error);
        bool bulk_bind_parameters(const std::vector<SimpleSqlTypes::SQLBinding> &bindings, std::string &error);

        // data getters
        const SimpleSqlTypes::Cell& get_cell(const std::string &key) const;
        const SimpleSqlTypes::Cell& get_cell(const std::string &column_name, const int &row) const;
        const SimpleSqlTypes::Cell& get_cell(const int &column_index, const int &row) const;
        const std::vector<SimpleSqlTypes::Cell>& get_data() const;
        const uint64_t& get_row_count() const;
        const uint16_t& get_column_count() const;

        // property getters
        const bool is_select() const { return m_is_select; }

        // bruh just end me
        void destroy();
    };
}

#endif