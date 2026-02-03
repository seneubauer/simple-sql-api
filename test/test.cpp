// SimQL stuff
#include <SimQuery.hpp>
#include <SimDatabase.hpp>
#include <SimDiagnosticSet.hpp>
#include <SimQL_Types.hpp>
#include <SimConnectionBuilder.hpp>

// STL stuff
#include <string>
#include <cstdint>
#include <iostream>
#include <utility>

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
    db.set_access_mode(SimpleSqlTypes::AccessModeType::READ_ONLY);
    db.set_autocommit(SimpleSqlTypes::AutocommitType::DISABLED);
    db.set_login_timeout(5);

    std::cout << conn_str << std::endl;
    std::uint8_t rc = db.connect(conn_str);
    if (rc > 0)
        std::cout << db.return_code_def(rc) << std::endl;

    std::cout << "creating query object" << std::endl;
    SimpleSql::SimQuery query(db.statement_handle());

    std::cout << "settings sql" << std::endl;
    query.set_sql(std::string(Secrets::QUERY));

    std::cout << "preparing" << std::endl;
    rc = query.prepare();
    if (rc > 0) {
        std::cout << "could not prepare..." << std::endl;
        std::cout << query.return_code_def(rc) << std::endl;

        if (query.diagnostics()) {
            std::cout << "diagnostics exist" << std::endl;
            for (auto& element : query.diagnostics()->view_diagnostics()) {
                std::cout << element.message << std::endl;
            }
        }

        // if (query.diagnostics()) {
        //     for (SimpleSql::SimDiagnosticSet::Diagnostic& diag : query.diagnostics()->view_diagnostics()) { 
        //         std::cout << diag.sql_state << " " << diag.message << std::endl;
        //     }
        // }
    }

    std::cout << "executing" << std::endl;
    if (query.execute()) {

        // auto result_set = query.get_result_set();
        // if (result_set) {
        //     std::cout << "r: " << std::to_string(result_set->rows()) << std::endl;
        //     std::cout << "c: " << std::to_string(result_set->columns()) << std::endl;
        //     std::cout << "i: " << std::to_string(result_set->size()) << std::endl;
        // }

        // print diagnostics
        // if (query.diagnostics()) {
        //     for (SimpleSql::SimDiagnosticSet::Diagnostic& diag : query.diagnostics()->view_diagnostics()) { 
        //         std::cout << diag.sql_state << " " << diag.message << std::endl;
        //     }
        // }
    }
    std::cout << "finished" << std::endl;

    return 0;
}
