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
    private:

        struct sql_column_base {
            std::uint8_t position{};
            std::string name{};
            simql_types::sql_val value{};
        };

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

        struct sql_column_string : sql_column_base {
            std::uint32_t max_character_count{};
            bool is_wide_string{false};
            std::string data() { return value.to_string(); }
        };
        
        struct sql_column_double : sql_column_base {
            double data() { return value.to_double(); }
        };

        struct sql_column_float : sql_column_base {
            float data() { return value.to_float(); }
        };

        struct sql_column_int8 : sql_column_base {
            std::int8_t data() { return value.to_int8(); }
        };

        struct sql_column_int16 : sql_column_base {
            std::int16_t data() { return value.to_int16(); }
        };

        struct sql_column_int32 : sql_column_base {
            std::int32_t data() { return value.to_int32(); }
        };

        struct sql_column_int64 : sql_column_base {
            std::int64_t data() { return value.to_int64(); }
        };

        struct sql_column_guid : sql_column_base {
            simql_types::guid_struct data() { return value.to_guid(); }
        };

        struct sql_column_datetime : sql_column_base {
            simql_types::datetime_struct data() { return value.to_datetime(); }
        };

        struct sql_column_date : sql_column_base {
            simql_types::date_struct data() { return value.to_date(); }
        };

        struct sql_column_time : sql_column_base {
            simql_types::time_struct data() { return value.to_time(); }
        };

        struct sql_column_blob : sql_column_base {
            std::uint32_t max_byte_count{};
            std::vector<std::uint8_t> data() { return value.to_blob(); }
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
        // COLUMN BINDING
        // --------------------------------------------------

        template<typename T>
        bool set_columns(T column);

        template<typename T, typename... args>
        bool set_columns(T first_column, args... other_columns);

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
