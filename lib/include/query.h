#ifndef query_header_h
#define query_header_h

#include <simql_types.h>
#include <unordered_map>
#include <vector>
#include <string>
#include <cstdint>
#include <memory>

namespace SimpleSql {
    class query {
    private:

        // handles
        void* mp_stmt_handle;

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
        std::string m_sql;
        std::vector<SimpleSqlTypes::DiagnosticRecord> m_diagnostics;
        std::vector<SimpleSqlTypes::ColumnMetadata> m_columns;
        SimpleSqlTypes<ResultStorageType> m_storage_type;

        // private operational methods
        bool define_columns(std::string &error);
        void define_diagnostics(const bool &reset);

    public:
        query(const uint16_t &max_column_name_length) : m_max_column_name_length(max_column_name_length) {}
        ~query() { destroy(); }

        // operational methods
        void set_handle(void* stmt_handle);
        void set_sql(const std::string &sql);
        bool prepare(std::string &error);
        void destroy();

        // property getters
        const bool is_select() const { return m_is_select; }

        // data getters
        const SimpleSqlTypes::Cell& get_cell(const std::string &key) const;
        const SimpleSqlTypes::Cell& get_cell(const std::string &column_name, const int &row) const;
        const SimpleSqlTypes::Cell& get_cell(const int &column_index, const int &row) const;
        const std::vector<SimpleSqlTypes::Cell>& get_data() const;
        const uint64_t& get_row_count() const;
        const uint16_t& get_column_count() const;
    };
}

#endif