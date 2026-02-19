// SimQL stuff
#include "database_connection.hpp"
#include "environment.hpp"
#include "simql_strings.hpp"
#include "diagnostic_set.hpp"

// STL stuff
#include <cstdint>
#include <memory>
#include <vector>
#include <string_view>
#include <iostream>

// OS stuff
#include "os_inclusions.hpp"

// ODBC stuff
#include <sqltypes.h>
#include <sqlext.h>
#include <sql.h>

namespace simql {

    extern void* get_env_handle(environment& env) noexcept;

    struct database_connection::handle {
        SQLHDBC h_dbc = SQL_NULL_HDBC;
        SQLHENV h_env = SQL_NULL_HENV;
        bool is_valid{true};
        diagnostic_set diag;

        explicit handle(environment& env, database_connection::alloc_options& options) {

            h_env = static_cast<SQLHENV>(get_env_handle(env));

            // allocate the handle
            switch (SQLAllocHandle(SQL_HANDLE_DBC, h_env, &h_dbc)) {
            case SQL_SUCCESS:
                break;
            case SQL_SUCCESS_WITH_INFO:
                diag.update(h_env, diagnostic_set::handle_type::env);
                break;
            default:
                diag.update(h_env, diagnostic_set::handle_type::env);
                is_valid = false;
                return;
            }

            // set attribute (access mode)
            SQLPOINTER p_access_mode = options.read_only ? reinterpret_cast<SQLPOINTER>(SQL_MODE_READ_ONLY) : reinterpret_cast<SQLPOINTER>(SQL_MODE_READ_WRITE);
            switch (SQLSetConnectAttrW(h_dbc, SQL_ATTR_ACCESS_MODE, p_access_mode, 0)) {
            case SQL_SUCCESS:
                break;
            case SQL_SUCCESS_WITH_INFO:
                diag.update(h_dbc, diagnostic_set::handle_type::dbc);
                break;
            default:
                diag.update(h_dbc, diagnostic_set::handle_type::dbc);
                is_valid = false;
                return;
            }

            // set attribute (connection timeout)
            SQLPOINTER p_connection_timeout = reinterpret_cast<SQLPOINTER>(static_cast<SQLUINTEGER>(options.connection_timeout));
            switch (SQLSetConnectAttrW(h_dbc, SQL_ATTR_CONNECTION_TIMEOUT, p_connection_timeout, 0)) {
            case SQL_SUCCESS:
                break;
            case SQL_SUCCESS_WITH_INFO:
                diag.update(h_dbc, diagnostic_set::handle_type::dbc);
                break;
            default:
                diag.update(h_dbc, diagnostic_set::handle_type::dbc);
                is_valid = false;
                return;
            }

            // set attribute (login timeout)
            SQLPOINTER p_login_timeout = reinterpret_cast<SQLPOINTER>(static_cast<SQLUINTEGER>(options.login_timeout));
            switch (SQLSetConnectAttrW(h_dbc, SQL_ATTR_LOGIN_TIMEOUT, p_login_timeout, 0)) {
            case SQL_SUCCESS:
                break;
            case SQL_SUCCESS_WITH_INFO:
                diag.update(h_dbc, diagnostic_set::handle_type::dbc);
                break;
            default:
                diag.update(h_dbc, diagnostic_set::handle_type::dbc);
                is_valid = false;
                return;
            }

            // set attribute (packet size)
            SQLPOINTER p_packet_size = reinterpret_cast<SQLPOINTER>(static_cast<SQLUINTEGER>(options.packet_size));
            switch (SQLSetConnectAttrW(h_dbc, SQL_ATTR_PACKET_SIZE, p_packet_size, 0)) {
            case SQL_SUCCESS:
                break;
            case SQL_SUCCESS_WITH_INFO:
                diag.update(h_dbc, diagnostic_set::handle_type::dbc);
                break;
            default:
                diag.update(h_dbc, diagnostic_set::handle_type::dbc);
                is_valid = false;
                return;
            }

            // set attribute (async)
            SQLPOINTER p_async = options.enable_async ? reinterpret_cast<SQLPOINTER>(SQL_ASYNC_ENABLE_ON) : reinterpret_cast<SQLPOINTER>(SQL_ASYNC_ENABLE_OFF);
            switch (SQLSetConnectAttrW(h_dbc, SQL_ATTR_ASYNC_ENABLE, p_async, 0)) {
            case SQL_SUCCESS:
                break;
            case SQL_SUCCESS_WITH_INFO:
                diag.update(h_dbc, diagnostic_set::handle_type::dbc);
                break;
            default:
                diag.update(h_dbc, diagnostic_set::handle_type::dbc);
                is_valid = false;
                return;
            }

            // set attribute (autocommit)
            SQLPOINTER p_autocommit = options.enable_autocommit ? reinterpret_cast<SQLPOINTER>(SQL_AUTOCOMMIT_ON) : reinterpret_cast<SQLPOINTER>(SQL_AUTOCOMMIT_OFF);
            switch (SQLSetConnectAttrW(h_dbc, SQL_ATTR_AUTOCOMMIT, p_autocommit, 0)) {
            case SQL_SUCCESS:
                break;
            case SQL_SUCCESS_WITH_INFO:
                diag.update(h_dbc, diagnostic_set::handle_type::dbc);
                break;
            default:
                diag.update(h_dbc, diagnostic_set::handle_type::dbc);
                is_valid = false;
                return;
            }

            // set attribute (tracefile)
            if (options.enable_tracing && !options.tracefile.empty()) {
                auto str = simql_strings::to_odbc_w(std::string_view(reinterpret_cast<char*>(options.tracefile.data()), options.tracefile.size()));
                switch (SQLSetConnectAttrW(h_dbc, SQL_ATTR_TRACEFILE, (SQLPOINTER)str.c_str(), SQL_NTS)) {
                case SQL_SUCCESS:
                    break;
                case SQL_SUCCESS_WITH_INFO:
                    diag.update(h_dbc, diagnostic_set::handle_type::dbc);
                    break;
                default:
                    diag.update(h_dbc, diagnostic_set::handle_type::dbc);
                    is_valid = false;
                    return;
                }
            }

            // set attribute (tracing)
            SQLPOINTER p_tracing = options.enable_tracing ? reinterpret_cast<SQLPOINTER>(SQL_OPT_TRACE_ON) : reinterpret_cast<SQLPOINTER>(SQL_OPT_TRACE_OFF);
            switch (SQLSetConnectAttrW(h_dbc, SQL_ATTR_TRACE, p_tracing, 0)) {
            case SQL_SUCCESS:
                break;
            case SQL_SUCCESS_WITH_INFO:
                diag.update(h_dbc, diagnostic_set::handle_type::dbc);
                break;
            default:
                diag.update(h_dbc, diagnostic_set::handle_type::dbc);
                is_valid = false;
                return;
            }
        }

