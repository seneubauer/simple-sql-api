#ifndef SimDatabase_header_h
#define SimDatabase_header_h

// SimQL stuff
#include <SimQL_Constants.hpp>
#include <SimQL_Types.hpp>

// STL stuff
#include <string>
#include <memory>
#include <functional>
#include <mutex>
#include <cstdint>
#include <vector>

namespace SimpleSql {
    class SimDatabase {
    private:

        // return codes
        static constexpr std::uint8_t SUCCESS                   = 0;
        static constexpr std::uint8_t STMT_HANDLE_ASSIGNMENT    = 1;
        static constexpr std::uint8_t ENV_HANDLE_ALLOC          = 2;
        static constexpr std::uint8_t ODBC_VERSION3             = 3;
        static constexpr std::uint8_t DBC_HANDLE_ALLOC          = 4;
        static constexpr std::uint8_t CONNECTION                = 5;
        const std::unordered_map<std::uint8_t, std::string_view> m_rc_def {
            {SUCCESS,                   std::string_view("process was successful")},
            {STMT_HANDLE_ASSIGNMENT,    std::string_view("could not assign the statement handle")},
            {ENV_HANDLE_ALLOC,          std::string_view("could not allocate the environment handle")},
            {ODBC_VERSION3,             std::string_view("could not set ODBC to version 3")},
            {DBC_HANDLE_ALLOC,          std::string_view("could not allocate the connection handle")},
            {CONNECTION,                std::string_view("could not open a connection to the database")}
        };

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
        SimDatabase(const std::uint8_t& stmt_count) : m_stmt_count(stmt_count > SimpleSqlConstants::Limits::max_statement_handle_pool_size ? SimpleSqlConstants::Limits::max_statement_handle_pool_size : stmt_count), m_skipped(0) {}
        ~SimDatabase() { disconnect(); }

        std::string_view return_code_definition(const std::uint8_t& return_code) {
            auto it = m_rc_def.find(return_code);
            if (it != m_rc_def.end())
                return it->second;
            return std::string_view("fallback return code for undefined unsigned 8bit integers");
        }

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
        bool extract_stmt_handle(SimpleSqlTypes::STMT_HANDLE& handle);
        void reclaim_stmt_handle(SimpleSqlTypes::STMT_HANDLE&& handle);

        // listener handling
        void listen(std::shared_ptr<std::function<void(std::uint8_t&&)>> p_listener);
    };
}

#endif
