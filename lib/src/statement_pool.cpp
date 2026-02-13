// SimQL stuff
#include <statement_pool.hpp>
#include <database_connection.hpp>
#include <statement.hpp>

// STL stuff
#include <cstdint>
#include <memory>
#include <deque>
#include <mutex>
#include <condition_variable>
#include <chrono>

// Windows stuff
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

            // set attribute (cursor type)
            SQLPOINTER p_cursor_type;
            switch (stmt_opts.cursor) {
            case statement::cursor_type::forward_only:
                p_cursor_type = reinterpret_cast<SQLPOINTER>(SQL_CURSOR_FORWARD_ONLY);
                break;
            case statement::cursor_type::static:
                p_cursor_type = reinterpret_cast<SQLPOINTER>(SQL_CURSOR_STATIC);
                break;
            case statement::cursor_type::dyanmic_cursor:
                p_cursor_type = reinterpret_cast<SQLPOINTER>(SQL_CURSOR_DYNAMIC);
                break;
            case statement::cursor_type::keyset_driven:
                p_cursor_type = reinterpret_cast<SQLPOINTER>(SQL_CURSOR_KEYSET_DRIVEN);
                break;
            }
            SQLSetStmtAttrW(h, SQL_ATTR_CURSOR_TYPE, p_cursor_type, 0);

            // set attribute (query timeout)
            SQLPOINTER p_query_timeout = reinterpret_cast<SQLPOINTER>(static_cast<SQLULEN>(stmt_opts.query_timeout));
            SQLSetStmtAttrW(h, SQL_ATTR_QUERY_TIMEOUT, p_query_timeout, 0);

            // set attribute (max rows)
            SQLPOINTER p_max_rows = reinterpret_cast<SQLPOINTER>(static_cast<SQLULEN>(stmt_opts.max_rows));
            SQLSetStmtAttrW(h, SQL_ATTR_MAX_ROWS, p_max_rows, 0);
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
