#ifndef SimQL_Types_header_h
#define SimQL_Types_header_h

// STL stuff
#include <cstdint>
#include <format>
#include <string>
#include <cstdlib>
#include <variant>
#include <vector>
#include <array>
#include <memory>
#include <cstdlib>

namespace SimpleSqlTypes {

    /* HANDLES */

    struct HandleDeleter { void operator()(void* p) const { std::free(p); } };
    using STMT_HANDLE = std::unique_ptr<void, HandleDeleter>;
    using DBC_HANDLE = std::unique_ptr<void, HandleDeleter>;
    using ENV_HANDLE = std::unique_ptr<void, HandleDeleter>;

    /* ENUMS */

    enum class ConnectionPoolingType : std::uint8_t {
        OFF                 = 0,
        ONE_PER_DRIVER      = 1,
        ONE_PER_ENV         = 2
    };

    enum class AccessModeType : std::uint8_t {
        READ_ONLY           = 0,
        READ_WRITE          = 1
    };

    enum class AsyncModeType : std::uint8_t {
        ENABLED             = 0,
        DISABLED            = 1
    };

    enum class AutocommitType : std::uint8_t {
        ENABLED             = 0,
        DISABLED            = 1
    };

    enum class DatabaseType : std::uint8_t {
        SQL_SERVER          = 0,
        POSTGRESQL          = 1
    };

    enum class NullRuleType : std::uint8_t {
        UNKNOWN             = 0,
        ALLOWED             = 1,
        NOT_ALLOWED         = 2
    };

    enum class BindingType : std::uint8_t {
        INPUT_OUTPUT        = 0,
        INPUT               = 1,
        OUTPUT              = 2
    };

    enum class SimDataType : std::uint8_t {
        UNKNOWN             = 0,
        STRING_UTF8         = 1,
        STRING_UTF16        = 2,
        LONG_STRING_UTF8    = 3,
        LONG_STRING_UTF16   = 4,
        FLOAT               = 5,
        DOUBLE              = 6,
        BOOLEAN             = 7,
        INT_8               = 8,
        INT_16              = 9,
        INT_32              = 10,
        INT_64              = 11,
        ODBC_GUID           = 12,
        GUID                = 13,
        DATETIME            = 14,
        DATE                = 15,
        TIME                = 16,
        BLOB                = 17,
        LONG_BLOB           = 18
    };
    constexpr std::uint8_t operator^(SimDataType l, SimDataType r) {
        return static_cast<std::uint8_t>(l) ^ static_cast<std::uint8_t>(r);
    }

    /* STRUCTS */

    struct ColumnMetadata {
    private:
        std::uint16_t m_position;
        std::string m_name;
        SimDataType m_sim_data_type;
        std::uint64_t m_data_size;
        std::uint16_t m_precision;
        NullRuleType m_null_rule;
    public:
        ColumnMetadata(
            const std::uint16_t &position,
            const std::string &name,
            const SimDataType &sim_data_type,
            const std::uint64_t &data_size,
            const std::uint16_t &precision,
            const NullRuleType &null_rule
        ) : m_position(position), m_name(name), m_sim_data_type(sim_data_type), m_data_size(data_size), m_precision(precision), m_null_rule(null_rule) {}
        std::uint16_t position() const { return m_position; }
        std::string name() const { return m_name; }
        SimDataType sim_data_type() const { return m_sim_data_type; }
        std::uint64_t data_size() const { return m_data_size; }
        std::uint16_t precision() const { return m_precision; }
        NullRuleType null_rule() const { return m_null_rule; }
    };

    struct DiagnosticRecord {
    private:
        std::int16_t m_record_number;
        std::string m_sql_state;
        std::int32_t m_native_error;
        std::string m_message;
    public:
        DiagnosticRecord(
            const std::int16_t &record_number,
            const std::string &sql_state,
            const std::int32_t &native_error,
            const std::string &message
        ) : m_record_number(record_number), m_sql_state(sql_state), m_native_error(native_error), m_message(message) {}
        std::int16_t record_number() const { return m_record_number; }
        std::string sql_state() const { return m_sql_state; }
        std::int32_t native_error() const { return m_native_error; }
        std::string message() const { return m_message; }
    };

    struct _Datetime {
        std::int16_t year;
        std::uint16_t month;
        std::uint16_t day;
        std::uint16_t hour;
        std::uint16_t minute;
        std::uint16_t second;
        std::uint32_t fraction;
    };

    struct _Date {
        std::int16_t year;
        std::uint16_t month;
        std::uint16_t day;
    };

    struct _Time {
        std::uint16_t hour;
        std::uint16_t minute;
        std::uint16_t second;
    };

    template<typename T>
    concept temporal_types = requires(T& t) {
        std::is_same_v<T, _Datetime> ||
        std::is_same_v<T, _Date> ||
        std::is_same_v<T, _Time>;
    };

