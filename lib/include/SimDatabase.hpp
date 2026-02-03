#ifndef SimDatabase_header_h
#define SimDatabase_header_h

// SimQL stuff
#include <SimQL_Constants.hpp>
#include <SimQL_Types.hpp>
#include <SimDiagnosticSet.hpp>

// STL stuff
#include <string>
#include <memory>
#include <functional>
#include <mutex>
#include <cstdint>
#include <vector>

namespace SimpleSql {
    class SimDatabase {
    public:

        /* constructor/destructor */
        SimDatabase(const std::uint8_t& stmt_count) : m_stmt_count(stmt_count > SimpleSqlConstants::Limits::max_statement_handle_pool_size ? SimpleSqlConstants::Limits::max_statement_handle_pool_size : stmt_count), m_skipped(0), p_diagnostics(std::make_unique<SimDiagnosticSet>()) {}
        ~SimDatabase() { disconnect(); }

        /* functions */

        // set properties (prior to connection)
        bool set_connection_pooling(const SimpleSqlTypes::ConnectionPoolingType& value);
        bool set_access_mode(const SimpleSqlTypes::AccessModeType& value);
        bool set_driver_async(const SimpleSqlTypes::AsyncModeType& value);
        bool set_autocommit(const SimpleSqlTypes::AutocommitType& value);
        bool set_login_timeout(const std::uint32_t& value);
        bool set_connection_timeout(const std::uint32_t& value);

        // get properties
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

        // connection control
        std::uint8_t connect(std::string& conn_str);
        void disconnect();

        // statement handle accessor
        SimpleSqlTypes::STMT_HANDLE* statement_handle();

        // diagnostics accessor
        SimDiagnosticSet* diagnostics();

        // set listener
        void listen(std::shared_ptr<std::function<void(std::uint8_t&&)>> p_listener);

        // what is the rc definition
        std::string_view return_code_def(const std::uint8_t& return_code);

    private:

        /* internal functions */
        bool remove_stmt_handle();

        /* members */
        SimpleSqlTypes::ENV_HANDLE h_env;
        SimpleSqlTypes::DBC_HANDLE h_dbc;
        std::uint8_t m_stmt_index;
        std::uint8_t m_stmt_count;
        std::uint8_t m_skipped;
        std::vector<SimpleSqlTypes::STMT_HANDLE> m_stmt_vector;
        std::shared_ptr<std::function<void(std::uint8_t&&)>> mp_stmt_pool_listener;
        std::mutex m_mutex;
        std::unique_ptr<SimDiagnosticSet> p_diagnostics;

        // return codes
        static constexpr std::uint8_t SUCCESS                   = 0;
        static constexpr std::uint8_t STMT_HANDLE_ASSIGNMENT    = 1;
        static constexpr std::uint8_t _RC_ENV_HANDLE_ALLOC          = 2;
        static constexpr std::uint8_t _RC_ODBC_VERSION3             = 3;
        static constexpr std::uint8_t _RC_DBC_HANDLE_ALLOC          = 4;
        static constexpr std::uint8_t _RC_CONNECTION                = 5;
        const std::unordered_map<std::uint8_t, std::string_view> m_return_codes {
            {SUCCESS,                   std::string_view("process was successful")},
            {STMT_HANDLE_ASSIGNMENT,    std::string_view("could not assign the statement handle")},
            {_RC_ENV_HANDLE_ALLOC,          std::string_view("could not allocate the environment handle")},
            {_RC_ODBC_VERSION3,             std::string_view("could not set ODBC to version 3")},
            {_RC_DBC_HANDLE_ALLOC,          std::string_view("could not allocate the connection handle")},
            {_RC_CONNECTION,                std::string_view("could not open a connection to the database")}
        };
    };
}

#endif
