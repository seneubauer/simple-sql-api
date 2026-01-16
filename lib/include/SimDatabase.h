#ifndef SimDatabase_header_h
#define SimDatabase_header_h

// SimQL stuff
#include <SimLinkedList.h>
#include <SimDataNode.h>
#include <SimQL_Types.h>

// STL stuff
#include <string>
#include <memory>
#include <function>
#include <queue>
#include <mutex>
#include <thread>
#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <vector>

namespace SimpleSql {
    
    class SimQuery;

    class SimDatabase {
    private:

        // handles
        std::unique_ptr<void> h_env;
        std::unique_ptr<void> h_dbc;

        // statement members
        uint8_t m_stmt_count;
        uint8_t m_skipped;
        SimpleSql::SimDataNode<std::unique_ptr<void>>* m_current_stmt;
        SimpleSql::SimLinkedList<std::unique_ptr<void>> m_stmt_list;

        // concurrency members
        constexpr uint8_t cm_governing_max_concurrency = 8;

        // async members
        std::shared_ptr<std::function<void(const SimpleSql::SimQuery &_query)>> mp_listener;
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
        SimDatabase(const uint8_t &stmt_count) : m_stmt_count(stmt_count) { m_statements.reserve(m_stmt_count); m_current_statement = 0; }
        ~SimDatabase() { stop(); }

        bool start(const std::string &driver, const std::string &server, const std::string &database, const int &port, const bool &readonly, const bool &trusted, const bool &encrypt, std::string &error);
        bool start(const std::string &driver, const std::string &server, const std::string &database, const int &port, const bool &readonly, const bool &trusted, const bool &encrypt, const std::string &username, const std::string &password, std::string &error);
        bool run_sync(SimpleSql::SimQuery &_query);
        void run_async(std::shared_ptr<SimpleSql::SimQuery> _query);
        void run_parallel(const uint8_t &max_concurrency, std::vector<SimpleSql::SimQuery> &queries);
        void stop();
        void listen(std::shared_ptr<std::function<void(const SimpleSql::simresult &result)>> p_listener);

    };
}

#endif