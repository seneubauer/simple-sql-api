// SimQL stuff
#include "database_connection.hpp"
#include "environment.hpp"
#include "simql_strings.hpp"
#include "diagnostic_set.hpp"

// STL stuff
#include <cstdint>
#include <memory>
#include <vector>
#include <string>
#include <string_view>

// OS stuff
#include "os_inclusions.hpp"

// ODBC stuff
#include <sqltypes.h>
#include <sqlext.h>
#include <sql.h>

namespace simql {

    extern void* get_env_handle(environment& env) noexcept;

    struct database_connection::handle {

        // handles
        SQLHENV h_env{SQL_NULL_HENV};
        SQLHDBC h_dbc{SQL_NULL_HDBC};

        // diagnostics
        std::string last_error{};
        diagnostic_set diag{};

        // trackers
        bool is_valid{true};

        explicit handle(environment& env, database_connection::alloc_options& options) {

            // allocate the handle
            h_env = static_cast<SQLHENV>(get_env_handle(env));
            switch (SQLAllocHandle(SQL_HANDLE_DBC, h_env, &h_dbc)) {
            case SQL_SUCCESS:
                break;
            case SQL_SUCCESS_WITH_INFO:
                diag.update(h_env, diagnostic_set::handle_type::env, std::string{"SQLAllocHandle(SQL_HANDLE_DBC) -> SUCCESS_WITH_INFO"});
                break;
            case SQL_INVALID_HANDLE:
                last_error = std::string{"could not allocate the database connection handle: invalid handle"};
                diag.update(h_env, diagnostic_set::handle_type::env, std::string{"SQLAllocHandle(SQL_HANDLE_DBC) -> INVALID_HANDLE"});
                is_valid = false;
                return;
            case SQL_ERROR:
                last_error = std::string{"could not allocate the database connection handle: generic error"};
                diag.update(h_env, diagnostic_set::handle_type::env, std::string{"SQLAllocHandle(SQL_HANDLE_DBC) -> ERROR"});
                is_valid = false;
                return;
            }

            // set access mode
            is_valid = set_access_mode(options.read_only);
            if (!is_valid)
                return;

            // set connection timeout
            is_valid = set_connection_timeout(options.connection_timeout);
            if (!is_valid)
                return;

            // set login timeout
            is_valid = set_login_timeout(options.login_timeout);
            if (!is_valid)
                return;

            // set packet size
            is_valid = set_packet_size(options.packet_size);
            if (!is_valid)
                return;

            // set autocommit
            is_valid = set_autocommit(options.enable_autocommit);
            if (!is_valid)
                return;

            // set tracefile
            is_valid = set_tracefile(options.tracefile);
            if (!is_valid)
                return;

            // set tracing
            is_valid = set_tracing(options.enable_tracing);
            if (!is_valid)
                return;

        }

        ~handle() {
            if (is_connected())
                SQLDisconnect(h_dbc);

            if (h_dbc)
                SQLFreeHandle(SQL_HANDLE_DBC, h_dbc);
        }

        bool set_access_mode(bool read_only) {

            SQLPOINTER p_access_mode = read_only ? reinterpret_cast<SQLPOINTER>(SQL_MODE_READ_ONLY) : reinterpret_cast<SQLPOINTER>(SQL_MODE_READ_WRITE);
            switch (SQLSetConnectAttrW(h_dbc, SQL_ATTR_ACCESS_MODE, p_access_mode, SQL_IS_INTEGER)) {
            case SQL_SUCCESS:
                return true;
            case SQL_SUCCESS_WITH_INFO:
                diag.update(h_dbc, diagnostic_set::handle_type::dbc, std::string{"SQLSetConnectAttr(SQL_ATTR_ACCESS_MODE) -> SUCCESS_WITH_INTO"});
                return true;
            case SQL_INVALID_HANDLE:
                last_error = std::string{"could not set the access mode: invalid handle"};
                diag.update(h_env, diagnostic_set::handle_type::env, std::string{"SQLSetConnectAttr(SQL_ATTR_ACCESS_MODE) -> INVALID_HANDLE"});
                return false;
            default:
                last_error = std::string{"could not set the access mode: generic error"};
                diag.update(h_dbc, diagnostic_set::handle_type::dbc, std::string{"SQLSetConnectAttr(SQL_ATTR_ACCESS_MODE) -> ERROR"});
                return false;
            }
        }

