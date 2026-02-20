#ifndef database_connection_header_h
#define database_connection_header_h

// SimQL stuff
#include "environment.hpp"

// STL stuff
#include <cstdint>
#include <memory>
#include <string_view>

namespace simql {
    class diagnostic_set;
    class database_connection {
    public:

        struct alloc_options {
            bool read_only{false};
            std::uint32_t connection_timeout{0};
            std::uint32_t login_timeout{0};
            std::uint32_t packet_size{0};
            bool enable_autocommit{true};
            bool enable_tracing{false};
            std::string tracefile{};
        };

        explicit database_connection(environment& env, alloc_options& options);
        ~database_connection();
        database_connection(database_connection&&) noexcept;
        database_connection& operator=(database_connection&&) noexcept;
        database_connection(const database_connection&) = delete;
        database_connection& operator=(const database_connection&) = delete;

        bool connect(std::string connection_string);
        bool is_connected();
        void disconnect();

        bool is_valid();
        std::string_view last_error();
        diagnostic_set* diagnostics();

    private:
        struct handle;
        std::unique_ptr<handle> p_handle;
        friend void* get_dbc_handle(database_connection& dbc) noexcept;
    };
}

#endif