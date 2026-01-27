// SimQL stuff
#include <SimQuery.hpp>
#include <SimDatabase.hpp>
#include <SimQL_Types.hpp>
#include <SimConnectionBuilder.hpp>

// STL stuff
#include <string>
#include <cstdint>
#include <iostream>

// secrets
#include <secrets.hpp>

int main() {

    // build connection string
    std::string conn_str;
    switch (Secrets::SYSTEM) {
    case Secrets::OperatingSystems::Windows:
        {
            auto builder = SimpleSql::SimConnectionBuilder(SimpleSqlTypes::DatabaseType::SQL_SERVER);
            builder.set_driver(std::string(SimpleSqlConstants::DatabaseDrivers::ODBC_17_SQL_SERVER));
            builder.set_server(std::string(Secrets::SERVER));
            builder.set_port(Secrets::PORT);
            builder.set_database(std::string(Secrets::DATABASE));
            builder.set_trusted(true);
            conn_str = builder.get();
        }
        break;
    case Secrets::OperatingSystems::Linux:
        {
            auto builder = SimpleSql::SimConnectionBuilder(SimpleSqlTypes::DatabaseType::POSTGRESQL);
            builder.set_driver(std::string(SimpleSqlConstants::DatabaseDrivers::PSQL_ODBC_ANSI));
            builder.set_server(std::string(Secrets::SERVER));
            builder.set_database(std::string(Secrets::DATABASE));
            builder.set_port(Secrets::PORT);
            builder.set_username(std::string(Secrets::UID));
            builder.set_password(std::string(Secrets::PWD));
            conn_str = builder.get();
        }
        break;
    default:
        return 0;
    }

    // initialize & configure SimDatabase
    std::cout << "creating db object" << std::endl;
    SimpleSql::SimDatabase db(16);
    // db.set_access_mode(SimpleSqlTypes::AccessModeType::READ_ONLY);
    // db.set_autocommit(SimpleSqlTypes::AutocommitType::DISABLED);
    // db.set_login_timeout(5);

    std::cout << conn_str << std::endl;
    std::uint8_t rc = db.connect(conn_str);
    if (rc > 0)
        std::cout << db.return_code_definition(rc) << std::endl;

    return 0;
}