        bool set_connection_timeout(std::uint32_t timeout) {

            SQLPOINTER p_connection_timeout = reinterpret_cast<SQLPOINTER>(static_cast<SQLUINTEGER>(timeout));
            switch (SQLSetConnectAttrW(h_dbc, SQL_ATTR_CONNECTION_TIMEOUT, p_connection_timeout, SQL_IS_INTEGER)) {
            case SQL_SUCCESS:
                return true;
            case SQL_SUCCESS_WITH_INFO:
                diag.update(h_dbc, diagnostic_set::handle_type::dbc, std::string{"SQLSetConnectAttr(SQL_ATTR_CONNECTION_TIMEOUT) -> SUCCESS_WITH_INFO"});
                return true;
            case SQL_INVALID_HANDLE:
                last_error = std::string{"could not set the connection timeout: invalid handle"};
                diag.update(h_env, diagnostic_set::handle_type::env, std::string{"SQLSetConnectAttr(SQL_ATTR_CONNECTION_TIMEOUT) -> INVALID_HANDLE"});
                return false;
            default:
                last_error = std::string{"could not set the connection timeout: generic error"};
                diag.update(h_dbc, diagnostic_set::handle_type::dbc, std::string{"SQLSetConnectAttr(SQL_ATTR_CONNECTION_TIMEOUT) -> ERROR"});
                return false;
            }
        }

        bool set_login_timeout(std::uint32_t timeout) {

            SQLPOINTER p_login_timeout = reinterpret_cast<SQLPOINTER>(static_cast<SQLUINTEGER>(timeout));
            switch (SQLSetConnectAttrW(h_dbc, SQL_ATTR_LOGIN_TIMEOUT, p_login_timeout, SQL_IS_INTEGER)) {
            case SQL_SUCCESS:
                return true;
            case SQL_SUCCESS_WITH_INFO:
                diag.update(h_dbc, diagnostic_set::handle_type::dbc, std::string{"SQLSetConnectAttr(SQL_ATTR_LOGIN_TIMEOUT) -> SUCCESS_WITH_INFO"});
                return true;
            case SQL_INVALID_HANDLE:
                last_error = std::string{"could not set the login timeout: invalid handle"};
                diag.update(h_env, diagnostic_set::handle_type::env, std::string{"SQLSetConnectAttr(SQL_ATTR_LOGIN_TIMEOUT) -> INVALID_HANDLE"});
                return false;
            default:
                last_error = std::string{"could not set the login timeout: generic error"};
                diag.update(h_dbc, diagnostic_set::handle_type::dbc, std::string{"SQLSetConnectAttr(SQL_ATTR_LOGIN_TIMEOUT) -> ERROR"});
                return false;
            }
        }

        bool set_packet_size(std::uint32_t packet_size) {

            SQLPOINTER p_packet_size = reinterpret_cast<SQLPOINTER>(static_cast<SQLUINTEGER>(packet_size));
            switch (SQLSetConnectAttrW(h_dbc, SQL_ATTR_PACKET_SIZE, p_packet_size, SQL_IS_INTEGER)) {
            case SQL_SUCCESS:
                return true;
            case SQL_SUCCESS_WITH_INFO:
                diag.update(h_dbc, diagnostic_set::handle_type::dbc, std::string{"SQLSetConnectAttr(SQL_ATTR_PACKET_SIZE) -> SUCCESS_WITH_INFO"});
                return true;
            case SQL_INVALID_HANDLE:
                last_error = std::string{"could not set the packet size: invalid handle"};
                diag.update(h_env, diagnostic_set::handle_type::env, std::string{"SQLSetConnectAttr(SQL_ATTR_PACKET_SIZE) -> INVALID_HANDLE"});
                return false;
            default:
                last_error = std::string{"could not set the packet size: generic error"};
                diag.update(h_dbc, diagnostic_set::handle_type::dbc, std::string{"SQLSetConnectAttr(SQL_ATTR_PACKET_SIZE) -> ERROR"});
                return false;
            }
        }

