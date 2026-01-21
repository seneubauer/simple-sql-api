// SimDatabase stuff
#include <SimDatabase.h>
#include <SimQuery.h>

// STL stuff
#include <memory>
#include <utility>
#include <functional>
#include <cstdint>
#include <vector>

// ODBC stuff
#include <sql.h>
#include <sqltypes.h>
#include <sqlext.h>

void SimpleSql::SimDatabase::process() {
    
    /*
    
    run the sql statement using the current statement handle
    m_statements[m_currentstatement]
    
    
    
    */
    // run the sql statement using m_current_stmt->data() as the statement handle
    
    SQLFreeStmt(m_stmt_vector[m_stmt_index].get(), SQL_CLOSE);
    SQLFreeStmt(m_stmt_vector[m_stmt_index].get(), SQL_RESET_PARAMS);
    SQLFreeStmt(m_stmt_vector[m_stmt_index].get(), SQL_UNBIND);

    m_stmt_index++;
    if (m_stmt_index >= m_stmt_vector.size())
        m_stmt_index = 0;
}

bool SimpleSql::SimDatabase::connect(std::string &conn_str, std::string &error) {

    SQLRETURN return_code;
    SQLCHAR* conn_str_in = const_cast<SQLCHAR*>(reinterpret_cast<const SQLCHAR*>(conn_str.c_str()));
    unsigned char conn_str_out[1024];

    SQLHANDLE env;
    return_code = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &env);
    if (return_code != SQL_SUCCESS && return_code != SQL_SUCCESS_WITH_INFO) {
        error = std::string("could not allocate the environment handle");
        goto end_of_function;
    }
    h_env = std::unique_ptr<void>(env);

    return_code = SQLSetEnvAttr(h_env.get(), SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3, 0);
    if (return_code != SQL_SUCCESS && return_code != SQL_SUCCESS_WITH_INFO) {
        error = std::string("could not set ODBC version to 3");
        goto free_env_handle;
    }

    SQLHANDLE dbc;
    return_code = SQLAllocHandle(SQL_HANDLE_DBC, h_env.get(), &dbc);
    if (return_code != SQL_SUCCESS && return_code != SQL_SUCCESS_WITH_INFO) {
        error = std::string("could not allocate the connection handle");
        goto free_dbc_handle;
    }
    h_dbc = std::unique_ptr<void>(dbc);

    SQLSMALLINT conn_str_out_len;
    return_code = SQLDriverConnect(h_dbc.get(), nullptr, conn_str_in, SQL_NTS, conn_str_out, sizeof(conn_str_out), &conn_str_out_len, SQL_DRIVER_NOPROMPT);
    if (return_code != SQL_SUCCESS && return_code != SQL_SUCCESS_WITH_INFO) {
        error = std::string("could not open a connection");
        goto free_dbc_handle;
    }

    for (uint8_t i = 0; i < m_stmt_count; ++i) {
        SQLHANDLE h;
        return_code = SQLAllocHandle(SQL_HANDLE_STMT, h_dbc.get(), &h);

        if (return_code != SQL_SUCCESS && return_code != SQL_SUCCESS_WITH_INFO) {
            m_skipped++;
            continue;
        }
        m_stmt_vector.push_back(std::unique_ptr<void>(h));
    }

    end_of_function:
    return true;

    free_dbc_handle:
    SQLFreeHandle(SQL_HANDLE_DBC, h_dbc.get());

    free_env_handle:
    SQLFreeHandle(SQL_HANDLE_ENV, h_env.get());

    return false;
}

void SimpleSql::SimDatabase::disconnect() {
    
}
        
bool SimpleSql::SimDatabase::start(const std::string &driver, const std::string &server, const std::string &database, const int &port, const bool &readonly, const bool &trusted, const bool &encrypt, std::string &error) {
    
    // open odbc connection to server/database
    
}

bool SimpleSql::SimDatabase::start(const std::string &driver, const std::string &server, const std::string &database, const int &port, const bool &readonly, const bool &trusted, const bool &encrypt, const std::string &username, const std::string &password, std::string &error) {
    
    // open odbc connection to server/database
}

bool SimpleSql::SimDatabase::run_sync(SimpleSql::SimQuery &_query) {
    
}

void SimpleSql::SimDatabase::run_async(std::shared_ptr<SimpleSql::SimQuery> _query) {
    
}

void SimpleSql::SimDatabase::stop() {
    
    // disconnect from the server/database
}

void SimpleSql::SimDatabase::listen(std::shared_ptr<std::function<void(const SimpleSql::SimQuery &query)>> p_listener) {
    mp_listener = std::move(p_listener);
}
