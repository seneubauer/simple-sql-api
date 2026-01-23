#include <SimDatabase.h>
#include <SimQuery.h>
#include <SimQL_Constants.h>
#include <string>
#include <cstdint>
#include <secrets.h>

int main() {

    std::uint8_t stmt_count = 16;
    std::string driver = SimpleSqlConstants::DatabaseDrivers::ODBC_17_SQL_SERVER;
    std::string server = Secrets::SERVER_ADDRESS;
    std::string database = Secrets::DATABASE_NAME;
    std::uint16_t port = Secrets::SERVER_PORT;
    bool readonly = true;
    bool trusted = true;
    bool encrypt = false;

    SimpleSql::SimDatabase db(stmt_count);
    db.start(driver, server, database, port, readonly, trusted, encrypt);

    return 0;
}