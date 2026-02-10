#ifndef SimQL_Strings_header_h
#define SimQL_Strings_header_h

// STL stuff
#include <string>
#include <string_view>
#include <memory>

namespace SimpleSqlStrings {
    std::basic_string<wchar_t> utf8_to_odbc(std::string_view s);
    std::string odbc_to_utf8(std::basic_string<wchar_t> s);

    class Converter {
    public:
        Converter();
        ~Converter();
    private:
        struct impl;
        std::unique_ptr<impl> p_impl;
    };
}

#endif