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

        // handles
        std::unique_ptr<void> h_env;
        std::unique_ptr<void> h_dbc;

        // statement members
        uint8_t m_stmt_index;
        uint8_t m_stmt_count;
        uint8_t m_skipped;
        std::vector<std::unique_ptr<void>> m_stmt_vector;

        // async members
        std::shared_ptr<std::function<void(const SimpleSql::SimQuery &query)>> mp_listener;
        std::queue<SimpleSql::SimQuery> m_queries;
        std::mutex m_mutex;
        std::thread m_thread;
        std::atomic<bool> m_in_progress;
        std::condition_variable m_cvar;

        // async functions
        void process();

        // utility functions
        bool connect(std::string &conn_str, std::string &error);
        void disconnect();

    public:
        SimDatabase(const uint8_t &stmt_count) : m_stmt_count(stmt_count), m_skipped(0) {}
        ~SimDatabase() { stop(); }

        bool start(const std::string &driver, const std::string &server, const std::string &database, const int &port, const bool &readonly, const bool &trusted, const bool &encrypt, std::string &error);
        bool start(const std::string &driver, const std::string &server, const std::string &database, const int &port, const bool &readonly, const bool &trusted, const bool &encrypt, const std::string &username, const std::string &password, std::string &error);
        bool run_sync(SimpleSql::SimQuery &_query);
        void run_async(std::shared_ptr<SimpleSql::SimQuery> _query);
        void run_parallel(const uint8_t &max_concurrency, std::vector<SimpleSql::SimQuery> &queries);
        void stop();
        void listen(std::shared_ptr<std::function<void(const SimpleSql::SimQuery &query)>> p_listener);

    };
}

#endif