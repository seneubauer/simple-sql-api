#include <simquery.h>
#include <sql.h>
#include <sqlext.h>

void SimpleSql::simquery::define_columns() {
    
    SQLNumResultCols()
    
    // iterate through the columns and get the descriptions
    // if there are no columns, then this is a non-select query with no return-able data
    
    SQLDescribeCol(*mp_stmthandle, )
}

void SimpleSql::simquery::set_handle(void** stmthandle) {
    mp_stmthandle = stmthandle;
}

void SimpleSql::simquery::set_sql(const std::string &sql) {
    m_sql = sql;
}

void SimpleSql::simquery::prepare() {
    
    SQLCHAR* sql = reinterpret_cast<SQLCHAR*>(const_cast<char*>(m_sql.c_str()));
    
    int16_t rc;
    
    // prepare here
    rc = SQLPrepare(*mp_stmthandle, sql, SQL_NTS);
    if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO) {
        
    }
    
    
    define_columns();
}

SimpleSqlTypes::Cell SimpleSql::simquery::get_cell(const std::string &key) {
    
}

SimpleSqlTypes::Cell SimpleSql::simquery::get_cell(const std::string &column_name, const int &row) {
    
}

SimpleSqlTypes::Cell SimpleSql::simquery::get_cell(const int &column_index, const int &row) {
    
}

std::vector<SimpleSqlTypes::Cell> SimpleSql::simquery::claim_data() {
    
}

uint64_t SimpleSql::simquery::get_rowcount() {
    
}

uint8_t SimpleSql::simquery::get_columncount() {
    
}

void SimpleSql::simquery::destroy() {
    
}