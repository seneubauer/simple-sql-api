// SimQL stuff
#include <SimConnectionBuilder.hpp>
#include <SimQL_Types.hpp>

// STL stuff
#include <string>
#include <cstdint>

std::string SimpleSql::SimConnectionBuilder::get_connection_string() const {
    std::string output;
    switch (m_database_type) {
    case SimpleSqlTypes::DatabaseType::SQL_SERVER:

        break;
    case SimpleSqlTypes::DatabaseType::POSTGRESQL:

        break;
    }
    return output;
}

void SimpleSql::SimConnectionBuilder::set_driver(const std::string& driver) {
    if (m_driver != driver)
        m_driver = driver;
}

void SimpleSql::SimConnectionBuilder::set_server(const std::string& server) {
    if (m_server != server)
        m_server = server;
}

void SimpleSql::SimConnectionBuilder::set_database(const std::string& database) {
    if (m_database != database)
        m_database = database;
}

void SimpleSql::SimConnectionBuilder::set_username(const std::string& username) {
    if (m_username != username)
        m_username = username;
}

void SimpleSql::SimConnectionBuilder::set_password(const std::string& password) {
    if (m_password != password)
        m_password = password;
}

void SimpleSql::SimConnectionBuilder::set_port(const std::uint16_t& port) {
    if (m_port != port)
        m_port = port;
}

void SimpleSql::SimConnectionBuilder::set_readonly(const bool& readonly) {
    if (m_readonly != readonly)
        m_readonly = readonly;
}

void SimpleSql::SimConnectionBuilder::set_trusted(const bool& trusted) {
    if (m_trusted != trusted)
        m_trusted = trusted;
}

void SimpleSql::SimConnectionBuilder::set_encrypt(const bool& encrypt) {
    if (m_encrypt != encrypt)
        m_encrypt = encrypt;
}
