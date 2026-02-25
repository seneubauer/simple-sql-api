// SimQL stuff
#include "statement_pool.hpp"
#include "database_connection.hpp"
#include "statement.hpp"

// STL stuff
#include <cstdint>
#include <memory>
#include <deque>
#include <mutex>
#include <condition_variable>
#include <chrono>

// OS stuff
#include "os_inclusions.hpp"

// ODBC stuff
#include <sqltypes.h>
#include <sqlext.h>
#include <sql.h>

namespace simql {

    extern void* get_dbc_handle(database_connection&) noexcept;

    struct pooled_statement {
        SQLHSTMT h_stmt = SQL_NULL_HSTMT;
        std::chrono::steady_clock::time_point last_used{};
    };

    struct statement_pool::pool {
        database_connection& connection;
        SQLHDBC h_dbc = SQL_NULL_HDBC;
        std::deque<pooled_statement> statements;
        statement_pool::alloc_options pool_opts;
        statement::alloc_options stmt_opts;
        std::mutex mtx;
        std::condition_variable cvar;
        std::uint8_t total_allocated = 0;

        pool(database_connection& conn, const statement_pool::alloc_options& pool_options, const statement::alloc_options& stmt_options) : connection(conn), pool_opts(pool_options) {
            h_dbc = static_cast<SQLHDBC>(get_dbc_handle(conn));

            stmt_opts = stmt_options;
            pool_opts = pool_options;
            if (pool_opts.min_size < simql_constants::limits::min_statement_handle_pool_size)
                pool_opts.min_size = simql_constants::limits::min_statement_handle_pool_size;

            if (pool_opts.max_size > simql_constants::limits::max_statement_handle_pool_size)
                pool_opts.max_size = simql_constants::limits::max_statement_handle_pool_size;

            if (pool_options.min_size > 0)
                build_pool(pool_options.min_size);
        }

        ~pool() {
            for (pooled_statement& ps : statements) {
                if (ps.h_stmt)
                    SQLFreeHandle(SQL_HANDLE_STMT, ps.h_stmt);
            }
            statements.clear();
        }

        void build_pool(const std::uint8_t& target) {
            while (total_allocated < target) {
                SQLHSTMT h = SQL_NULL_HSTMT;
                if (SQL_SUCCEEDED(SQLAllocHandle(SQL_HANDLE_STMT, h_dbc, &h))) {
                    configure_stmt(h);
                    total_allocated++;
                    statements.push_back(pooled_statement {
                        h,
                        std::chrono::steady_clock::now()
                    });
                }
            }
        }

        void configure_stmt(SQLHSTMT h) {

            // set query timeout
            SQLPOINTER p_query_timeout = reinterpret_cast<SQLPOINTER>(static_cast<SQLULEN>(stmt_opts.query_timeout));
            SQLSetStmtAttrW(h, SQL_ATTR_QUERY_TIMEOUT, p_query_timeout, SQL_IS_INTEGER);

            // set max rows
            SQLPOINTER p_max_rows = reinterpret_cast<SQLPOINTER>(static_cast<SQLULEN>(stmt_opts.max_rows));
            SQLSetStmtAttrW(h, SQL_ATTR_MAX_ROWS, p_max_rows, SQL_IS_INTEGER);

            // set rowset size
            SQLPOINTER p_rowset_size = reinterpret_cast<SQLPOINTER>(static_cast<SQLUINTEGER>(stmt_opts.rowset_size));
            SQLSetStmtAttrW(h, SQL_ATTR_ROW_ARRAY_SIZE, p_rowset_size, SQL_IS_INTEGER);

            // set cursor scrollability
            SQLPOINTER p_is_scrollable = stmt_opts.is_scrollable ? reinterpret_cast<SQLPOINTER>(SQL_SCROLLABLE) : reinterpret_cast<SQLPOINTER>(SQL_NONSCROLLABLE);
            SQLSetStmtAttrW(h, SQL_ATTR_CURSOR_SCROLLABLE, p_is_scrollable, SQL_IS_INTEGER);

            // set cursor sensitivity
            SQLPOINTER p_cursor_sensitivity;
            switch (stmt_opts.sensitivity) {
            case statement::cursor_sensitivity::unspecified:
                p_cursor_sensitivity = reinterpret_cast<SQLPOINTER>(SQL_UNSPECIFIED);
                break;
            case statement::cursor_sensitivity::sensitive:
                p_cursor_sensitivity = reinterpret_cast<SQLPOINTER>(SQL_SENSITIVE);
                break;
            case statement::cursor_sensitivity::insensitive:
                p_cursor_sensitivity = reinterpret_cast<SQLPOINTER>(SQL_INSENSITIVE);
                break;
            }
            SQLSetStmtAttrW(h, SQL_ATTR_CURSOR_SENSITIVITY, p_cursor_sensitivity, SQL_IS_INTEGER);
        }

