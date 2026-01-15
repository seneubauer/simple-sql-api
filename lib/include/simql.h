#ifndef simql_header_h
#define simql_header_h

#include <linkedlist.h>
#include <datanode.h>
#include <simql_types.h>
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
    
    class query;

    class simql {
    private:

        // handles
        void* h_env;
        void* h_dbc;

        // statement members
        uint8_t m_stmtcount;
        uint8_t m_skipped;
        SimpleSql::datanode<void*>* m_currentstmt;
        SimpleSql::linkedlist<void*> m_stmtlist;

        // concurrency members
        constexpr uint8_t cm_maxconcurrency = 8;

        // async members
        std::shared_ptr<std::function<void(const SimpleSql::query &_query)>> mp_listener;
        std::queue<SimpleSql::query> m_queries;
        std::mutex m_mutex;
        std::thread m_thread;
        std::atomic<bool> m_inprogress;
        std::condition_variable m_cvar;

        // async functions
        void process();

        // utility functions
        bool connect(std::string &conn_str, std::string &error);
        void disconnect();

    public:
        simql(const uint8_t &stmtcount) : m_stmtcount(stmtcount) { m_statements.reserve(m_stmtcount); m_currentstatement = 0; }
        ~simql() { stop(); }

        bool start(const std::string &driver, const std::string &server, const std::string &database, const int &port, const bool &readonly, const bool &trusted, const bool &encrypt, std::string &error);
        bool start(const std::string &driver, const std::string &server, const std::string &database, const int &port, const bool &readonly, const bool &trusted, const bool &encrypt, const std::string &username, const std::string &password, std::string &error);
        bool run_sync(SimpleSql::query &_query);
        void run_async(std::shared_ptr<SimpleSql::query> _query);
        void run_parallel(const uint8_t &maxconcurrency, std::vector<SimpleSql::query> &queries);
        void stop();
        void listen(std::shared_ptr<std::function<void(const SimpleSql::simresult &result)>> p_listener);

    };
}

#endif