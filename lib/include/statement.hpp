#ifndef statement_header_h
#define statement_header_h

// SimQL stuff
#include <database_connection.hpp>
#include <simql_returncodes.hpp>
#include <simql_types.hpp>

// STL stuff
#include <cstdint>
#include <memory>
#include <unordered_map>

namespace simql {
    class statement {
    public:

        /* enums */
        enum class cursor_type : std::uint8_t {
            forward_only,
            static_cursor,
            dyanmic_cursor,
            keyset_driven
        };

        /* structs */
        struct alloc_options {
            cursor_type cursor = cursor_type::forward_only;
            std::uint32_t query_timeout = 0;
            std::uint64_t max_rows = 0;
        };

        struct value_pair {
            std::string name;
            simql_types::sql_value value;
        };

        /* constructor/destructor */
        explicit statement(database_connection& dbc, const alloc_options& options);
        ~statement();

        /* set move assignment */
        statement(statement&&) noexcept;
        statement& operator=(statement&&) noexcept;

        /* set copy assignment */
        statement(const statement&) = delete;
        statement& operator=(const statement&) = delete;

        /* generic */
        simql_returncodes::code prepare(std::string_view sql);
        simql_returncodes::code execute();
        simql_returncodes::code execute_direct(std::string_view sql);

        /* data retrieval */
        simql_returncodes::code get_result_set(std::vector<simql_types::sql_value>& results, std::vector<simql_types::sql_column>& columns, std::uint64_t& row_count, std::uint8_t& skipped_columns, std::uint64_t& skipped_rows);
        simql_returncodes::code get_value_set(std::vector<value_pair>& value_pairs);
        bool next_result_set();
        bool next_value_set();

        /* parameter binding */
        simql_returncodes::code bind_string(std::string name, std::string value, simql_types::parameter_binding_type binding_type, bool set_null);
        simql_returncodes::code bind_floating_point(std::string name, double value, simql_types::parameter_binding_type binding_type, bool set_null);
        simql_returncodes::code bind_boolean(std::string name, bool value, simql_types::parameter_binding_type binding_type, bool set_null);
        simql_returncodes::code bind_integer(std::string name, int value, simql_types::parameter_binding_type binding_type, bool set_null);
        simql_returncodes::code bind_guid(std::string name, simql_types::guid_struct value, simql_types::parameter_binding_type binding_type, bool set_null);
        simql_returncodes::code bind_datetime(std::string name, simql_types::datetime_struct value, simql_types::parameter_binding_type binding_type, bool set_null);
        simql_returncodes::code bind_date(std::string name, simql_types::date_struct value, simql_types::parameter_binding_type binding_type, bool set_null);
        simql_returncodes::code bind_time(std::string name, simql_types::time_struct value, simql_types::parameter_binding_type binding_type, bool set_null);
        simql_returncodes::code bind_blob(std::string name, std::vector<std::uint8_t> value, simql_types::parameter_binding_type binding_type, bool set_null);

        /* process transparency */
        const simql_returncodes::code& return_code();

    private:
        friend class statement_pool;
        statement(void* raw_stmt_handle, database_connection& conn, void* pool);
        void* detach_handle() noexcept;
        struct handle;
        std::unique_ptr<handle> sp_handle;
    };
}

#endif