#ifndef statement_header_h
#define statement_header_h

// SimQL stuff
#include "database_connection.hpp"
#include "simql_types.hpp"

// STL stuff
#include <cstdint>
#include <memory>
#include <concepts>
#include <type_traits>

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

        struct sql_column {
            std::uint8_t position{};
            std::string name{};
            simql_types::sql_val value{};
        };

        struct sql_column_string : sql_column {
            std::uint32_t max_character_count{};
            bool is_wide{false};
            std::string data() { return value.get<std::string>(); }
        };

        struct sql_column_character : sql_column {
            bool is_wide{false};
            char data() { return value.get<char>(); }
        };

        struct sql_column_boolean : sql_column {
            bool data() { return value.get<bool>(); }
        };

        struct sql_column_double : sql_column {
            double data() { return value.get<double>(); }
        };

        struct sql_column_float : sql_column {
            float data() { return value.get<float>(); }
        };

        struct sql_column_int8 : sql_column {
            std::int8_t data() { return value.get<std::int8_t>(); }
        };

        struct sql_column_int16 : sql_column {
            std::int16_t data() { return value.get<std::int16_t>(); }
        };

        struct sql_column_int32 : sql_column {
            std::int32_t data() { return value.get<std::int32_t>(); }
        };

        struct sql_column_int64 : sql_column {
            std::int64_t data() { return value.get<std::int64_t>(); }
        };

        struct sql_column_guid : sql_column {
            simql_types::guid_struct data() { return value.get<simql_types::guid_struct>(); }
        };

        struct sql_column_datetime : sql_column {
            simql_types::datetime_struct data() { return value.get<simql_types::datetime_struct>(); }
        };

        struct sql_column_date : sql_column {
            simql_types::date_struct data() { return value.get<simql_types::date_struct>(); }
        };

        struct sql_column_time : sql_column {
            simql_types::time_struct data() { return value.get<simql_types::time_struct>(); }
        };

        struct sql_column_blob : sql_column {
            std::uint32_t max_byte_count{};
            std::vector<std::uint8_t> data() { return value.get<std::vector<std::uint8_t>>(); }
        };

        struct sql_parameter {
            std::uint8_t position{};
            std::string name{};
            simql_types::parameter_binding_type binding_type{};
            simql_types::sql_val value{};
        };

        struct sql_parameter_string : sql_parameter {
            std::uint32_t max_character_count{};
            bool is_wide{false};
            bool variadic_characters{true};
            std::string data() { return value.get<std::string>(); }
        };

        struct sql_parameter_character : sql_parameter {
            bool is_wide{false};
            char data() { return value.get<char>(); }
        };

        struct sql_parameter_boolean : sql_parameter {
            bool data() { return value.get<bool>(); }
        };

        struct sql_parameter_double : sql_parameter {
            double data() { return value.get<double>(); }
        };

        struct sql_parameter_float : sql_parameter {
            float data() { return value.get<float>(); }
        };

        struct sql_parameter_int8 : sql_parameter {
            std::int8_t data() { return value.get<std::int8_t>(); }
        };

        struct sql_parameter_int16 : sql_parameter {
            std::int16_t data() { return value.get<std::int16_t>(); }
        };

        struct sql_parameter_int32 : sql_parameter {
            std::int32_t data() { return value.get<std::int32_t>(); }
        };

        struct sql_parameter_int64 : sql_parameter {
            std::int64_t data() { return value.get<std::int64_t>(); }
        };

        struct sql_parameter_guid : sql_parameter {
            simql_types::guid_struct data() { return value.get<simql_types::guid_struct>(); }
        };

        struct sql_parameter_datetime : sql_parameter {
            simql_types::datetime_struct data() { return value.get<simql_types::datetime_struct>(); }
        };

        struct sql_parameter_date : sql_parameter {
            simql_types::date_struct data() { return value.get<simql_types::date_struct>(); }
        };

        struct sql_parameter_time : sql_parameter {
            simql_types::time_struct data() { return value.get<simql_types::time_struct>(); }
        };

        struct sql_parameter_blob : sql_parameter {
            std::uint32_t max_byte_count{};
            std::vector<std::uint8_t> data() { return value.get<std::vector<std::uint8_t>>(); }
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

        std::int64_t rows_affected();

        // --------------------------------------------------
        // COLUMN BINDING
        // --------------------------------------------------

        template<typename T> requires std::derived_from<std::remove_cvref_t<T>, sql_column>
        bool bind_columns(T& column);

        template<typename T, typename... args> requires std::derived_from<std::remove_cvref_t<T>, sql_column> && (std::derived_from<std::remove_cvref_t<args>, sql_column> && ...)
        bool bind_columns(T& first_column, args&... other_columns);

        // --------------------------------------------------
        // PARAMETER BINDING
        // --------------------------------------------------

        template<typename T> requires std::derived_from<std::remove_cvref_t<T>, sql_parameter>
        bool bind_parameters(T&& parameter);

        template<typename T, typename... args> requires std::derived_from<std::remove_cvref_t<T>, sql_parameter> && (std::derived_from<std::remove_cvref_t<args>, sql_parameter> && ...)
        bool bind_parameters(T&& first_parameter, args&&... other_parameters);

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
