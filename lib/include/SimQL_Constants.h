#ifndef SimQL_Constants_header_h
#define SimQL_Constants_header_h

#include <cstdint>

namespace SimpleSqlConstants {

    // sets the upper limit for user defined statement pool size
    static constexpr std::uint8_t governing_maximum_concurrency = 16;

}

#endif