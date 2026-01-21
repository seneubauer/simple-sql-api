#ifndef SimQL_Types_header_h
#define SimQL_Types_header_h

// STL stuff
#include <cstdint>
#include <format>
#include <string>
#include <string_view>
#include <cstdlib>
#include <variant>
#include <vector>
#include <array>
#include <span>
#include <concepts>

namespace SimpleSqlTypes {

    /* ENUMS */

    enum class NullRuleType : uint8_t {
        NOT_SET             = 0,
        UNKNOWN             = 1,
        ALLOWED             = 2,
        NOT_ALLOWED         = 3
    };

    enum class BindingType : uint8_t {
        INPUT_OUTPUT        = 0,
        INPUT               = 1,
        OUTPUT              = 2
    };

    enum class SimDataType : uint8_t {
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
    constexpr uint8_t operator^(SimDataType l, SimDataType r) {
        return static_cast<uint8_t>(l) ^ static_cast<uint8_t>(r);
    }

    /* STRUCTS */

    struct ColumnMetadata {
    private:
        uint16_t m_position;
        std::string m_name;
        SimDataType m_sim_data_type;
        uint64_t m_data_size;
        uint16_t m_precision;
        NullRuleType m_null_rule;
    public:
        ColumnMetadata(
            const uint16_t &position,
            const std::string &name,
            const SimDataType &sim_data_type,
            const uint64_t &data_size,
            const uint16_t &precision,
            const NullRuleType &null_rule
        ) : m_position(position), m_name(name), m_sim_data_type(sim_data_type), m_data_size(data_size), m_precision(precision), m_null_rule(null_rule) {}
        uint16_t position() const { return m_position; }
        std::string name() const { return m_name; }
        SimDataType sim_data_type() const { return m_sim_data_type; }
        uint64_t data_size() const { return m_data_size; }
        uint16_t precision() const { return m_precision; }
        NullRuleType null_rule() const { return m_null_rule; }
    };

    struct DiagnosticRecord {
    private:
        int16_t m_record_number;
        std::string m_sql_state;
        int32_t m_native_error;
        std::string m_message;
    public:
        DiagnosticRecord(
            const int16_t &record_number,
            const std::string &sql_state,
            const int32_t &native_error,
            const std::string &message
        ) : m_record_number(record_number), m_sql_state(sql_state), m_native_error(native_error), m_message(message) {}
        int16_t record_number() const { return m_record_number; }
        std::string sql_state() const { return m_sql_state; }
        int32_t native_error() const { return m_native_error; }
        std::string message() const { return m_message; }
    };

    struct _Datetime {
        int16_t year;
        uint16_t month;
        uint16_t day;
        uint16_t hour;
        uint16_t minute;
        uint16_t second;
        uint32_t fraction;
    };

    struct _Date {
        int16_t year;
        uint16_t month;
        uint16_t day;
    };

    struct _Time {
        uint16_t hour;
        uint16_t minute;
        uint16_t second;
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
        const bool is_utc() const { return m_is_utc; }
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
        uint32_t Data1;
        uint16_t Data2;
        uint16_t Data3;
        uint8_t Data4[8];
    };

    using GUID = std::array<uint8_t, 16>;

    using SQLData = std::variant<
        std::string,
        std::wstring,
        float,
        double,
        bool,
        int8_t,
        int16_t,
        int32_t,
        int64_t,
        ODBC_GUID,
        GUID,
        Datetime,
        Date,
        Time,
        std::vector<uint8_t>>;

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
        const std::string& name() const { return m_name; }
        const SQLData& data() const { return m_data; }
        const BindingType& type() const { return m_type; }
        const SimDataType& data_type() const { return m_data_type; }
        const bool& set_null() const { return m_set_null; } 
    };

    struct SQLBoundOutput {
    private:
        SQLBinding m_binding;
        size_t m_buffer_size;
    public:
        SQLBoundOutput(
            const SimpleSqlTypes::SQLBinding &binding,
            const size_t &buffer_size
        ) : m_binding(binding), m_buffer_size(buffer_size) {}
        SQLBinding& binding() { return m_binding; }
        size_t& buffer_size() { return m_buffer_size; }
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