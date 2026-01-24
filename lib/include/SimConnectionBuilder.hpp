#ifndef SimConnectionBuilder_header_h
#define SimConnectionBuilder_header_h

// SimQL stuff
#include <SimQL_Types.hpp>

// STL stuff
#include <string>
#include <cstdint>

namespace SimpleSql {
    class SimConnectionBuilder {
    private:

        // members
        SimpleSqlTypes::DatabaseType m_database_type;
        std::string m_driver;
        std::string m_server;
        std::string m_database;
        std::string m_username;
        std::string m_password;
        std::uint16_t m_port;
        bool m_readonly;
        bool m_trusted;
        bool m_encrypt;

    public:
        SimConnectionBuilder(const SimpleSqlTypes::DatabaseType& database_type) : m_database_type(database_type) {}
        ~SimConnectionBuilder() {}

        std::string get_connection_string() const;
        void set_driver(const std::string& driver);
        void set_server(const std::string& server);
        void set_database(const std::string& database);
        void set_username(const std::string& username);
        void set_password(const std::string& password);
        void set_port(const std::uint16_t& port);
        void set_readonly(const bool& readonly);
        void set_trusted(const bool& trusted);
        void set_encrypt(const bool& encrypt);
    };
}

#endif