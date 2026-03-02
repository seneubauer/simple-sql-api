// SimQL stuff
#include "connection_string_builder.hpp"
#include "simql_constants.hpp"
#include "environment.hpp"
#include "database_connection.hpp"
#include "statement.hpp"
#include "diagnostic_set.hpp"

// STL stuff
#include <string>
#include <cstdint>
#include <iostream>
#include <filesystem>
#include <vector>

// secrets
#include <test_secrets.hpp>

// helper to print diagnostics
void diag_printer(simql::diagnostic_set* diag) {
    if (diag) {
        if (diag->view_diagnostics().empty()) {
            std::cout << "diagnostic buffer is empty" << std::endl;
            return;
        }

        for (auto& element : diag->view_diagnostics()) {
            std::cout << element.sql_state << ", " << diag->state_description(element.sql_state) << ", " << element.message << std::endl;
        }
    } else {
        std::cout << "diagnostic buffer is null" << std::endl;
    }
}

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
    env_opts.match = simql::environment::match_type::strict_match;
    env_opts.pooling = simql::environment::pooling_type::one_per_driver;

    // allocate the environment handle
    simql::environment env(env_opts);
    if (!env.is_valid()) {
        std::cout << "environment alloc error: " << env.last_error() << std::endl;
        diag_printer(env.diagnostics());
        return 1;
    }

    // select connection allocation options
    simql::database_connection::alloc_options dbc_opts;
    dbc_opts.connection_timeout = 30;
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
        std::cout << "database connection alloc error: " << dbc.last_error() << std::endl;
        diag_printer(dbc.diagnostics());
        return 1;
    }

    // open the connection to the database
    if (!dbc.connect(builder.get())) {
        std::cout << "database connection open error: " << dbc.last_error() << std::endl;
        diag_printer(dbc.diagnostics());
        return 1;
    }

    // select statement allocation options
    simql::statement::alloc_options stmt_opts;
    stmt_opts.is_scrollable = true;
    stmt_opts.max_rows = 100000;
    stmt_opts.query_timeout = 30;
    stmt_opts.rowset_size = 1000;
    stmt_opts.sensitivity = simql::statement::cursor_sensitivity::sensitive;

    // allocate a statement handle
    simql::statement stmt(dbc, stmt_opts);
    if (!stmt.is_valid()) {
        std::cout << "statement alloc error: " << stmt.last_error() << std::endl;
        diag_printer(stmt.diagnostics());
        return 1;
    }

    // prepare a select statement
    if (!stmt.prepare(test_secrets::generic_unfiltered_select)) {
        std::cout << "prepare error: " << stmt.last_error() << std::endl;
        diag_printer(stmt.diagnostics());
        return 1;
    }

    // execute the select statement
    if (!stmt.execute()) {
        std::cout << "execute error: " << stmt.last_error() << std::endl;
        diag_printer(stmt.diagnostics());
        return 1;
    }

    // bind the select query columns for the first (and only) result set
    simql::statement::sql_column_string first_name(0, 1024, false);
    simql::statement::sql_column_string last_name(1, 1024, false);
    simql::statement::sql_column_datetime birth_date(2);
    if (!stmt.define_columns<simql::statement::sql_column_string, simql::statement::sql_column_string, simql::statement::sql_column_datetime>(first_name, last_name, birth_date)) {
        std::cout << "column binding error: " << stmt.last_error() << std::endl;
        diag_printer(stmt.diagnostics());
        return 1;
    }

    // access the result set from the beginning
    if (!stmt.first_record()) {
        std::cout << "retrieve error: " << stmt.last_error() << std::endl;
        diag_printer(stmt.diagnostics());
        return 1;
    }

    // the bound columns should now be populated
    std::cout << "first name: " << first_name.data() << std::endl;
    std::cout << "last name: " << last_name.data() << std::endl;
    std::cout << "birth date: " << birth_date.data().to_string() << std::endl;

    std::cout << "end of test" << std::endl;
    return 0;
}
