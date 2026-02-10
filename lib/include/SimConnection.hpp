#ifndef SimConnection_header_h
#define SimConnection_header_h

// SimQL stuff
#include <SimEnvironment.hpp>
#include <SimQL_ReturnCodes.hpp>

// STL stuff
#include <cstdint>
#include <memory>

namespace SimpleSql {
    class Connection {
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
        explicit Connection(Environment& env, const Options& options);
        ~Connection();

        /* set to move assignment */
        Connection(Connection&&) noexcept;
        Connection& operator=(Connection&&) noexcept;

        /* disable copy assignment */
        Connection(const Connection&) = delete;
        Connection& operator=(const Connection&) = delete;

        /* functions */
        void connect(std::string_view connection_string);
        bool is_connected();
        void disconnect();
        const SimQL_ReturnCodes::Code& return_code();

    private:
        struct handle;
        std::unique_ptr<handle> sp_handle;
        friend void* get_dbc_handle(Connection& dbc) noexcept;
    };
}

#endif