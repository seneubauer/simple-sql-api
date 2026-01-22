// SimQL stuff
#include <SimDatabase.h>
#include <SimQuery.h>
#include <SimQL_Types.h>
#include <SimQL_Utility.h>
#include <SimQL_Constants.h>

// STL stuff
#include <memory>
#include <utility>
#include <functional>
#include <cstdint>
#include <vector>
#include <mutex>
#include <condition_variable>

// ODBC stuff
#include <sql.h>
#include <sqltypes.h>
#include <sqlext.h>

const bool SimpleSql::SimDatabase::assign_stmt_handle(SimpleSql::SimQuery &query) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!query.claim_handle(std::move(m_stmt_vector[m_stmt_index]), m_last_error)) {
        SQLHANDLE h;
        SQLRETURN return_code = SQLAllocHandle(SQL_HANDLE_STMT, h_dbc.get(), &h);
        if (return_code != SQL_SUCCESS && return_code != SQL_SUCCESS_WITH_INFO) {
            m_last_error = std::string("could not repair broken statement handle");
            return false;
        }
        m_stmt_vector[m_stmt_index] = std::unique_ptr<void>(h);
        if (!query.claim_handle(std::move(m_stmt_vector[m_stmt_index]), m_last_error)) {
            m_last_error = std::string("a broken statement handle was repaired but could not be assigned");
            return false;
        }
    }

    m_stmt_index++;
    if (m_stmt_index >= m_stmt_vector.size())
        m_stmt_index = 0;

    return true;
}

const bool SimpleSql::SimDatabase::reclaim_stmt_handle(SimpleSql::SimQuery &query) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_stmt_vector[m_stmt_index] = query.return_handle();

    SQLRETURN return_code;
    return_code = SQLFreeStmt(m_stmt_vector[m_stmt_index].get(), SQL_CLOSE);
    if (return_code != SQL_SUCCESS && return_code != SQL_SUCCESS_WITH_INFO) {
        m_last_error = std::string("could not close the recycled statement handle");
        return false;
    }

    SQLFreeStmt(m_stmt_vector[m_stmt_index].get(), SQL_RESET_PARAMS);
    if (return_code != SQL_SUCCESS && return_code != SQL_SUCCESS_WITH_INFO) {
        m_last_error = std::string("could not reset the statement handle parameter(s)");
        return false;
    }

    SQLFreeStmt(m_stmt_vector[m_stmt_index].get(), SQL_UNBIND);
    if (return_code != SQL_SUCCESS && return_code != SQL_SUCCESS_WITH_INFO) {
        m_last_error = std::string("could not unbind the statement handle");
        return false;
    }

    return true;
}

const bool SimpleSql::SimDatabase::run_query(SimpleSql::SimQuery&& query) {

    bool return_state = assign_stmt_handle(query);
    if (!return_state)
        goto listener_emit;

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return_state = true; // perform query execution
    }
    return_state = reclaim_stmt_handle(query);

    listener_emit:
    if (mp_listener)
        (*mp_listener)(std::move(query));

    return return_state;
}

void SimpleSql::SimDatabase::process_async() {
    while (true) {
        SimpleSql::SimQuery query;
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_cvar.wait(lock, [&] { return !m_queries.empty() || !m_in_progress; });
            if (!m_in_progress && m_queries.empty())
                return;

            query = std::move(m_queries.front());
            m_queries.pop();
        }
        run_query(std::move(query));
    }
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

const uint8_t SimpleSql::SimDatabase::start(const std::string &driver, const std::string &server, const std::string &database, const int &port, const bool &readonly, const bool &trusted, const bool &encrypt) {

    auto conn_str = SimpleSqlUtility::connection_string(driver, server, database, port, readonly, trusted, encrypt);
    if (!connect(conn_str, m_last_error))
        return false;

    m_thread = std::thread(&SimpleSql::SimDatabase::process_async, this);
    return true;
}

const uint8_t SimpleSql::SimDatabase::start(const std::string &driver, const std::string &server, const std::string &database, const int &port, const bool &readonly, const bool &trusted, const bool &encrypt, const std::string &username, const std::string &password) {
    
    auto conn_str = SimpleSqlUtility::connection_string(driver, server, database, port, readonly, trusted, encrypt, username, password);
    if (!connect(conn_str, m_last_error))
        return false;

    m_thread = std::thread(&SimpleSql::SimDatabase::process_async, this);
    return true;
}

const uint8_t SimpleSql::SimDatabase::run_sync(SimpleSql::SimQuery &query) {
    // process the query with the native thread

    uint8_t return_code;

    if (!assign_stmt_handle(query)) {
        return_code = 0;
        goto end_of_function;
    }
        

    // run the query

    reclaim_stmt_handle(query);

    end_of_function:
    return return_state;
}

void SimpleSql::SimDatabase::run_async(SimpleSql::SimQuery query) {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_queries.push(query);
    }
    m_cvar.notify_one();
}

void SimpleSql::SimDatabase::run_parallel(const uint8_t &max_concurrency, std::vector<SimpleSql::SimQuery> &queries) {

}

void SimpleSql::SimDatabase::stop() {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_in_progress = false;
    }
    m_cvar.notify_one();
    if (m_thread.joinable())
        m_thread.join();

    disconnect();
}

void SimpleSql::SimDatabase::listen(std::shared_ptr<std::function<void(SimpleSql::SimQuery&&)>> p_listener) {
    mp_listener = std::move(p_listener);
}
