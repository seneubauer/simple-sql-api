// SimQL stuff
#include "environment.hpp"
#include "diagnostic_set.hpp"

// STL stuff
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>

// OS stuff
#include "os_inclusions.hpp"

// ODBC stuff
#include <sqltypes.h>
#include <sqlext.h>
#include <sql.h>

namespace simql {

    struct environment::handle {

        // handle(s)
        SQLHENV h_env{SQL_NULL_HENV};

        // diagnostics
        std::string last_error{};
        diagnostic_set diag{};

        // trackers
        bool is_valid{true};

        explicit handle(const environment::alloc_options& options) {

            // allocate the handle
            switch (SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &h_env)) {
            case SQL_SUCCESS:
                break;
            case SQL_SUCCESS_WITH_INFO:
                diag.update(h_env, diagnostic_set::handle_type::env, std::string{"SQLAllocHandle(SQL_HANDLE_ENV) -> SUCCESS_WITH_INFO"});
                break;
            case SQL_INVALID_HANDLE:
                last_error = std::string{"could not allocate the environment handle: invalid handle"};
                is_valid = false;
                return;
            default:
                last_error = std::string{"could not allocate the environment handle: generic error"};
                is_valid = false;
                return;
            }

            // set version
            is_valid = set_odbc_version(options.odbc);
            if (!is_valid)
                return;

            // set connection pooling type
            is_valid = set_pooling_type(options.pooling);
            if (!is_valid)
                return;

            // set connection pool match type
            is_valid = set_match_type(options.match);
            if (!is_valid)
                return;

        }

        ~handle() {
            if (h_env)
                SQLFreeHandle(SQL_HANDLE_ENV, h_env);
        }

        bool set_odbc_version(environment::odbc_version odbc) {

            SQLPOINTER p_odbc{};
            switch (odbc) {
            case environment::odbc_version::odbc3x:
                p_odbc = reinterpret_cast<SQLPOINTER>(SQL_OV_ODBC3);
                break;
            case environment::odbc_version::odbc38:
                last_error = std::string{"ODBC version 3.8 is not available yet"};
                return false;
            }

            switch (SQLSetEnvAttr(h_env, SQL_ATTR_ODBC_VERSION, p_odbc, 0)) {
            case SQL_SUCCESS:
                return true;
            case SQL_SUCCESS_WITH_INFO:
                diag.update(h_env, diagnostic_set::handle_type::env, std::string{"SQLSetEnvAttr(SQL_ATTR_ODBC_VERSION) -> SUCCESS_WITH_INFO"});
                return true;
            case SQL_INVALID_HANDLE:
                last_error = std::string{"could not set the ODBC version: invalid handle"};
                diag.update(h_env, diagnostic_set::handle_type::env, std::string{"SQLSetEnvAttr(SQL_ATTR_ODBC_VERSION) -> INVALID_HANDLE"});
                return false;
            default:
                last_error = std::string{"could not set the ODBC version: generic error"};
                diag.update(h_env, diagnostic_set::handle_type::env, std::string{"SQLSetEnvAttr(SQL_ATTR_ODBC_VERSION) -> ERROR"});
                return false;
            }
        }

        bool set_pooling_type(environment::pooling_type pooling) {

            SQLPOINTER p_pooling{};
            switch (pooling) {
            case environment::pooling_type::off:
                p_pooling = reinterpret_cast<SQLPOINTER>(SQL_CP_OFF);
                break;
            case environment::pooling_type::one_per_driver:
                p_pooling = reinterpret_cast<SQLPOINTER>(SQL_CP_ONE_PER_DRIVER);
                break;
            case environment::pooling_type::one_per_env:
                p_pooling = reinterpret_cast<SQLPOINTER>(SQL_CP_ONE_PER_HENV);
                break;
            }

            switch (SQLSetEnvAttr(h_env, SQL_ATTR_CONNECTION_POOLING, p_pooling, 0)) {
            case SQL_SUCCESS:
                return true;
            case SQL_SUCCESS_WITH_INFO:
                diag.update(h_env, diagnostic_set::handle_type::env, std::string{"SQLSetEnvAttr(SQL_ATTR_CONNECTION_POOLING) -> SUCCESS_WITH_INFO"});
                return true;
            case SQL_INVALID_HANDLE:
                last_error = std::string{"could not set the connection pooling type: invalid handle"};
                diag.update(h_env, diagnostic_set::handle_type::env, std::string{"SQLSetEnvAttr(SQL_ATTR_CONNECTION_POOLING) -> INVALID_HANDLE"});
                return false;
            default:
                last_error = std::string{"could not set the connection pooling type: generic error"};
                diag.update(h_env, diagnostic_set::handle_type::env, std::string{"SQLSetEnvAttr(SQL_ATTR_CONNECTION_POOLING) -> ERROR"});
                return false;
            }
        }

        bool set_match_type(environment::match_type match) {

            SQLPOINTER p_match{};
            switch (match) {
            case environment::match_type::relaxed_match:
                p_match = reinterpret_cast<SQLPOINTER>(SQL_CP_RELAXED_MATCH);
                break;
            case environment::match_type::strict_match:
                p_match = reinterpret_cast<SQLPOINTER>(SQL_CP_STRICT_MATCH);
                break;
            }

            switch (SQLSetEnvAttr(h_env, SQL_ATTR_CP_MATCH, p_match, 0)) {
            case SQL_SUCCESS:
                return true;
            case SQL_SUCCESS_WITH_INFO:
                diag.update(h_env, diagnostic_set::handle_type::env, std::string{"SQLSetEnvAttr(SQL_ATTR_CP_MATCH) -> SUCCESS_WITH_INFO"});
                return true;
            case SQL_INVALID_HANDLE:
                last_error = std::string{"could not set the pool match type: invalid handle"};
                diag.update(h_env, diagnostic_set::handle_type::env, std::string{"SQLSetEnvAttr(SQL_ATTR_CP_MATCH) -> INVALID_HANDLE"});
                return false;
            default:
                last_error = std::string{"could not set the pool match type: generic error"};
                diag.update(h_env, diagnostic_set::handle_type::env, std::string{"SQLSetEnvAttr(SQL_ATTR_CP_MATCH) -> ERROR"});
                return false;
            }
        }

    };

    environment::environment(const environment::alloc_options& options) : p_handle(std::make_unique<handle>(options)) {}
    environment::~environment() = default;
    environment::environment(environment&&) noexcept = default;
    environment& environment::operator=(environment&&) noexcept = default;

    bool environment::is_valid() {
        return !p_handle ? false : p_handle->is_valid;
    }

    std::string_view environment::last_error() {
        return !p_handle ? std::string_view{} : p_handle->last_error;
    }

    diagnostic_set* environment::diagnostics() {
        return !p_handle ? nullptr : &p_handle->diag;
    }

    void* get_env_handle(environment& env) noexcept {
        return env.p_handle ? reinterpret_cast<void*>(env.p_handle->h_env) : nullptr;
    }

}