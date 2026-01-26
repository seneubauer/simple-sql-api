// SimQL stuff
#include <SimQuery.hpp>
#include <SimDatabase.hpp>
#include <SimQL_Types.hpp>
#include <SimQL_Constants.hpp>
#include <SimConnectionBuilder.hpp>

// STL stuff
#include <string>
#include <cstdint>
#include <iostream>

// secrets
#include <secrets.hpp>

int main() {

    // get secret values >:)    
    std::string driver(SimpleSqlConstants::DatabaseDrivers::ODBC_17_SQL_SERVER);
    std::string server(Secrets::SERVER);
    std::string database(Secrets::DATABASE);
    // std::string uid(Secrets::UID);
    // std::string password(Secrets::PWD);

    // build connection string
    auto builder = SimpleSql::SimConnectionBuilder(SimpleSqlTypes::DatabaseType::SQL_SERVER);
    builder.set_driver(driver);
    builder.set_server(server);
    builder.set_port(Secrets::PORT);
    builder.set_database(database);
    builder.set_readonly(true);
    builder.set_trusted(true);
    std::string conn_str = builder.get();

    // initialize & configure SimDatabase
    SimpleSql::SimDatabase db(16);
    db.set_access_mode(SimpleSqlTypes::AccessModeType::READ_ONLY);
    db.set_autocommit(SimpleSqlTypes::AutocommitType::DISABLED);
    db.set_login_timeout(5);

    std::uint8_t rc = db.connect(conn_str);
    if (rc > 0)
        std::cout << SimpleSqlConstants::return_code_def(rc);

    return 0;
}
