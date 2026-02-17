// SimQL stuff
#include "connection_string_builder.hpp"
#include "simql_constants.hpp"
#include "environment.hpp"
#include "database_connection.hpp"
#include "statement.hpp"
#include "diagnostic_set.hpp"
#include "result_set.hpp"

// STL stuff
#include <string>
#include <cstdint>
#include <iostream>
#include <filesystem>
#include <vector>

// secrets
#include <test_secrets.hpp>

int main() {

    simql::connection_string_builder builder;
    switch (test_secrets::current_os) {
    case test_secrets::operating_system::windows:
        builder.set_db_type(simql::connection_string_builder::database_type::sql_server);
        builder.set_driver(std::string(simql_constants::database_drivers::odbc_17_sql_server));
        builder.set_trusted(true);
        break;
    case test_secrets::operating_system::linux:
        builder.set_db_type(simql::connection_string_builder::database_type::postgresql);
        builder.set_driver(std::string(simql_constants::database_drivers::psql_odbc));
        builder.set_username(std::string(test_secrets::uid));
        builder.set_password(std::string(test_secrets::pwd));
        break;
    default:
        return 0;
    }
    builder.set_server(std::string(test_secrets::server));
    builder.set_database(std::string(test_secrets::database));
    builder.set_port(test_secrets::port);

    // select environment allocation options
    simql::environment::alloc_options env_opts;
    env_opts.match_type = simql::environment::pooling_match_type::strict_match;
    env_opts.pool_type = simql::environment::pooling_type::one_per_driver;

    // helper to print diagnostics
    auto diag_printer = [&](simql::diagnostic_set* diag) {
        if (diag) {
            if (diag->view_diagnostics().empty())
                std::cout << "diagnostic buffer is empty" << std::endl;

            for (auto& element : diag->view_diagnostics()) {
                std::cout << element.sql_state << "," << diag->state_description(element.sql_state) << "," << element.message << std::endl;
            }
        } else {
            std::cout << "diagnostic buffer is null" << std::endl;
        }
    };

    // allocate the environment handle
    simql::environment env(env_opts);
    if (!env.is_valid()) {
        std::cout << "environment alloc error" << std::endl;
        diag_printer(env.diagnostics());
        return 1;
    }

    // select connection allocation options
    simql::database_connection::alloc_options dbc_opts;
    dbc_opts.connection_timeout = 30;
    dbc_opts.enable_async = false;
    dbc_opts.enable_autocommit = true;
    dbc_opts.enable_tracing = true;
    dbc_opts.login_timeout = 10;
    dbc_opts.packet_size = 0;
    dbc_opts.read_only = true;
    dbc_opts.tracefile = test_secrets::tracefile_path;

    // reset the tracefile
    if (dbc_opts.enable_tracing && !dbc_opts.tracefile.empty())
        if (std::filesystem::exists(test_secrets::tracefile_path))
            std::filesystem::remove(test_secrets::tracefile_path);

    // allocate the database connection handle
    simql::database_connection dbc(env, dbc_opts);
    if (!dbc.is_valid()) {
        std::cout << "database connection alloc error" << std::endl;
        diag_printer(dbc.diagnostics());
        return 1;
    }

    // open the connection to the database
    if (!dbc.connect(builder.get())) {
        std::cout << "database connection open error" << std::endl;
        diag_printer(dbc.diagnostics());
        return 1;
    }

    // test connection
    if (dbc.is_connected())
        std::cout << "connection is open" << std::endl;

    // select statement allocation options
    simql::statement::alloc_options stmt_opts;
    stmt_opts.cursor = simql::statement::cursor_type::forward_only;
    stmt_opts.max_rows = 100000;
    stmt_opts.query_timeout = 30;

    // allocate a statement handle
    simql::statement stmt(dbc, stmt_opts);
    if (!stmt.is_valid()) {
        std::cout << "statement alloc error" << std::endl;
        diag_printer(stmt.diagnostics());
        return 1;
    }

    // prepare a select statement
    if (!stmt.prepare(test_secrets::generic_unfiltered_select)) {
        std::cout << "prepare error" << std::endl;
        diag_printer(stmt.diagnostics());
        return 1;
    }

    // execute the select statement
    if (!stmt.execute()) {
        std::cout << "execute error" << std::endl;
        diag_printer(stmt.diagnostics());
        return 1;
    }

    // get the result set
    if (!stmt.results()) {
        std::cout << "no result set" << std::endl;
    } else {
        std::cout << "row: " << std::to_string(stmt.results()->row_count()) << std::endl;
        std::cout << "col: " << std::to_string(stmt.results()->column_count()) << std::endl;
    }

    std::cout << "end of test" << std::endl;
    return 0;
}
