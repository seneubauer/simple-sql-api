#ifndef SimRunner_header_h
#define SimRunner_header_h

// SimQL stuff
#include <SimDatabase.hpp>
#include <SimQuery.hpp>

// STL stuff
#include <cstdint>
#include <vector>
#include <utility>
#include <memory>
#include <functional>
#include <queue>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <thread>

namespace SimpleSql {
    class SimRunner {
    private:

        // db
        std::shared_ptr<SimpleSql::SimDatabase> mp_db;

        // listeners
        std::shared_ptr<std::function<void(SimpleSql::SimQuery&&)>> mp_query_listener;

        // async members
        std::queue<SimpleSql::SimQuery> m_queries;
        std::mutex m_mutex;
        std::atomic<bool> m_in_progress;
        std::condition_variable m_cvar;
        std::thread m_thread;

        // functions
        void run(SimpleSql::SimQuery&& query);
        void process();

    public:
        SimRunner(std::shared_ptr<SimpleSql::SimDatabase> db) : mp_db(std::move(db)) {}
        ~SimRunner() { stop(); }

        void start();
        void stop();
        void listen(std::shared_ptr<std::function<void(SimpleSql::SimQuery&&)>> p_listener);
        void run_async(SimpleSql::SimQuery&& query);
        void run_parallel(std::uint8_t& thread_count, std::vector<SimpleSql::SimQuery>& queries);
    };
}

#endif