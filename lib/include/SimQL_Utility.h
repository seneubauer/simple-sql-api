#ifndef SimQL_Utility_header_h
#define SimQL_Utility_header_h

// STL stuff
#include <string>
#include <format>
#include <cstdlib>
#include <cstdint>

namespace SimpleSqlUtility {

    struct HandleDeleter { void operator()(void* p) const { std::free(p); } };

    inline std::string connection_string(const std::string& driver, const std::string& server, const std::string& database, const std::uint16_t& port, const bool& readonly, const bool& trusted, const bool& encrypt) {
        std::string conn_str = "";
        conn_str += driver.empty() ? std::string() : std::format("Driver={{{}}};", driver);
        conn_str += server.empty() ? std::string() : std::format("Server={{{}}};", server);
        conn_str += database.empty() ? std::string() : std::format("Database={{{}}};", database);
        conn_str += port < 0 ? std::string() : std::format("Port={{{}}};", port);
        conn_str += readonly ? std::string("Application_Intent=ReadOnly};") : std::string("Application_Intent={ReadWrite};");
        conn_str += trusted ? std::string("Trusted_Connection={Yes};") : std::string("Trusted_Connection={No};");
        conn_str += encrypt ? std::string("Encrypt={Yes};") : std::string("Encrypt={No};");
        return conn_str;
    }

    inline std::string connection_string(const std::string& driver, const std::string& server, const std::string& database, const std::uint16_t& port, const bool& readonly, const bool& trusted, const bool& encrypt, const std::string& username, const std::string& password) {
        std::string conn_str = "";
        conn_str += driver.empty() ? std::string() : std::format("Driver={{{}}};", driver);
        conn_str += server.empty() ? std::string() : std::format("Server={{{}}};", server);
        conn_str += database.empty() ? std::string() : std::format("Database={{{}}};", database);
        conn_str += port < 0 ? std::string() : std::format("Port={{{}}};", port);
        conn_str += readonly ? std::string("Application_Intent=ReadOnly};") : std::string("Application_Intent={ReadWrite};");
        conn_str += trusted ? std::string("Trusted_Connection={Yes};") : std::string("Trusted_Connection={No};");
        conn_str += encrypt ? std::string("Encrypt={Yes};") : std::string("Encrypt={No};");
        conn_str += username.empty() ? std::string() : std::format("UID={{{}}};", username);
        conn_str += password.empty() ? std::string() : std::format("PWD={{{}}};", password);
        return conn_str;
    }
    
}

#endif