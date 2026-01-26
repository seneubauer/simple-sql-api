#include <SimDatabase.hpp>
#include <SimQuery.hpp>
#include <SimQL_Constants.hpp>
#include <string>
#include <cstdint>
#include <secrets.hpp>
#include <iostream>

int main() {

    // get secret values >:)    
    std::string driver(SimpleSqlConstants::DatabaseDrivers::ODBC_17_SQL_SERVER);
    std::string server(Secrets::SERVER);
    std::string database(Secrets::DATABASE);
    std::string uid(Secrets::UID);
    std::string password(Secrets::PWD);

    std::uint8_t stmt_count = 16;
    bool readonly = true;
    bool trusted = true;
    bool encrypt = false;

    std::cout << "creating SimDatabase" << std::endl;
    SimpleSql::SimDatabase db(stmt_count);

    std::cout << "starting..." << std::endl;
    std::uint8_t rc = db.start(driver, server, database, Secrets::PORT, readonly, trusted, encrypt);
    if (rc > 0)
        std::cout << SimpleSqlConstants::return_code_def(rc) << std::endl;

    std::cout << "end" << std::endl;
    return 0;
}
