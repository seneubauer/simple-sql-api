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
        double,
        bool,
        int,
        guid_struct,
        datetime_struct,
        date_struct,
        time_struct,
        std::vector<std::uint8_t>
    >;

    struct sql_val {
    private:
        using sql_val_variant = std::variant<
            std::monostate,
            std::string,
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

        constexpr bool is_null() {
            return std::holds_alternative<std::monostate>(data);
        }

        constexpr bool is_string() {
            return std::is_same_v<std::decay_t<decltype(data)>, std::string>;
        }

        constexpr bool is_double() {
            return std::is_same_v<std::decay_t<decltype(data)>, double>;
        }

        constexpr bool is_float() {
            return std::is_same_v<std::decay_t<decltype(data)>, float>;
        }

        constexpr bool is_int8() {
            return std::is_same_v<std::decay_t<decltype(data)>, std::int8_t>;
        }

        constexpr bool is_int16() {
            return std::is_same_v<std::decay_t<decltype(data)>, std::int16_t>;
        }

        constexpr bool is_int32() {
            return std::is_same_v<std::decay_t<decltype(data)>, std::int32_t>;
        }

        constexpr bool is_int64() {
            return std::is_same_v<std::decay_t<decltype(data)>, std::int64_t>;
        }

        constexpr bool is_guid() {
            return std::is_same_v<std::decay_t<decltype(data)>, guid_struct>;
        }

        constexpr bool is_datetime() {
            return std::is_same_v<std::decay_t<decltype(data)>, datetime_struct>;
        }

        constexpr bool is_date() {
            return std::is_same_v<std::decay_t<decltype(data)>, date_struct>;
        }

        constexpr bool is_time() {
            return std::is_same_v<std::decay_t<decltype(data)>, time_struct>;
        }

        constexpr bool is_blob() {
            return std::is_same_v<std::decay_t<decltype(data)>, std::vector<std::uint8_t>>;
        }

        void set_null() {
            data = std::monostate{};
        }

        void set_value(std::string value) {
            data = value;
        }

        void set_value(double value) {
            data = value;
        }

        void set_value(float value) {
            data = value;
        }

        void set_value(std::int8_t value) {
            data = value;
        }

        void set_value(std::int16_t value) {
            data = value;
        }

        void set_value(std::int32_t value) {
            data = value;
        }

        void set_value(std::int64_t value) {
            data = value;
        }

        void set_value(guid_struct value) {
            data = value;
        }

        void set_value(datetime_struct value) {
            data = value;
        }

        void set_value(date_struct value) {
            data = value;
        }

        void set_value(time_struct value) {
            data = value;
        }

        void set_value(std::vector<std::uint8_t> value) {
            data = value;
        }

        std::string to_string() {
            auto visitor = [&](sql_val_variant& d) -> std::string {
                return is_string() ? std::get<std::string>(data) : std::string{};
            };
            return std::visit<std::string>(visitor, data);
        }

        double to_double() {
            auto visitor = [&](sql_val_variant& d) -> double {
                return is_double() ? std::get<double>(data) : double{};
            };
            return std::visit<double>(visitor, data);
        }

        float to_float() {
            auto visitor = [&](sql_val_variant& d) -> float {
                return is_float() ? std::get<float>(data) : float{};
            };
            return std::visit<float>(visitor, data);
        }

        std::int8_t to_int8() {
            auto visitor = [&](sql_val_variant& d) -> std::int8_t {
                return is_int8() ? std::get<std::int8_t>(data) : std::int8_t{};
            };
            return std::visit<std::int8_t>(visitor, data);
        }

        std::int16_t to_int16() {
            auto visitor = [&](sql_val_variant& d) -> std::int16_t {
                return is_int16() ? std::get<std::int16_t>(data) : std::int16_t{};
            };
            return std::visit<std::int16_t>(visitor, data);
        }

        std::int32_t to_int32() {
            auto visitor = [&](sql_val_variant& d) -> std::int32_t {
                return is_int32() ? std::get<std::int32_t>(data) : std::int32_t{};
            };
            return std::visit<std::int32_t>(visitor, data);
        }

        std::int64_t to_int64() {
            auto visitor = [&](sql_val_variant& d) -> std::int64_t {
                return is_int64() ? std::get<std::int64_t>(data) : std::int64_t{};
            };
            return std::visit<std::int64_t>(visitor, data);
        }

        guid_struct to_guid() {
            auto visitor = [&](sql_val_variant& d) -> guid_struct {
                return is_guid() ? std::get<guid_struct>(data) : guid_struct{};
            };
            return std::visit<guid_struct>(visitor, data);
        }

        datetime_struct to_datetime() {
            auto visitor = [&](sql_val_variant& d) -> datetime_struct {
                return is_datetime() ? std::get<datetime_struct>(data) : datetime_struct{};
            };
            return std::visit<datetime_struct>(visitor, data);
        }

        date_struct to_date() {
            auto visitor = [&](sql_val_variant& d) -> date_struct {
                return is_date() ? std::get<date_struct>(data) : date_struct{};
            };
            return std::visit<date_struct>(visitor, data);
        }

        time_struct to_time() {
            auto visitor = [&](sql_val_variant& d) -> time_struct {
                return is_time() ? std::get<time_struct>(data) : time_struct{};
            };
            return std::visit<time_struct>(visitor, data);
        }

        std::vector<std::uint8_t> to_blob() {
            auto visitor = [&](sql_val_variant& d) -> std::vector<std::uint8_t> {
                return is_blob() ? std::get<std::vector<std::uint8_t>>(data) : std::vector<std::uint8_t>{};
            };
            return std::visit<std::vector<std::uint8_t>>(visitor, data);
        }

    };

    struct sql_value {
        sql_variant data;
        sql_dtype data_type;
        bool is_null;

        std::string to_string(bool* invalid_dtype = nullptr) {
            if (invalid_dtype)
                *invalid_dtype = false;

            if (is_null)
                return std::string();

            if (data_type != sql_dtype::string) {
                if (invalid_dtype)
                    *invalid_dtype = true;

                return std::string();
            }
            return std::get<std::basic_string<char>>(data);
        }

        double to_double(bool* invalid_dtype = nullptr) {
            if (invalid_dtype)
                *invalid_dtype = false;

            if (is_null)
                return 0.0;

            if (data_type != sql_dtype::floating_point) {
                if (invalid_dtype)
                    *invalid_dtype = true;

                return 0.0;
            }
            return std::get<double>(data);
        }

        int to_int(bool* invalid_dtype = nullptr) {
            if (invalid_dtype)
                *invalid_dtype = false;

            if (is_null)
                return 0;

            if (data_type != sql_dtype::integer) {
                if (invalid_dtype)
                    *invalid_dtype = true;

                return 0;
            }
            return std::get<int>(data);
        }

        guid_struct to_guid(bool* invalid_dtype = nullptr) {
            if (invalid_dtype)
                *invalid_dtype = false;

            if (is_null)
                return guid_struct{};

            if (data_type != sql_dtype::guid) {
                if (invalid_dtype)
                    *invalid_dtype = true;

                return guid_struct{};
            }
            return std::get<guid_struct>(data);
        }

        datetime_struct to_datetime(bool* invalid_dtype = nullptr) {
            if (invalid_dtype)
                *invalid_dtype = false;

            if (is_null)
                return datetime_struct{};

            if (data_type != sql_dtype::datetime) {
                if (invalid_dtype)
                    *invalid_dtype = true;

                return datetime_struct{};
            }
            return std::get<datetime_struct>(data);
        }

        date_struct to_date(bool* invalid_dtype = nullptr) {
            if (invalid_dtype)
                *invalid_dtype = false;

            if (is_null)
                return date_struct{};

            if (data_type != sql_dtype::date) {
                if (invalid_dtype)
                    *invalid_dtype = true;

                return date_struct{};
            }
            return std::get<date_struct>(data);
        }

        time_struct to_time(bool* invalid_dtype = nullptr) {
            if (invalid_dtype)
                *invalid_dtype = false;

            if (is_null)
                return time_struct{};

            if (data_type != sql_dtype::time) {
                if (invalid_dtype)
                    *invalid_dtype = true;

                return time_struct{};
            }
            return std::get<time_struct>(data);
        }

        std::vector<std::uint8_t> to_blob(bool* invalid_dtype = nullptr) {
            if (invalid_dtype)
                *invalid_dtype = false;

            if (is_null)
                return {};

            if (data_type != sql_dtype::blob) {
                if (invalid_dtype)
                    *invalid_dtype = true;

                return {};
            }
            return std::get<std::vector<std::uint8_t>>(data);
        }
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