    template<temporal_types T>
    struct BaseTemporal {
    private:
        T m_temporal;
        bool m_is_utc;
    public:
        BaseTemporal(T&& t, const bool &is_utc) : m_temporal(std::move(t)), m_is_utc(is_utc) {}
        T& temporal() { return m_temporal; }
        bool is_utc() const { return m_is_utc; }
    };

    struct Datetime : BaseTemporal<_Datetime> {
        std::string to_string() { return std::format("{:04}-{:02}-{:02} {:02}:{:02}:{:02}.{:03}", temporal().year, temporal().month, temporal().day, temporal().hour, temporal().minute, temporal().second, temporal().fraction / 1000000); }
    };

    struct Date : BaseTemporal<_Date> {
        std::string to_string() { return std::format("{:04}-{:02}-{:02}", temporal().year, temporal().month, temporal().day); }
    };

    struct Time : BaseTemporal<_Time> {
        std::string to_string() { return std::format("{:02}:{:02}:{:02}", temporal().hour, temporal().minute, temporal().second); }
    };

    struct ODBC_GUID {
        std::uint32_t Data1;
        std::uint16_t Data2;
        std::uint16_t Data3;
        std::uint8_t Data4[8];
    };

    using GUID = std::array<std::uint8_t, 16>;

    using SQLData = std::variant<
        std::string,
        std::wstring,
        float,
        double,
        bool,
        std::int8_t,
        std::int16_t,
        std::int32_t,
        std::int64_t,
        SimpleSqlTypes::ODBC_GUID,
        SimpleSqlTypes::GUID,
        SimpleSqlTypes::Datetime,
        SimpleSqlTypes::Date,
        SimpleSqlTypes::Time,
        std::vector<std::uint8_t>>;

    struct SQLBinding {
    private:
        std::string m_name;
        SQLData m_data;
        BindingType m_type;
        SimDataType m_data_type;
        bool m_set_null;
    public:
        SQLBinding(
            const std::string &name,
            const SQLData &data,
            const BindingType &type,
            const SimDataType &data_type,
            const bool &set_null = false
        ) : m_name(name), m_data(data), m_type(type), m_data_type(data_type), m_set_null(set_null) {}
        ~SQLBinding() {}
        const std::string& name() const { return m_name; }
        SQLData& data() { return m_data; }
        const BindingType& type() const { return m_type; }
        const SimDataType& data_type() const { return m_data_type; }
        const bool& set_null() const { return m_set_null; } 
    };

    struct SQLBoundOutput {
    private:
        SQLBinding m_binding;
        std::int64_t m_buffer_size;
    public:
        SQLBoundOutput(
            const SimpleSqlTypes::SQLBinding &binding,
            const size_t &buffer_size
        ) : m_binding(binding), m_buffer_size(buffer_size) {}
        ~SQLBoundOutput() {}
        SQLBinding& binding() { return m_binding; }
        std::int64_t& buffer_size() { return m_buffer_size; }
    };

    struct SQLCell {
    private:
        SQLData m_data;
        SimDataType m_data_type;
        bool m_is_null;
        bool m_is_valid;
    public:
        SQLCell() : m_is_valid(false) {}
        SQLCell(
            const SQLData &data,
            const SimDataType &data_type,
            const bool &is_null = false
        ) : m_data(data), m_data_type(data_type), m_is_null(is_null), m_is_valid(true) {}
        ~SQLCell() {}
        const SQLData& data() const { return m_data; }
        const SimDataType& data_type() const { return m_data_type; }
        const bool& is_null() const { return m_is_null; }
        const bool& is_valid() const { return m_is_valid; }
    };

    struct SQLMatrix {
    private:
        std::vector<SQLCell> m_cells;
        size_t m_rows;
        size_t m_columns;
        bool m_is_valid;
    public:
        SQLMatrix() : m_is_valid(false) {}
        SQLMatrix(
            std::vector<SQLCell>&& cells,
            const size_t &rows,
            const size_t &columns
        ) : m_cells(std::move(cells)), m_rows(rows), m_columns(columns), m_is_valid(true) {}
        ~SQLMatrix() {}
        const std::vector<SQLCell>& cells() const { return m_cells; }
        const size_t& rows() const { return m_rows; }
        const size_t& columns() const { return m_columns; }
        const bool& is_valid() const { return m_is_valid; }
        std::vector<SQLCell> claim_cells() const {
            return std::move(m_cells);
        }

        void set_data(std::vector<SQLCell>&& cells, const size_t &rows) {
            m_cells = cells;
            m_rows = rows;
        }

        void make_valid(const size_t &columns) {
            m_columns = columns;
            m_is_valid = true;
        }

        void add_row(std::vector<SQLCell>&& row) {
            m_cells.resize(m_cells.size() + row.size());
            m_cells.insert(m_cells.end(), std::make_move_iterator(row.begin()), std::make_move_iterator(row.end()));
        }
    };
}

#endif
