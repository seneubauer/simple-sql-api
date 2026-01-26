#ifndef SimDatabase_header_h
#define SimDatabase_header_h

// SimQL stuff
#include <SimQuery.hpp>
#include <SimQL_Types.hpp>
#include <SimQL_Utility.hpp>
#include <SimQL_Constants.hpp>

// STL stuff
#include <string>
#include <memory>
#include <functional>
#include <utility>
#include <mutex>
#include <cstdint>
#include <vector>

namespace SimpleSql {
    class SimDatabase {
    private:

        // handles
        SimpleSqlTypes::ENV_HANDLE h_env;
        SimpleSqlTypes::DBC_HANDLE h_dbc;

        // statement members
        std::uint8_t m_stmt_index;
        std::uint8_t m_stmt_count;
        std::uint8_t m_skipped;
        std::vector<SimpleSqlTypes::STMT_HANDLE> m_stmt_vector;

        // listeners
        std::shared_ptr<std::function<void(std::uint8_t&&)>> mp_stmt_pool_listener;

        // concurrency members
        std::mutex m_mutex;

        // internal statement function
        bool remove_stmt_handle();

    public:
        SimDatabase(const std::uint8_t& stmt_count) : m_stmt_count(stmt_count > SimpleSqlConstants::max_statement_handle_pool_size ? SimpleSqlConstants::max_statement_handle_pool_size : stmt_count), m_skipped(0) {}
        ~SimDatabase() { disconnect(); }

        // dbc attibutes (before opening a connection)
        bool set_connection_pooling(const SimpleSqlTypes::ConnectionPoolingType& value);
        bool set_access_mode(const SimpleSqlTypes::AccessModeType& value);
        bool set_driver_async(const SimpleSqlTypes::AsyncModeType& value);
        bool set_autocommit(const SimpleSqlTypes::AutocommitType& value);
        bool set_login_timeout(const std::uint32_t& value);
        bool set_connection_timeout(const std::uint32_t& value);

        // attribute getters
        bool get_connection_pooling(SimpleSqlTypes::ConnectionPoolingType& value);
        bool get_access_mode(SimpleSqlTypes::AccessModeType& value);
        bool get_driver_async(SimpleSqlTypes::AsyncModeType& value);
        bool get_autocommit(SimpleSqlTypes::AutocommitType& value);
        bool get_login_timeout(std::uint32_t& value);
        bool get_connection_timeout(std::uint32_t& value);
        bool get_connection_state(bool& connected);

        // transactions
        bool open_transaction();
        bool rollback_transaction();
        bool commit_transaction();

        // lifecycle handling
        std::uint8_t connect(std::string& conn_str);
        void disconnect();

        // statement handling
        bool assign_stmt_handle(SimpleSql::SimQuery& query);
        void reclaim_stmt_handle(SimpleSql::SimQuery& query);

        // listener handling
        void listen(std::shared_ptr<std::function<void(std::uint8_t&&)>> p_listener);
    };
}

#endif