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
#include <future>

// for compiling on Windows (ew)
#ifdef _WIN32

#define WIN32_LEAN_AND_MEAN
#define NOGDICAPMASKS               // CC_*, LC_*, PC_*, CP_*, TC_*, RC_
#define NOVIRTUALKEYCODES           // VK_*
#define NOWINMESSAGES               // WM_*, EM_*, LB_*, CB_*
#define NOWINSTYLES                 // WS_*, CS_*, ES_*, LBS_*, SBS_*, CBS_*
#define NOSYSMETRICS                // SM_*
#define NOMENUS                     // MF_*
#define NOICONS                     // IDI_*
#define NOKEYSTATES                 // MK_*
#define NOSYSCOMMANDS               // SC_*
#define NORASTEROPS                 // Binary and Tertiary raster ops
#define NOSHOWWINDOW                // SW_*
#define OEMRESOURCE                 // OEM Resource values
#define NOATOM                      // Atom Manager routines
#define NOCLIPBOARD                 // Clipboard routines
#define NOCOLOR                     // Screen colors
#define NOCTLMGR                    // Control and Dialog routines
#define NODRAWTEXT                  // DrawText() and DT_*
#define NOGDI                       // All GDI defines and routines
#define NOKERNEL                    // All KERNEL defines and routines
#define NOUSER                      // All USER defines and routines
#define NONLS                       // All NLS defines and routines
#define NOMB                        // MB_* and MessageBox()
#define NOMEMMGR                    // GMEM_*, LMEM_*, GHND, LHND, associated routines
#define NOMETAFILE                  // typedef METAFILEPICT
// #define NOMINMAX                 // Macros min(a,b) and max(a,b) || already set in mingw os_defines.h
#define NOMSG                       // typedef MSG and associated routines
#define NOOPENFILE                  // OpenFile(), OemToAnsi, AnsiToOem, and OF_*
#define NOSCROLL                    // SB_* and scrolling routines
#define NOSERVICE                   // All Service Controller routines, SERVICE_ equates, etc.
#define NOSOUND                     // Sound driver routines
#define NOTEXTMETRIC                // typedef TEXTMETRIC and associated routines
#define NOWH                        // SetWindowsHook and WH_*
#define NOWINOFFSETS                // GWL_*, GCL_*, associated routines
#define NOCOMM                      // COMM driver routines
#define NOKANJI                     // Kanji support stuff.
#define NOHELP                      // Help engine interface.
#define NOPROFILER                  // Profiler interface.
#define NODEFERWINDOWPOS            // DeferWindowPos routines
#define NOMCX                       // Modem Configuration Extensions
#include <windows.h>

#endif

// ODBC stuff
#include <sqltypes.h>
#include <sqlext.h>
#include <sql.h>

bool SimpleSql::SimDatabase::remove_stmt_handle() {

    // remove handle from the vector
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_stmt_vector.erase(m_stmt_vector.begin() + m_stmt_index);
        if (m_stmt_index >= m_stmt_vector.size())
            m_stmt_index = 0;
    }

    // run the statement pool listener
    std::uint8_t remaining_stmt_count = m_stmt_vector.size();
    (*mp_stmt_pool_listener)(std::move(remaining_stmt_count));

    return m_stmt_vector.size() > 0 ? true : false;
}

bool SimpleSql::SimDatabase::assign_stmt_handle(SimpleSql::SimQuery& query) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!query.claim_handle(std::move(m_stmt_vector[m_stmt_index]))) {

        // the handle could not be moved b/c it is null, so try to make a new one it its place
        SQLHANDLE h;
        SQLRETURN sr = SQLAllocHandle(SQL_HANDLE_STMT, h_dbc.get(), &h);
        if (sr != SQL_SUCCESS && sr != SQL_SUCCESS_WITH_INFO)
            if (!remove_stmt_handle())
                return false;

        // assign the new handle to the vector
        m_stmt_vector[m_stmt_index] = std::unique_ptr<void, SimpleSqlUtility::HandleDeleter>(h);
        query.claim_handle(std::move(m_stmt_vector[m_stmt_index]));
    }

    // advance to the next index so the next assignment starts on the next handle
    m_stmt_index++;
    if (m_stmt_index >= m_stmt_vector.size())
        m_stmt_index = 0;

    return true;
}

