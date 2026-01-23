#ifndef SimDatabase_header_h
#define SimDatabase_header_h

// SimQL stuff
#include <SimQuery.h>
#include <SimQL_Types.h>
#include <SimQL_Utility.h>
#include <SimQL_Constants.h>

// STL stuff
#include <string>
#include <memory>
#include <functional>
#include <utility>
#include <queue>
#include <mutex>
#include <thread>
#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <vector>

namespace SimpleSql {
    class SimDatabase {
    private:

        // handles
        std::unique_ptr<void, SimpleSqlUtility::HandleDeleter> h_env;
        std::unique_ptr<void, SimpleSqlUtility::HandleDeleter> h_dbc;

        // statement members
        std::uint8_t m_stmt_index;
        std::uint8_t m_stmt_count;
        std::uint8_t m_skipped;
        std::vector<std::unique_ptr<void, SimpleSqlUtility::HandleDeleter>> m_stmt_vector;

        // listeners
        std::shared_ptr<std::function<void(SimpleSql::SimQuery&&)>> mp_query_listener;
        std::shared_ptr<std::function<void(std::uint8_t&&)>> mp_stmt_pool_listener;

        // async members
        std::queue<SimpleSql::SimQuery> m_queries;
        std::mutex m_mutex;
        std::thread m_thread;
        std::atomic<bool> m_in_progress;
        std::condition_variable m_cvar;

        // statement functions
        bool remove_stmt_handle();
        bool assign_stmt_handle(SimpleSql::SimQuery& query);
        void reclaim_stmt_handle(SimpleSql::SimQuery& query);

        // processing functions
        std::uint8_t run_query(SimpleSql::SimQuery&& query);
        void process_async();

        // utility functions
        std::uint8_t connect(std::string& conn_str);
        void disconnect();

    public:
        SimDatabase(const std::uint8_t& stmt_count) : m_stmt_count(stmt_count > SimpleSqlConstants::max_statement_handle_pool_size ? SimpleSqlConstants::max_statement_handle_pool_size : stmt_count), m_skipped(0), mp_query_listener(nullptr), m_in_progress(true) {}
        ~SimDatabase() { stop(); }

        std::uint8_t start(const std::string& driver, const std::string& server, const std::string& database, const std::uint16_t& port, const bool& readonly, const bool& trusted, const bool& encrypt);
        std::uint8_t start(const std::string& driver, const std::string& server, const std::string& database, const std::uint16_t& port, const bool& readonly, const bool& trusted, const bool& encrypt, const std::string& username, const std::string& password);
        std::uint8_t run_sync(SimpleSql::SimQuery& query);
        void run_async(SimpleSql::SimQuery&& query);
        void run_parallel(std::uint8_t& thread_count, std::vector<SimpleSql::SimQuery>& queries);
        void stop();
        void listen(std::shared_ptr<std::function<void(SimpleSql::SimQuery&&)>> p_listener);
        void listen(std::shared_ptr<std::function<void(std::uint8_t&&)>> p_listener);
    };
}

#endif