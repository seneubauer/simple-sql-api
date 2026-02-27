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
        unknown,
        allowed,
        not_allowed
    };

    enum class parameter_binding_type : std::uint8_t {
        input_output,
        input,
        output
    };

    enum class sql_dtype : std::uint8_t {
        string,
        floating_point,
        boolean,
        integer,
        guid,
        datetime,
        date,
        time,
        blob
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
        bool,
        double,
        int,
        guid_struct,
        datetime_struct,
        date_struct,
        time_struct,
        std::vector<std::uint8_t>
    >;

    template<typename T>
    concept sql_variant_type = 
        std::is_same_v<T, std::monostate> ||
        std::is_same_v<T, std::string> ||
        std::is_same_v<T, bool> ||
        std::is_same_v<T, double> ||
        std::is_same_v<T, float> ||
        std::is_same_v<T, std::int8_t> ||
        std::is_same_v<T, std::int16_t> ||
        std::is_same_v<T, std::int32_t> ||
        std::is_same_v<T, std::int64_t> ||
        std::is_same_v<T, guid_struct> ||
        std::is_same_v<T, datetime_struct> ||
        std::is_same_v<T, date_struct> ||
        std::is_same_v<T, time_struct> ||
        std::is_same_v<T, std::vector<std::uint8_t>>;

    struct sql_val {
    private:

        using sql_val_variant = std::variant<
            std::monostate,
            std::string,
            bool,
            double,
            float,
            std::int8_t,
            std::int16_t,
            std::int32_t,
            std::int64_t,
            guid_struct,
            datetime_struct,
            date_struct,
            time_struct,
            std::vector<std::uint8_t>
        >;
        sql_val_variant data;

    public:

        sql_val() : data() {}

        template<sql_variant_type T>
        sql_val(T value) : data(value) {}

        constexpr bool is_null() { return std::holds_alternative<std::monostate>(data); }
        void set_null() { data = std::monostate{}; }

        template<sql_variant_type T>
        void set(T value) { data = value; }

        template<sql_variant_type T>
        T get() {
            auto visitor = [&](sql_val_variant& d) -> T {
                if constexpr (std::is_same_v<std::decay_t<decltype(d)>, T)
                    return std::get<T>(d);

                return T{};
            };
            return std::visit<T>(visitor, data);
        }

    };

}

#endif
