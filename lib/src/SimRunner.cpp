// SimQL stuff
#include <SimRunner.hpp>
#include <SimDatabase.hpp>
#include <SimQuery.hpp>

// STL stuff
#include <cstdint>
#include <vector>
#include <memory>
#include <utility>
#include <functional>
#include <future>

void SimpleSql::SimRunner::run(SimpleSql::SimQuery&& query) {
    std::lock_guard<std::mutex> lock(m_mutex);

    // assign statement handle to the query
    if (query.claim_handle(mp_db->extract_stmt_handle())) {
        // run the query

        mp_db->reclaim_stmt_handle(query.return_handle());
    }

    if (mp_query_listener)
        (*mp_query_listener)(std::move(query));
}

void SimpleSql::SimRunner::process() {
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
        run(std::move(query));
    }
}

void SimpleSql::SimRunner::start() {
    m_thread = std::thread(&SimpleSql::SimRunner::process, this);
}

void SimpleSql::SimRunner::stop() {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_in_progress = false;
    }
    m_cvar.notify_one();
    if (m_thread.joinable())
        m_thread.join();
}

void SimpleSql::SimRunner::listen(std::shared_ptr<std::function<void(SimpleSql::SimQuery&&)>> p_listener) {
    mp_query_listener = std::move(p_listener);
}

void SimpleSql::SimRunner::run_async(SimpleSql::SimQuery&& query) {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_queries.push(std::move(query));
    }
    m_cvar.notify_one();
}

void SimpleSql::SimRunner::run_parallel(std::uint8_t& thread_count, std::vector<SimpleSql::SimQuery>& queries) {

    auto execute = [&](size_t&& index, SimpleSql::SimQuery&& query, std::promise<std::pair<size_t, SimpleSql::SimQuery>>&& prom) -> void {

        // assign statement handle, goto end_of_lambda on failure
        if (!query.claim_handle(mp_db->extract_stmt_handle()))
            goto end_of_lambda;

        {
            std::lock_guard<std::mutex> lock(m_mutex);
            // run the query
        }

        // return statement handle
        mp_db->reclaim_stmt_handle(query.return_handle());

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