        SQLHSTMT acquire() {
            std::unique_lock<std::mutex> lock(mtx);

            auto try_pop = [&]() -> SQLHSTMT {
                if (!statements.empty()) {
                    SQLHSTMT h = statements.back().h_stmt;
                    statements.pop_back();
                    return h;
                }
                return SQL_NULL_HSTMT;
            };

            if (auto h = try_pop())
                return h;

            if (pool_opts.max_size == 0 || total_allocated < pool_opts.max_size) {
                lock.unlock();
                SQLHSTMT h = SQL_NULL_HSTMT;
                if (SQL_SUCCEEDED(SQLAllocHandle(SQL_HANDLE_STMT, h_dbc, &h))) {
                    configure_stmt(h);
                    lock.lock();
                    total_allocated++;
                    return h;
                }
                lock.lock();
            }

            if (pool_opts.acquire_timeout.count() > 0) {
                auto deadline = std::chrono::steady_clock::now() + pool_opts.acquire_timeout;
                while (true) {
                    if (auto h = try_pop())
                        return h;

                    if (cvar.wait_until(lock, deadline) == std::cv_status::timeout)
                        break;
                }
            }

            return try_pop();
        }

        void release(SQLHSTMT h) {
            if (!h)
                return;

            SQLFreeStmt(h, SQL_CLOSE);
            SQLFreeStmt(h, SQL_RESET_PARAMS);
            SQLFreeStmt(h, SQL_UNBIND);

            std::unique_lock<std::mutex> lock(mtx);
            if (pool_opts.idle_ttl.count() > 0 && total_allocated > pool_opts.min_size) {
                auto now = std::chrono::steady_clock::now();

                while (!statements.empty() && total_allocated > pool_opts.min_size) {
                    const auto& oldest = statements.front();
                    if (now - oldest.last_used >= pool_opts.idle_ttl) {
                        SQLFreeHandle(SQL_HANDLE_STMT, oldest.h_stmt);
                        statements.pop_front();
                        total_allocated--;
                    } else {
                        break;
                    }
                }
            }

            statements.push_back(pooled_statement {
                h,
                std::chrono::steady_clock::now()
            });

            if (pool_opts.max_size > 0) {
                while (total_allocated > pool_opts.max_size && !statements.empty()) {
                    SQLFreeHandle(SQL_HANDLE_STMT, statements.front().h_stmt);
                    statements.pop_front();
                    total_allocated--;
                }
            }

            lock.unlock();
            cvar.notify_one();
        }

    };

    statement_pool::statement_pool(database_connection& conn, const statement_pool::alloc_options& pool_options, const statement::alloc_options& stmt_options) : m_pool(std::make_unique<pool>(conn, pool_options, stmt_options)), m_conn(conn) {}
    statement_pool::~statement_pool() = default;

    void statement_pool::build_pool() {
        std::lock_guard<std::mutex> lock(m_pool.get()->mtx);
        m_pool.get()->build_pool(m_pool.get()->pool_opts.min_size);
    }

    statement statement_pool::acquire() {
        SQLHSTMT h_stmt = m_pool.get()->acquire();
        return statement(std::move(h_stmt), m_conn, m_pool.get());
    }

    void statement_pool::release(statement&& stmt) {
        SQLHSTMT h_stmt = stmt.detach_handle();
        m_pool.get()->release(h_stmt);
    }

}