        bool set_autocommit(bool autocommit) {

            SQLPOINTER p_autocommit = autocommit ? reinterpret_cast<SQLPOINTER>(SQL_AUTOCOMMIT_ON) : reinterpret_cast<SQLPOINTER>(SQL_AUTOCOMMIT_OFF);
            switch (SQLSetConnectAttrW(h_dbc, SQL_ATTR_AUTOCOMMIT, p_autocommit, SQL_IS_INTEGER)) {
            case SQL_SUCCESS:
                return true;
            case SQL_SUCCESS_WITH_INFO:
                diag.update(h_dbc, diagnostic_set::handle_type::dbc, std::string{"SQLSetConnectAttr(SQL_ATTR_AUTOCOMMIT) -> SUCCESS_WITH_INTO"});
                return true;
            case SQL_INVALID_HANDLE:
                last_error = std::string{"could not set the autocommit mode: invalid handle"};
                diag.update(h_env, diagnostic_set::handle_type::env, std::string{"SQLSetConnectAttr(SQL_ATTR_AUTOCOMMIT) -> INVALID_HANDLE"});
                return false;
            default:
                last_error = std::string{"could not set the autocommit mode: generic error"};
                diag.update(h_dbc, diagnostic_set::handle_type::dbc, std::string{"SQLSetConnectAttr(SQL_ATTR_AUTOCOMMIT) -> ERROR"});
                return false;
            }
        }

        bool set_tracing(bool tracing) {

            SQLPOINTER p_tracing = tracing ? reinterpret_cast<SQLPOINTER>(SQL_OPT_TRACE_ON) : reinterpret_cast<SQLPOINTER>(SQL_OPT_TRACE_OFF);
            switch (SQLSetConnectAttrW(h_dbc, SQL_ATTR_TRACE, p_tracing, SQL_IS_INTEGER)) {
            case SQL_SUCCESS:
                return true;
            case SQL_SUCCESS_WITH_INFO:
                diag.update(h_dbc, diagnostic_set::handle_type::dbc, std::string{"SQLSetConnectAttr(SQL_ATTR_TRACE) -> SUCCESS_WITH_INTO"});
                return true;
            case SQL_INVALID_HANDLE:
                last_error = std::string{"could not set the tracing mode: invalid handle"};
                diag.update(h_env, diagnostic_set::handle_type::env, std::string{"SQLSetConnectAttr(SQL_ATTR_TRACE) -> INVALID_HANDLE"});
                return false;
            default:
                last_error = std::string{"could not set the tracing mode: generic error"};
                diag.update(h_dbc, diagnostic_set::handle_type::dbc, std::string{"SQLSetConnectAttr(SQL_ATTR_TRACE) -> ERROR"});
                return false;
            }
        }

        bool set_tracefile(std::string tracefile) {
            if (tracefile.empty())
                return true;

            auto str = simql_strings::to_odbc_w(std::string_view(reinterpret_cast<char*>(tracefile.data()), tracefile.size()));
            switch (SQLSetConnectAttrW(h_dbc, SQL_ATTR_TRACEFILE, (SQLPOINTER)str.c_str(), SQL_NTS)) {
            case SQL_SUCCESS:
                return true;
            case SQL_SUCCESS_WITH_INFO:
                diag.update(h_dbc, diagnostic_set::handle_type::dbc, std::string{"SQLSetConnectAttr(SQL_ATTR_TRACEFILE) -> SUCCESS_WITH_INFO"});
                return true;
            case SQL_INVALID_HANDLE:
                last_error = std::string{"could not set the tracefile: invalid handle"};
                diag.update(h_env, diagnostic_set::handle_type::env, std::string{"SQLSetConnectAttr(SQL_ATTR_TRACEFILE) -> INVALID_HANDLE"});
                return false;
            default:
                last_error = std::string{"could not set the tracefile: generic error"};
                diag.update(h_dbc, diagnostic_set::handle_type::dbc, std::string{"SQLSetConnectAttr(SQL_ATTR_TRACEFILE) -> ERROR"});
                return false;
            }
        }

