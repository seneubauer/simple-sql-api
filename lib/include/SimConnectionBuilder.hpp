#ifndef SimConnectionBuilder_header_h
#define SimConnectionBuilder_header_h

// STL stuff
#include <string>
#include <cstdint>

namespace SimpleSql {
    class SimConnectionBuilder {
    private:

        // members
        std::string m_driver;
        std::string m_server;
        std::string m_database;
        std::uint16_t m_port;
        bool m_readonly;
        bool m_trusted;
        bool m_encrypt;

    public:
        SimConnectionBuilder() {}
        ~SimConnectionBuilder() {}
    };
}

#endif