#ifndef SimDatabase_header_h
#define SimDatabase_header_h

// SimQL stuff
#include <SimQuery.h>
#include <SimQL_Types.h>
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

        // utility
        std::string m_last_error;

        // handles
        std::unique_ptr<void> h_env;
        std::unique_ptr<void> h_dbc;

        // statement members
        uint8_t m_stmt_index;
        uint8_t m_stmt_count;
        uint8_t m_skipped;
        std::vector<std::unique_ptr<void>> m_stmt_vector;

        // async members
        std::shared_ptr<std::function<void(SimpleSql::SimQuery&&)>> mp_listener;
        std::queue<SimpleSql::SimQuery> m_queries;
        std::mutex m_mutex;
        std::thread m_thread;
        std::atomic<bool> m_in_progress;
        std::condition_variable m_cvar;

        // statement functions
        const bool assign_stmt_handle(SimpleSql::SimQuery &query);
        const bool reclaim_stmt_handle(SimpleSql::SimQuery &query);

        // processing functions
        const bool run_query(SimpleSql::SimQuery&& query);
        void process_async();

        // utility functions
        bool connect(std::string &conn_str, std::string &error);
        void disconnect();

    public:
        SimDatabase(const uint8_t &stmt_count) : m_stmt_count(stmt_count), m_skipped(0), m_in_progress(true), mp_listener(nullptr) {}
        ~SimDatabase() { stop(); }

        const std::string& last_error() const { return m_last_error; }
        const uint8_t start(const std::string &driver, const std::string &server, const std::string &database, const int &port, const bool &readonly, const bool &trusted, const bool &encrypt);
        const uint8_t start(const std::string &driver, const std::string &server, const std::string &database, const int &port, const bool &readonly, const bool &trusted, const bool &encrypt, const std::string &username, const std::string &password);
        const uint8_t run_sync(SimpleSql::SimQuery &query);
        void run_async(SimpleSql::SimQuery query);
        void run_parallel(const uint8_t &max_concurrency, std::vector<SimpleSql::SimQuery> &queries);
        void stop();
        void listen(std::shared_ptr<std::function<void(SimpleSql::SimQuery&&)>> p_listener);
    };
}

#endif