        bool connect(std::string connection_string) {
            auto connection_string_in = simql_strings::to_odbc_w(std::string_view(reinterpret_cast<const char*>(connection_string.data()), connection_string.size()));
            std::basic_string<SQLWCHAR> connection_string_out;
            connection_string_out.resize(1024);
            SQLSMALLINT connection_string_out_length{0};
            SQLSMALLINT buffer_length = connection_string_out.size();
            switch (SQLDriverConnectW(h_dbc, nullptr, connection_string_in.data(), SQL_NTS, connection_string_out.data(), buffer_length, &connection_string_out_length, SQL_DRIVER_NOPROMPT)) {
            case SQL_SUCCESS:
                return true;
            case SQL_SUCCESS_WITH_INFO:
                diag.update(h_dbc, diagnostic_set::handle_type::dbc, std::string{"SQLDriverConnect() -> SUCCESS_WITH_INFO"});
                return true;
            case SQL_INVALID_HANDLE:
                last_error = std::string{"could not open the connection: invalid handle"};
                diag.update(h_env, diagnostic_set::handle_type::env, std::string{"SQLDriverConnect() -> INVALID_HANDLE"});
                return false;
            default:
                last_error = std::string{"could not open the connection: generic error"};
                diag.update(h_dbc, diagnostic_set::handle_type::dbc, std::string{"SQLDriverConnect() -> ERROR"});
                return false;
            }
        }

        bool is_connected() {
            SQLUINTEGER output;
            switch (SQLGetConnectAttrW(h_dbc, SQL_ATTR_CONNECTION_DEAD, &output, SQL_IS_INTEGER, nullptr)) {
            case SQL_SUCCESS:
                return output == SQL_CD_FALSE;
            case SQL_SUCCESS_WITH_INFO:
                return output == SQL_CD_FALSE;
            case SQL_INVALID_HANDLE:
                last_error = std::string{"could not determine the connection status: invalid handle"};
                diag.update(h_env, diagnostic_set::handle_type::env, std::string{"SQLGetConnectAttr(SQL_ATTR_CONNECTION_DEAD) -> INVALID HANDLE"});
                return false;
            default:
                last_error = std::string{"could not determine the connection status: generic error"};
                diag.update(h_dbc, diagnostic_set::handle_type::dbc, std::string{"SQLGetConnectAttr(SQL_ATTR_CONNECTION_DEAD) -> ERROR"});
                return false;
            }
        }

        void disconnect() {
            if (is_connected())
                SQLDisconnect(h_dbc);
        }
    };

    database_connection::database_connection(environment& env, database_connection::alloc_options& options) : p_handle(std::make_unique<handle>(env, options)) {}
    database_connection::~database_connection() = default;
    database_connection::database_connection(database_connection&&) noexcept = default;
    database_connection& database_connection::operator=(database_connection&&) noexcept = default;

    bool database_connection::connect(std::string connection_string) {
        return p_handle ? p_handle->connect(connection_string) : false;
    }

    bool database_connection::is_connected() {
        return p_handle ? p_handle->is_connected() : false;
    }

    void database_connection::disconnect() {
        if (p_handle)
            p_handle->disconnect();
    }

    bool database_connection::is_valid() {
        return !p_handle ? false : p_handle->is_valid;
    }

    std::string_view database_connection::last_error() {
        return !p_handle ? std::string_view{} : p_handle->last_error;
    }

    diagnostic_set* database_connection::diagnostics() {
        return !p_handle ? nullptr : &p_handle->diag;
    }

    void* get_dbc_handle(database_connection& dbc) noexcept {
        return dbc.p_handle ? reinterpret_cast<void*>(dbc.p_handle->h_dbc) : nullptr;
    }

}