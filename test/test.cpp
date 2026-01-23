#include <SimDatabase.h>
#include <SimQuery.h>
#include <SimQL_Constants.h>
#include <string>
#include <cstdint>

int main() {

    std::uint8_t stmt_count = 16;
    std::string driver = SimpleSqlConstants::DatabaseDrivers::ODBC_17_SQL_SERVER;
    std::string server = "10.223.26.60";
    std::string database = "staging_mn";
    std::uint16_t port = 1433;
    bool readonly = true;
    bool trusted = true;
    bool encrypt = false;

    SimpleSql::SimDatabase db(stmt_count);

    return 0;
}