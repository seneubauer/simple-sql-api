#ifndef statement_header_h
#define statement_header_h

// SimQL stuff
#include "database_connection.hpp"
#include "simql_types.hpp"

// STL stuff
#include <cstdint>
#include <memory>

namespace simql {
    class diagnostic_set;
    class statement {
    public:

        enum class cursor_sensitivity : std::uint8_t {
            unspecified,
            insensitive,
            sensitive
        };

        struct alloc_options {
            std::uint32_t query_timeout{0};
            std::uint64_t max_rows{0};
            std::uint32_t rowset_size{1000};
            bool is_scrollable{false};
            cursor_sensitivity sensitivity{cursor_sensitivity::unspecified};
        };

        struct column_value {
            std::string name{};
            simql_types::sql_val data{};

            std::uint8_t column_number;
            std::uint32_t column_size;
            std::uint16_t precision;

            // data type hint flags to assist with data binding
            bool is_wide_string{false};
        };

        // --------------------------------------------------
        // LIFECYCLE
        // --------------------------------------------------

        explicit statement(database_connection& dbc, const alloc_options& options);
        ~statement();
        statement(statement&&) noexcept;
        statement& operator=(statement&&) noexcept;
        statement(const statement&) = delete;
        statement& operator=(const statement&) = delete;

        // --------------------------------------------------
        // EXECUTION
        // --------------------------------------------------

        template<typename T, typename...args>
        bool prepare(std::string_view sql, T first, args... rest);
        bool prepare(std::string_view sql);
        bool execute();
        bool execute_direct(std::string_view sql);
        bool open_cursor();

        // --------------------------------------------------
        // RESULT NAVIGATION
        // --------------------------------------------------

        bool first_record();
        bool last_record();
        bool prev_record();
        bool next_record();
        bool next_result_set();
        bool goto_bound_parameters();

        // --------------------------------------------------
        // DATA RETRIEVAL
        // --------------------------------------------------

        bool parameter_value(const std::string& name, simql_types::sql_value& value);
        bool current_record(std::vector<simql_types::sql_value>& values);
        std::int64_t rows_affected();

        // --------------------------------------------------
        // PARAMETER BINDING
        // --------------------------------------------------

        bool bind_string(std::string name, std::string value, simql_types::parameter_binding_type binding_type, bool set_null);
        bool bind_floating_point(std::string name, double value, simql_types::parameter_binding_type binding_type, bool set_null);
        bool bind_boolean(std::string name, bool value, simql_types::parameter_binding_type binding_type, bool set_null);
        bool bind_integer(std::string name, int value, simql_types::parameter_binding_type binding_type, bool set_null);
        bool bind_guid(std::string name, simql_types::guid_struct value, simql_types::parameter_binding_type binding_type, bool set_null);
        bool bind_datetime(std::string name, simql_types::datetime_struct value, simql_types::parameter_binding_type binding_type, bool set_null);
        bool bind_date(std::string name, simql_types::date_struct value, simql_types::parameter_binding_type binding_type, bool set_null);
        bool bind_time(std::string name, simql_types::time_struct value, simql_types::parameter_binding_type binding_type, bool set_null);
        bool bind_blob(std::string name, std::vector<std::uint8_t> value, simql_types::parameter_binding_type binding_type, bool set_null);
        
        // --------------------------------------------------
        // DIAGNOSTICS
        // --------------------------------------------------

        bool is_valid();
        std::string_view last_error();
        diagnostic_set* diagnostics();

    private:
        friend class statement_pool;
        statement(void* raw_stmt_handle, database_connection& conn, void* pool);
        void* detach_handle() noexcept;
        struct handle;
        std::unique_ptr<handle> p_handle;
    };
}

#endif
