#ifndef database_connection_header_h
#define database_connection_header_h

// SimQL stuff
#include <SimEnvironment.hpp>
#include <SimQL_ReturnCodes.hpp>

// STL stuff
#include <cstdint>
#include <memory>

namespace simql {
    class database_connection {
    public:

        /* handle options */
        struct Options {
            bool read_only = false;
            std::uint32_t connection_timeout = 0;
            std::uint32_t login_timeout = 0;
            std::uint32_t packet_size = 0;
            bool enable_async = false;
            bool enable_autocommit = true;
            bool enable_tracing = false;
            std::string tracefile;
        };

        /* constructor/destructor */
        explicit database_connection(environment& env, Options& options);
        ~database_connection();

        /* set to move assignment */
        database_connection(database_connection&&) noexcept;
        database_connection& operator=(database_connection&&) noexcept;

        /* disable copy assignment */
        database_connection(const database_connection&) = delete;
        database_connection& operator=(const database_connection&) = delete;

        /* functions */
        void connect(std::string_view connection_string);
        bool is_connected();
        void disconnect();
        const simql_returncodes::code& return_code();

    private:
        struct handle;
        std::unique_ptr<handle> sp_handle;
        friend void* get_dbc_handle(database_connection& dbc) noexcept;
    };
}

#endif