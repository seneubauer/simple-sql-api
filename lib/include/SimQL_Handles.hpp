#ifndef SimQL_Handles_header_h
#define SimQL_Handles_header_h

// STL stuff
#include <memory>
#include <cstdint>

namespace SimpleSqlHandles {

    class Environment {
    public:

        /* enums */
        enum class PoolingType : std::uint8_t {
            OFF,
            ONE_PER_DRIVER,
            ONE_PER_ENV
        };

        enum class PoolMatchType : std::uint8_t {
            STRICT_MATCH,
            RELAXED_MATCH
        };

        /* handle options */
        struct env_options {
            PoolingType pool_type = PoolingType::ONE_PER_DRIVER;
            PoolMatchType match_type = PoolMatchType::STRICT_MATCH;
        };

        /* constructor/destructor */
        explicit Environment(const env_options& options = {});
        ~Environment();

        /* set to move assignment */
        Environment(Environment&&) noexcept;
        Environment& operator=(Environment&&) noexcept;

        /* disable copy assignment */
        Environment(const Environment&) = delete;
        Environment& operator=(const Environment&) = delete;

        /* get return states */
        const bool& pending_info();
        const std::uint8_t& return_code();

    private:

        /* handle container */
        struct handle;
        std::unique_ptr<handle> sp_handle;

        /* return codes */
        static constexpr std::uint8_t _RC_SUCCESS               = 0;
        static constexpr std::uint8_t _RC_ALLOC_HANDLE          = 1;
        static constexpr std::uint8_t _RC_ODBC_VERSION3         = 2;
        static constexpr std::uint8_t _RC_POOLING_TYPE          = 2;
        static constexpr std::uint8_t _RC_POOLING_MATCH_TYPE    = 3;
    };

    class Connection {
    private:

        /* custom deleter */
        struct deleter;

    public:

        /* constructor/destructor */
        Connection() = default;
        ~Connection() { deallocate(); }

        /* functions */
        const std::uint8_t& allocate(void* h_env);
        void deallocate();
        std::unique_ptr<void, deleter>& handle();

    private:

        /* handle container */
        std::unique_ptr<void, deleter> m_handle;

        /* return codes */
        static constexpr std::uint8_t _RC_SUCCESS               = 0;
        static constexpr std::uint8_t _RC_SUCCESS_WITH_INFO     = 1;
        static constexpr std::uint8_t _RC_FAILURE               = 2;
    };

    class Statement {
    private:

        /* custom deleter */
        struct deleter;

    public:

        /* constructor/destructor */
        Statement() = default;
        ~Statement() { deallocate(); }

        /* functions */
        const std::uint8_t& allocate(void* h_dbc);
        const std::uint8_t& reset();
        void deallocate();
        std::unique_ptr<void, deleter>& handle();

    private:

        /* handle container */
        std::unique_ptr<void, deleter> m_handle;

        /* return codes */
        static constexpr std::uint8_t _RC_SUCCESS               = 0;
        static constexpr std::uint8_t _RC_SUCCESS_WITH_INFO     = 1;
        static constexpr std::uint8_t _RC_FAILURE               = 2;
        static constexpr std::uint8_t _RC_CLOSE_CURSOR          = 3;
        static constexpr std::uint8_t _RC_RESET_PARAMS          = 4;
        static constexpr std::uint8_t _RC_UNBIND_COLS           = 5;
    };
}

#endif