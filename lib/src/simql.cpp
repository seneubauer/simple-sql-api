#include <simql.h>
#include <query.h>
#include <simresult.h>
#include <memory>
#include <utility>
#include <function>
#include <cstdint>
#include <vector>
#include <sql.h>
#include <sqltypes.h>
#include <sqlext.h>

void SimpleSql::simql::proces() {
    
    /*
    
    run the sql statement using the current statement handle
    m_statements[m_currentstatement]
    
    
    
    */
    // run the sql statement using m_currentstmt->data() as the statement handle
    
    SQLFreeStmt(m_currentstmt->data(), SQL_CLOSE);
    SQLFreeStmt(m_currentstmt->data(), SQL_RESET_PARAMS);
    SQLFreeStmt(m_currentstmt->data(), SQL_UNBIND);
    m_currentstmt = m_currentstmt->next();
}

bool SimpleSql::simql::connect(std::string &conn_str, std::string &error) {

    bool rs = true;
    int16_t rc;

    rc = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &h_env);
    if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO) {
        error = std::string("could not allocate the environment handle");
        rs = false;
        goto end_of_function;
    }

    rc = SqlSetEnvAttr(h_env, SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3, 0);
    if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO) {
        error = std::string("could not set ODBC version to 3");
        rs = false;
        goto free_env_handle;
    }

    rc = SqlAllocHandle(SQL_HANDLE_DBC, h_env, &h_dbc);
    if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO) {
        error = std::string("could not allocate the connection handle");
        rs = false;
        goto free_dbc_handle;
    }

    unsigned char *conn_str_in = conn_str.data();
    unsigned char conn_str_out[1024];
    int16_t conn_str_out_len;
    rc = SQLDriverConnect(h_dbc, nullptr, conn_str_in, SQL_NTS, conn_str_out, sizeof(conn_str_out), &conn_str_out_len, SQL_DRIVER_NOPROMPT);
    if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO) {
        error = std::string("could not open a connection");
        rs = false;
        goto free_dbc_handle;
    }

    for (uint8_t i = 0; i < m_stmtcount; ++i) {
        void* h = nullptr;
        rc = SQLAllocHandle(SQL_HANDLE_STMT, h_dbc, &h);
        rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO ? h = nullptr : (void)0;
        h == nullptr ? m_skipped++ : (void)0;
        h == nullptr ? m_stmtlist.push_front(h) : (void)0;
    }
    m_currentstmt = m_stmtlist.first();

    free_dbc_handle:
    SQLFreeHandle(SQL_HANDLE_DBC, h_dbc);
    
    free_env_handle:
    SQLFreeHandle(SQL_HANDLE_ENV, h_env);
    
    end_of_function:
    return rs;
}

void SimpleSql::simql::disconnect() {
    
}
        
bool SimpleSql::simql::start(const std::string &driver, const std::string &server, const std::string &database, const int &port, const bool &readonly, const bool &trusted, const bool &encrypt, std::string &error) {
    
    // open odbc connection to server/database
    
}

bool SimpleSql::simql::start(const std::string &driver, const std::string &server, const std::string &database, const int &port, const bool &readonly, const bool &trusted, const bool &encrypt, const std::string &username, const std::string &password, std::string &error) {
    
    // open odbc connection to server/database
}

bool SimpleSql::simql::run_sync(SimpleSql::query &_query) {
    
}

void SimpleSql::simql::run_async(std::shared_ptr<SimpleSql::query> _query) {
    
}

void SimpleSql::simql::stop() {
    
    // disconnect from the server/database
}

void SimpleSql::simql::listen(std::shared_ptr<std::function<void(const simresult &result)>> p_listener) {
    mp_listener = std::move(p_listener);
}
