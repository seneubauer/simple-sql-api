#include <SimDatabase.hpp>
#include <SimQuery.hpp>
#include <SimQL_Constants.hpp>
#include <string>
#include <cstdint>
#include <secrets.hpp>
#include <iostream>

int main() {

    std::uint8_t stmt_count = 16;
    std::string driver = SimpleSqlConstants::DatabaseDrivers::ODBC_17_SQL_SERVER;
    std::string server = Secrets::SERVER_ADDRESS;
    std::string database = Secrets::DATABASE_NAME;
    std::uint16_t port = Secrets::SERVER_PORT;
    bool readonly = true;
    bool trusted = true;
    bool encrypt = false;

    std::cout << "creating SimDatabase" << std::endl;
    SimpleSql::SimDatabase db(stmt_count);

    std::cout << "starting..." << std::endl;
    
    std::uint8_t rc = db.start(driver, server, database, port, readonly, trusted, encrypt);
    if (rc > 0)
        std::cout << SimpleSqlConstants::return_code_def(rc) << std::endl;

    std::cout << "end" << std::endl;
    return 0;
}