void SimpleSql::SimDatabase::reclaim_stmt_handle(SimpleSql::SimQuery& query) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_stmt_vector[m_stmt_index] = query.return_handle();

    auto make_stmt_handle = [&](SQLHANDLE &h) -> bool {
        SQLRETURN sr;
        sr = SQLAllocHandle(SQL_HANDLE_STMT, h_dbc.get(), &h);
        if (sr != SQL_SUCCESS && sr != SQL_SUCCESS_WITH_INFO)
            return false;

        return true;
    };

    SQLRETURN sr;
    sr = SQLFreeStmt(m_stmt_vector[m_stmt_index].get(), SQL_CLOSE);
    if (sr != SQL_SUCCESS && sr != SQL_SUCCESS_WITH_INFO) {
        SQLHANDLE h;
        if (!make_stmt_handle(h)) {
            remove_stmt_handle();
            return;
        }
        m_stmt_vector[m_stmt_index].reset(h);
        return;
    }

    sr = SQLFreeStmt(m_stmt_vector[m_stmt_index].get(), SQL_RESET_PARAMS);
    if (sr != SQL_SUCCESS && sr != SQL_SUCCESS_WITH_INFO) {
        SQLHANDLE h;
        if (!make_stmt_handle(h)) {
            remove_stmt_handle();
            return;
        }
        m_stmt_vector[m_stmt_index].reset(h);
        return;
    }

    sr = SQLFreeStmt(m_stmt_vector[m_stmt_index].get(), SQL_UNBIND);
    if (sr != SQL_SUCCESS && sr != SQL_SUCCESS_WITH_INFO) {
        SQLHANDLE h;
        if (!make_stmt_handle(h)) {
            remove_stmt_handle();
            return;
        }
        m_stmt_vector[m_stmt_index].reset(h);
        return;
    }
}

std::uint8_t SimpleSql::SimDatabase::run_query(SimpleSql::SimQuery&& query) {

    if (!assign_stmt_handle(query))
        return SimpleSqlConstants::ReturnCodes::D_STMT_HANDLE_ASSIGNMENT;

    std::uint8_t rc = SimpleSqlConstants::ReturnCodes::SUCCESS;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        // run the query
    }
    reclaim_stmt_handle(query);

    if (mp_query_listener)
        (*mp_query_listener)(std::move(query));

    return rc;
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

std::uint8_t SimpleSql::SimDatabase::connect(std::string& conn_str) {

    std::uint8_t rc;
    SQLRETURN sr;
    SQLCHAR* conn_str_in = const_cast<SQLCHAR*>(reinterpret_cast<const SQLCHAR*>(conn_str.c_str()));
    unsigned char conn_str_out[1024];

    SQLHANDLE env;
    sr = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &env);
    if (sr != SQL_SUCCESS && sr != SQL_SUCCESS_WITH_INFO) {
        rc = SimpleSqlConstants::ReturnCodes::D_ENV_HANDLE_ALLOC;
        goto end_of_function;
    }
    h_env = std::unique_ptr<void, SimpleSqlUtility::HandleDeleter>(env);

    sr = SQLSetEnvAttr(h_env.get(), SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3, 0);
    if (sr != SQL_SUCCESS && sr != SQL_SUCCESS_WITH_INFO) {
        rc = SimpleSqlConstants::ReturnCodes::D_ODBC_VERSION3;
        goto free_env_handle;
    }

    SQLHANDLE dbc;
    sr = SQLAllocHandle(SQL_HANDLE_DBC, h_env.get(), &dbc);
    if (sr != SQL_SUCCESS && sr != SQL_SUCCESS_WITH_INFO) {
        rc = SimpleSqlConstants::ReturnCodes::D_DBC_HANDLE_ALLOC;
        goto free_dbc_handle;
    }
    h_dbc = std::unique_ptr<void, SimpleSqlUtility::HandleDeleter>(dbc);

    SQLSMALLINT conn_str_out_len;
    sr = SQLDriverConnect(h_dbc.get(), nullptr, conn_str_in, SQL_NTS, conn_str_out, sizeof(conn_str_out), &conn_str_out_len, SQL_DRIVER_NOPROMPT);
    if (sr != SQL_SUCCESS && sr != SQL_SUCCESS_WITH_INFO) {
        rc = SimpleSqlConstants::ReturnCodes::D_CONNECTION;
        goto free_dbc_handle;
    }

    for (std::uint8_t i = 0; i < m_stmt_count; ++i) {
        SQLHANDLE h;
        sr = SQLAllocHandle(SQL_HANDLE_STMT, h_dbc.get(), &h);

        if (sr != SQL_SUCCESS && sr != SQL_SUCCESS_WITH_INFO) {
            m_skipped++;
            continue;
        }
        m_stmt_vector.push_back(std::unique_ptr<void, SimpleSqlUtility::HandleDeleter>(h));
    }

    end_of_function:
    return SimpleSqlConstants::ReturnCodes::SUCCESS;

    free_dbc_handle:
    SQLFreeHandle(SQL_HANDLE_DBC, h_dbc.get());

    free_env_handle:
    SQLFreeHandle(SQL_HANDLE_ENV, h_env.get());

    return rc;
}

