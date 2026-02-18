#ifndef statement_header_h
#define statement_header_h

// SimQL stuff
#include "database_connection.hpp"
#include "simql_types.hpp"

// STL stuff
#include <cstdint>
#include <memory>
#include <unordered_map>

namespace simql {
    class diagnostic_set;
    class result_set;
    class value_set;
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
        bool prepare(std::string_view sql);
        bool execute();
        bool execute_direct(std::string_view sql);

        /* data retrieval */
        result_set* results();
        value_set* values();
        bool next_result_set();
        bool next_value_set();

        /* parameter binding */
        bool bind_string(std::string name, std::string value, simql_types::parameter_binding_type binding_type, bool set_null);
        bool bind_floating_point(std::string name, double value, simql_types::parameter_binding_type binding_type, bool set_null);
        bool bind_boolean(std::string name, bool value, simql_types::parameter_binding_type binding_type, bool set_null);
        bool bind_integer(std::string name, int value, simql_types::parameter_binding_type binding_type, bool set_null);
        bool bind_guid(std::string name, simql_types::guid_struct value, simql_types::parameter_binding_type binding_type, bool set_null);
        bool bind_datetime(std::string name, simql_types::datetime_struct value, simql_types::parameter_binding_type binding_type, bool set_null);
        bool bind_date(std::string name, simql_types::date_struct value, simql_types::parameter_binding_type binding_type, bool set_null);
        bool bind_time(std::string name, simql_types::time_struct value, simql_types::parameter_binding_type binding_type, bool set_null);
        bool bind_blob(std::string name, std::vector<std::uint8_t> value, simql_types::parameter_binding_type binding_type, bool set_null);

        /* process transparency */
        bool is_valid();
        diagnostic_set* diagnostics();
        std::string_view last_error();

    private:
        friend class statement_pool;
        statement(void* raw_stmt_handle, database_connection& conn, void* pool);
        void* detach_handle() noexcept;
        struct handle;
        std::unique_ptr<handle> sp_handle;
    };
}

#endif