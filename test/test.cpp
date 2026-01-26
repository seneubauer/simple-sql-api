#include <secrets.hpp>
#include <SimDatabase.hpp>
#include <SimQuery.hpp>
#include <SimQL_Constants.hpp>
#include <SimConnectionBuilder.hpp>
#include <string>
#include <cstdint>
#include <iostream>

int main() {

    // get secret values >:)    
    std::string driver(SimpleSqlConstants::DatabaseDrivers::ODBC_17_SQL_SERVER);
    std::string server(Secrets::SERVER);
    std::string database(Secrets::DATABASE);
    // std::string uid(Secrets::UID);
    // std::string password(Secrets::PWD);

    // std::uint8_t stmt_count = 16;

    auto builder = SimpleSql::SimConnectionBuilder(SimpleSqlTypes::DatabaseType::SQL_SERVER);
    builder.set_driver(driver);
    builder.set_server(server);
    builder.set_port(Secrets::PORT);
    builder.set_database(database);
    builder.set_readonly(true);
    builder.set_trusted(true);

    std::cout << "connection string: " << builder.get() << std::endl;

    // std::cout << "creating SimDatabase" << std::endl;
    // SimpleSql::SimDatabase db(stmt_count);


    // std::cout << "end" << std::endl;
    return 0;
}
