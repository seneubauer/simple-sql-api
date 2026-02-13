#ifndef test_secrets_header_h
#define test_secrets_header_h

#include <string_view>
#include <cstdint>

namespace test_secrets {

    enum class operating_system : std::uint8_t {
        windows,
        linux
    };

    static constexpr operating_system current_os{operating_system::windows};
    static constexpr std::string_view server{"10.223.26.60"};
    static constexpr std::string_view database{"staging_mn"};
    static constexpr std::string_view uid{};
    static constexpr std::string_view pwd{};
    static constexpr std::uint16_t port = 1433;
    static constexpr std::string_view query{"SELECT * FROM dbo.district"};
}

#endif