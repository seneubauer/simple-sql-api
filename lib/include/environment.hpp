#ifndef environment_header_h
#define environment_header_h

// SimQL stuff
#include <simql_returncodes.hpp>

// STL stuff
#include <cstdint>
#include <memory>

namespace simql {
    class environment {
    public:

        /* enums */
        enum class pooling_type : std::uint8_t {
            off,
            one_per_driver,
            one_per_env
        };

        enum class pooling_match_type : std::uint8_t {
            strict_match,
            relaxed_match
        };

        /* handle options */
        struct alloc_options {
            pooling_type pool_type = pooling_type::one_per_driver;
            pooling_match_type match_type = pooling_match_type::strict_match;
        };

        /* constructor/destructor */
        explicit environment(const alloc_options& options);
        ~environment();

        /* set to move assignment */
        environment(environment&&) noexcept;
        environment& operator=(environment&&) noexcept;

        /* disable copy assignment */
        environment(const environment&) = delete;
        environment& operator=(const environment&) = delete;

        /* functions */
        const simql_returncodes::code& return_code();

    private:
        struct handle;
        std::unique_ptr<handle> sp_handle;
        friend void* get_env_handle(environment& env) noexcept;
    };
}

#endif