        ~handle() {
            if (is_connected())
                SQLDisconnect(h_dbc);

            if (h_dbc)
                SQLFreeHandle(SQL_HANDLE_DBC, h_dbc);
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
                diag.update(h_dbc, diagnostic_set::handle_type::dbc);
                return true;
            default:
                diag.update(h_dbc, diagnostic_set::handle_type::dbc);
                return false;
            }
        }

        bool is_connected() {
            SQLUINTEGER output;
            switch (SQLGetConnectAttrW(h_dbc, SQL_ATTR_CONNECTION_DEAD, &output, 0, nullptr)) {
            case SQL_SUCCESS:
                break;
            case SQL_SUCCESS_WITH_INFO:
                break;
            default:
                diag.update(h_dbc, diagnostic_set::handle_type::dbc);
                return false;
            }

            switch (output) {
            case SQL_CD_TRUE:
                return false;
            case SQL_CD_FALSE:
                return true;
            default:
                return false;
            }
        }

        void disconnect() {
            if (is_connected())
                SQLDisconnect(h_dbc);
        }
    };

    // Connection definition
    database_connection::database_connection(environment& env, database_connection::alloc_options& options) : sp_handle(std::make_unique<handle>(env, options)) {}
    database_connection::~database_connection() = default;
    database_connection::database_connection(database_connection&&) noexcept = default;
    database_connection& database_connection::operator=(database_connection&&) noexcept = default;

    bool database_connection::connect(std::string connection_string) {
        return sp_handle ? sp_handle->connect(connection_string) : false;
    }

    bool database_connection::is_connected() {
        return sp_handle ? sp_handle->is_connected() : false;
    }

    void database_connection::disconnect() {
        if (sp_handle)
            sp_handle->disconnect();
    }

    bool database_connection::is_valid() {
        return !sp_handle ? false : sp_handle->is_valid;
    }

    diagnostic_set* database_connection::diagnostics() {
        return !sp_handle ? nullptr : &sp_handle->diag;
    }

    void* get_dbc_handle(database_connection& dbc) noexcept {
        return dbc.sp_handle ? reinterpret_cast<void*>(dbc.sp_handle->h_dbc) : nullptr;
    }
}