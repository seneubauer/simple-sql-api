#ifndef SimEnvironment_header_h
#define SimEnvironment_header_h

// SimQL stuff
#include <SimQL_ReturnCodes.hpp>

// STL stuff
#include <cstdint>
#include <memory>

namespace SimpleSql {
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
        struct Options {
            PoolingType pool_type = PoolingType::ONE_PER_DRIVER;
            PoolMatchType match_type = PoolMatchType::STRICT_MATCH;
        };

        /* constructor/destructor */
        explicit Environment(const Options& options = {});
        ~Environment();

        /* set to move assignment */
        Environment(Environment&&) noexcept;
        Environment& operator=(Environment&&) noexcept;

        /* disable copy assignment */
        Environment(const Environment&) = delete;
        Environment& operator=(const Environment&) = delete;

        /* functions */
        const SimQL_ReturnCodes::Code& return_code();

    private:
        struct handle;
        std::unique_ptr<handle> sp_handle;
        friend void* get_env_handle(Environment& env) noexcept;
    };
}

#endif