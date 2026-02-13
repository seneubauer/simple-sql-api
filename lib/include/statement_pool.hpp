#ifndef statement_pool_header_h
#define statement_pool_header_h

// SimQL stuff
#include <database_connection.hpp>
#include <statement.hpp>
#include <simql_returncodes.hpp>
#include <simql_constants.hpp>

// STL stuff
#include <cstdint>
#include <memory>
#include <chrono>

namespace simql {
    class statement_pool {
    public:

        /* structs */
        struct alloc_options {
            std::uint8_t min_size = simql_constants::limits::min_statement_handle_pool_size;
            std::uint8_t max_size = simql_constants::limits::max_statement_handle_pool_size;
            std::chrono::milliseconds acquire_timeout{0};
            std::chrono::milliseconds idle_ttl{0};
        };

        /* constructor/destructor */
        explicit statement_pool(database_connection& conn, const alloc_options& pool_options, const statement::alloc_options& stmt_options);
        ~statement_pool();

        /* functions */
        void build_pool();
        statement acquire();
        void release(statement&& stmt);

    private:
        struct pool;
        std::unique_ptr<pool> m_pool;
        database_connection& m_conn;
    };
}

#endif