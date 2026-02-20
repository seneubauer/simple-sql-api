#ifndef environment_header_h
#define environment_header_h

// STL stuff
#include <cstdint>
#include <memory>
#include <string_view>

namespace simql {
    class diagnostic_set;
    class environment {
    public:

        enum class pooling_type : std::uint8_t {
            off,
            one_per_driver,
            one_per_env
        };

        enum class match_type : std::uint8_t {
            strict_match,
            relaxed_match
        };

        enum class odbc_version : std::uint8_t {
            odbc3x,
            odbc38
        };

        struct alloc_options {
            pooling_type pooling = pooling_type::one_per_driver;
            match_type match = match_type::strict_match;
            odbc_version odbc = odbc_version::odbc3x;
        };

        explicit environment(const alloc_options& options);
        ~environment();
        environment(environment&&) noexcept;
        environment& operator=(environment&&) noexcept;
        environment(const environment&) = delete;
        environment& operator=(const environment&) = delete;

        bool is_valid();
        std::string_view last_error();
        diagnostic_set* diagnostics();

    private:
        struct handle;
        std::unique_ptr<handle> p_handle;
        friend void* get_env_handle(environment& env) noexcept;
    };
}

#endif