void SimpleSql::SimDatabase::disconnect() {

    for (std::unique_ptr<void, SimpleSqlUtility::HandleDeleter>& h : m_stmt_vector) {
        if (h.get() != SQL_NULL_HANDLE)
            SQLFreeHandle(SQL_HANDLE_STMT, h.get());
    }

    SQLDisconnect(h_dbc.get());
    SQLFreeHandle(SQL_HANDLE_DBC, h_dbc.get());
    SQLFreeHandle(SQL_HANDLE_ENV, h_env.get());

}

std::uint8_t SimpleSql::SimDatabase::start(const std::string& driver, const std::string& server, const std::string& database, const std::uint16_t& port, const bool& readonly, const bool& trusted, const bool& encrypt) {

    auto conn_str = SimpleSqlUtility::connection_string(driver, server, database, port, readonly, trusted, encrypt);
    std::uint8_t rc = connect(conn_str);
    if (rc > 0)
        return rc;

    m_thread = std::thread(&SimpleSql::SimDatabase::process_async, this);
    return SimpleSqlConstants::ReturnCodes::SUCCESS;
}

std::uint8_t SimpleSql::SimDatabase::start(const std::string& driver, const std::string& server, const std::string& database, const std::uint16_t& port, const bool& readonly, const bool& trusted, const bool& encrypt, const std::string& username, const std::string& password) {
    
    auto conn_str = SimpleSqlUtility::connection_string(driver, server, database, port, readonly, trusted, encrypt, username, password);
    std::uint8_t rc = connect(conn_str);
    if (rc > 0)
        return rc;

    m_thread = std::thread(&SimpleSql::SimDatabase::process_async, this);
    return SimpleSqlConstants::ReturnCodes::SUCCESS;
}

std::uint8_t SimpleSql::SimDatabase::run_sync(SimpleSql::SimQuery& query) {
    if (!assign_stmt_handle(query))
        return SimpleSqlConstants::ReturnCodes::D_STMT_HANDLE_ASSIGNMENT;

    // run the query

    reclaim_stmt_handle(query);

    return SimpleSqlConstants::ReturnCodes::SUCCESS;
}

void SimpleSql::SimDatabase::run_async(SimpleSql::SimQuery&& query) {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_queries.push(std::move(query));
    }
    m_cvar.notify_one();
}

void SimpleSql::SimDatabase::run_parallel(std::uint8_t& thread_count, std::vector<SimpleSql::SimQuery>& queries) {

    auto execute = [&](size_t&& index, SimpleSql::SimQuery&& query, std::promise<std::pair<size_t, SimpleSql::SimQuery>>&& prom) -> void {

        if (!assign_stmt_handle(query))
            goto end_of_lambda;

        {
            std::lock_guard<std::mutex> lock(m_mutex);
            // run the query
        }

        reclaim_stmt_handle(query);

        end_of_lambda:
        auto output = std::make_pair<size_t&&, SimpleSql::SimQuery&&>(std::move(index), std::move(query));
        prom.set_value(std::move(output));
    };

    if (thread_count > SimpleSqlConstants::max_parallel_query_count)
        thread_count = SimpleSqlConstants::max_parallel_query_count;

    for (size_t i = 0; i < queries.size(); i += thread_count) {
        std::uint8_t used_threads = i + thread_count <= queries.size() ? thread_count : queries.size() % thread_count;
        
        std::vector<std::thread> threads;
        threads.reserve(used_threads);

        std::vector<std::future<std::pair<size_t, SimpleSql::SimQuery>>> futures;
        futures.reserve(used_threads);

        for (std::uint8_t j = 0; j < used_threads; ++j) {
            std::promise<std::pair<size_t, SimpleSql::SimQuery>> p;
            futures.push_back(p.get_future());
            size_t index = static_cast<size_t>(j) + i;
            threads.emplace_back(execute, std::move(index), std::move(queries[index]), std::move(p));
        }

        // collect results
        for (std::uint8_t j = 0; j < used_threads; ++j) {
            auto result_pair = futures[j].get();
            queries[result_pair.first] = std::move(result_pair.second);
        }

        // let all threads complete
        for (std::thread& t : threads)
            if (t.joinable())
                t.join();
    }
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
    mp_query_listener = std::move(p_listener);
}

void SimpleSql::SimDatabase::listen(std::shared_ptr<std::function<void(std::uint8_t&&)>> p_listener) {
    mp_stmt_pool_listener = std::move(p_listener);
}