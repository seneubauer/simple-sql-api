// SimQL stuff
#include <connection_string_builder.hpp>
#include <simql_types.hpp>

// STL stuff
#include <format>
#include <string>
#include <cstdint>

namespace simql {

    std::string connection_string_builder::get() {
        std::string connection_string = assemble();
        destroy();
        return connection_string;
    }

    void connection_string_builder::set_driver(const std::string& driver) {
        if (m_driver != driver)
            m_driver = driver;
    }

    void connection_string_builder::set_server(const std::string& server) {
        if (m_server != server)
            m_server = server;
    }

    void connection_string_builder::set_port(const std::uint16_t& port) {
        if (m_port != port)
            m_port = port;
    }

    void connection_string_builder::set_database(const std::string& database) {
        if (m_database != database)
            m_database = database;
    }

    void connection_string_builder::set_username(const std::string& username) {
        if (m_username != username)
            m_username = username;
    }

    void connection_string_builder::set_password(const std::string& password) {
        if (m_password != password)
            m_password = password;
    }

    void connection_string_builder::set_sslmode(const bool& sslmode) {
        if (m_sslmode != sslmode)
            m_sslmode = sslmode;
    }

    void connection_string_builder::set_mars(const bool& mars) {
        if (m_mars != mars)
            m_mars = mars;
    }

    void connection_string_builder::set_readonly(const bool& readonly) {
        if (m_readonly != readonly)
            m_readonly = readonly;
    }

    void connection_string_builder::set_trusted(const bool& trusted) {
        if (m_trusted != trusted)
            m_trusted = trusted;
    }

    void connection_string_builder::set_encrypt(const bool& encrypt) {
        if (m_encrypt != encrypt)
            m_encrypt = encrypt;
    }

    void connection_string_builder::destroy() {
        m_driver.clear();
        m_server.clear();
        m_port = 0;
        m_database.clear();
        m_username.clear();
        m_password.clear();
        m_sslmode = false;
        m_mars = false;
        m_readonly = false;
        m_trusted = false;
        m_encrypt = false;
    }

    std::string connection_string_builder::assemble() const {
        std::string output;
        switch (m_db_type) {
        case database_type::sql_server:

            if (!m_driver.empty())
                output += std::format("Driver={{{}}};", m_driver);
            
            if (!m_server.empty()) {
                if (m_port > 0) {
                    output += std::format("Server={{{},{}}};", m_server, std::to_string(m_port));
                } else {
                    output += std::format("Server={{{}}};", m_server);
                }
            }

            if (!m_database.empty())
                output += std::format("Database={{{}}};", m_database);

            if (!m_username.empty())
                output += std::format("UID={{{}}};", m_username);

            if (!m_password.empty())
                output += std::format("PWD={{{}}};", m_password);

            output += std::format("MARS_Connection={};", m_mars ? "yes" : "no");
            output += std::format("ApplicationIntent={};", m_readonly ? "ReadOnly" : "ReadWrite");
            output += std::format("Trusted_Connection={};", m_trusted ? "yes" : "no");
            output += std::format("Encrypt={};", m_encrypt ? "yes" : "no");

            break;
        case database_type::postgresql:

            if (!m_driver.empty())
                output += std::format("Driver={{{}}};", m_driver);

            if (!m_server.empty())
                output += std::format("Server={{{}}};", m_server);

            if (!m_database.empty())
                output += std::format("Database={{{}}};", m_database);

            if (!m_username.empty())
                output += std::format("Uid={{{}}};", m_username);

            if (!m_password.empty())
                output += std::format("Pwd={{{}}};", m_password);

            if (m_port > 0)
                output += std::format("Port={};", std::to_string(m_port));

            if (m_sslmode)
                output += std::string("sslmode=require;");

            break;
        }
        return output;
    }

}