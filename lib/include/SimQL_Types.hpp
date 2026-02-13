#ifndef simql_types_header_h
#define simql_types_header_h

// STL stuff
#include <cstdint>
#include <format>
#include <string>
#include <variant>
#include <vector>

namespace simql_types {

    /* ENUMS */

    enum class null_rule_type : std::uint8_t {
        UNKNOWN,
        ALLOWED,
        NOT_ALLOWED
    };

    enum class parameter_binding_type : std::uint8_t {
        INPUT_OUTPUT,
        INPUT,
        OUTPUT
    };

    enum class sql_dtype : std::uint8_t {
        STRING,
        FLOATING_POINT,
        BOOLEAN,
        INTEGER,
        GUID,
        DATETIME,
        DATE,
        TIME,
        BLOB
    };
    constexpr std::uint8_t operator^(sql_dtype l, sql_dtype r) {
        return static_cast<std::uint8_t>(l) ^ static_cast<std::uint8_t>(r);
    }

    /* STRUCTS */

    struct datetime_struct {
        std::int16_t year;
        std::uint16_t month;
        std::uint16_t day;
        std::uint16_t hour;
        std::uint16_t minute;
        std::uint16_t second;
        std::uint32_t fraction;
        std::string to_string() { return std::format("{:04}-{:02}-{:02} {:02}:{:02}:{:02}.{:03}", year, month, day, hour, minute, second, fraction / 1000000); }
        datetime_struct(std::int16_t year, std::uint16_t month, std::uint16_t day, std::uint16_t hour, std::uint16_t minute, std::uint16_t second, std::uint32_t fraction = 0) {
            this->year = year;
            this->month = month;
            this->day = day;
            this->hour = hour;
            this->minute = minute;
            this->second = second;
            this->fraction = fraction;
        }
        datetime_struct() : year(0), month(0), day(0), hour(0), minute(0), second(0), fraction(0) {}
    };

    struct date_struct {
        std::int16_t year;
        std::uint16_t month;
        std::uint16_t day;
        std::string to_string() { return std::format("{:04}-{:02}-{:02}", year, month, day); }
        date_struct(std::int16_t year, std::uint16_t month, std::uint16_t day) {
            this->year = year;
            this->month = month;
            this->day = day;
        }
        date_struct() : year(0), month(0), day(0) {}
    };

    struct time_struct {
        std::uint16_t hour;
        std::uint16_t minute;
        std::uint16_t second;
        std::string to_string() { return std::format("{:02}:{:02}:{:02}", hour, minute, second); }
        time_struct(std::uint16_t hour, std::uint16_t minute, std::uint16_t second) {
            this->hour = hour;
            this->minute = minute;
            this->second = second;
        }
        time_struct() : hour(0), minute(0), second(0) {}
    };

    struct guid_struct {
        std::uint32_t time_low;
        std::uint16_t time_mid;
        std::uint16_t time_high;
        std::uint8_t clock_seq_node[8];
    };

    using sql_variant = std::variant<
        std::string,
        double,
        bool,
        int,
        guid_struct,
        datetime_struct,
        date_struct,
        time_struct,
        std::vector<std::uint8_t>
    >;

    struct sql_value {
        sql_variant data;
        sql_dtype data_type;
        bool is_null;
    };

    struct sql_column {
        std::string name;
        sql_dtype data_type;
        std::uint64_t size;
        std::int16_t precision;
        null_rule_type null_type;
    };
}

#endif
