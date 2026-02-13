// SimQL stuff
#include <connection_string_builder.hpp>
#include <simql_constants.hpp>
#include <environment.hpp>
#include <simql_returncodes.hpp>

// STL stuff
#include <string>
#include <cstdint>
#include <iostream>

// secrets
#include <test_secrets.hpp>

int main() {

    simql::connection_string_builder builder;
    switch (test_secrets::current_os) {
    case test_secrets::operating_system::windows:
        builder.set_driver(std::string(simql_constants::database_drivers::odbc_17_sql_server));
        builder.set_trusted(true);
        break;
    case test_secrets::operating_system::linux:
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

    // helper to print environment diagnostics
    auto env_printer = [&](simql::environment& e) {
        auto diag = e.diagnostics();
        if (diag) {
            for (auto& element : diag->view_diagnostics()) {
                std::cout << element.message << std::endl;
            }
        }
    };

    // allocate the environment handle
    simql::environment env(env_opts);
    switch (env.return_code()) {
    case simql_returncodes::code::success:
        break;
    case simql_returncodes::code::success_info:
        std::cout << "environment info..." << std::endl;
        env_printer(env);
        break;
    default:
        std::cout << "environment error..." << std::endl;
        env_printer(env);
        return 1;
    }

    return 0;
}
