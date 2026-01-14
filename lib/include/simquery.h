#ifndef simquery_header_h
#define simquery_header_h

#include <simql_types.h>
#include <unordered_map>
#include <vector>
#include <string>
#include <cstdint>
#include <memory>

namespace SimpleSql {
    class simquery {
    private:

        // handles
        void** mp_stmthandle;

        // members for vector-based results
        uint64_t m_rowcount;
        uint8_t m_columncount;
        std::vector<SimpleSqlTypes::Cell> m_vectordata;

        // members for key based results
        std::unordered_map<std::string, SimpleSqlTypes::Cell> m_keydata;

        // default members
        std::string m_sql;
        std::vector<SimpleSqlTypes::SqlColumn> m_columns;
        std::unordered_map<std::string, uint8_t> m_columnindices;

        // methods
        void define_columns();

    public:
        simquery() {}
        ~simquery() { destroy(); }

        void set_handle(void** stmthandle);
        
        void set_sql(const std::string &sql);
        void prepare();
        
        SimpleSqlTypes::Cell get_cell(const std::string &key);
        SimpleSqlTypes::Cell get_cell(const std::string &column_name, const int &row);
        SimpleSqlTypes::Cell get_cell(const int &column_index, const int &row);
        std::vector<SimpleSqlTypes::Cell> claim_data();
        uint64_t get_rowcount();
        uint8_t get_columncount();
        void destroy();
    };
}

#endif


/*

REQUIRED: raw SQL
REQUIRED: specify if data storage is vector or key based
OPTIONAL: specify input/output/inputoutput bindings
OPTIONAL: specify resulting columns -> build tuple data bindings
OPTIONAL: supply optimization parameters for the query
REQUIRED: prepare
REQUIRED: execute

ON FAILURE: get ODBC diagnostic dump and return false

ON SUCCESS:
        UNDEFINED COLUMNS: build tuple data bindings
        
        
        
        RETURN: std::vector<std::tuple>>



*/