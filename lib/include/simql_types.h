#ifndef simql_types_header_h
#define simql_types_header_h

#include <cstdint>
#include <format>
#include <string>
#include <string_view>

namespace SimpleSqlTypes {

    enum class CellDataType {
        null,
        boolean,
        int8,
        int16,
        int32,
        int64,
        uint8,
        uint16,
        uint32,
        uint64,
        num32,
        num64,
        date,
        time,
        datetime,
        str
    };

    struct Date {
        uint16_t year;
        uint8_t month;
        uint8_t day;
        std::string tostring() const { return std::format("{:04}-{:02}-{:02}", year, month, day); }
    };

    struct Time {
        uint8_t hour;
        uint8_t minute;
        uint8_t second;
        uint16_t millisecond;
        std::string tostring() const { return std::format("{:02}:{:02}:{:02}.{:03}", hour, minute, second, millisecond); }
    };

    struct Datetime {
        uint16_t year;
        uint8_t month;
        uint8_t day;
        uint8_t hour;
        uint8_t minute;
        uint8_t second;
        uint16_t millisecond;
        std::string tostring() const { return std::format("{:04}-{:02}-{:02} {:02}:{:02}:{:02}.{:03}", year, month, day, hour, minute, second, millisecond); }
    };

    struct Cell {
        CellDataType datatype;
        union {
            bool        b;              // boolean
            int8_t      i8;             // int8
            int16_t     i16;            // int16
            int32_t     i32;            // int32
            int64_t     i64;            // int64
            uint8_t     ui8;            // uint8
            uint16_t    ui16;           // uint16
            uint32_t    ui32;           // uint32
            uint64_t    ui64;           // uint64
            float       n32;            // num32
            double      n64;            // num64
            Date        ts_date;        // date
            Time        ts_time;        // time
            Datetime    ts_datetime;    // datetime
            std::string _string;        // str
        };

        Cell(const Cell&);
        Cell(Cell&&);
        Cell& operator=(Cell);

        const bool& is_null() const { return datatype == CellDataType::null; }
        const bool& as_bool() const { return b; }
        const int8_t& as_int8() const { return i8; }
        const int16_t& as_int16() const { return i16; }
        const int32_t& as_int32() const { return i32; }
        const int64_t& as_int64() const { return i64; }
        const uint8_t& as_uint8() const { return ui8; }
        const uint16_t& as_uint16() const { return ui16; }
        const uint32_t& as_uint32() const { return ui32; }
        const uint64_t& as_uint64() const { return ui64; }
        const float& as_float() const { return n32; }
        const double& as_double() const { return n64; }
        const Date& as_date() const { return ts_date; }
        const Time& as_time() const { return ts_time; }
        const Datetime& as_datetime() const { return ts_datetime; }
        const std::string& as_string() const { return _string; }

        void set_null() { datatype = CellDataType::null; }
        void set_bool(bool value) { datatype = CellDataType::boolean; b = value; }
        void set_int8(int8_t value) { datatype = CellDataType::int8; i8 = value; }
        void set_int16(int16_t value) { datatype = CellDataType::int16; i16 = value; }
        void set_int32(int32_t value) { datatype = CellDataType::int32; i32 = value; }
        void set_int64(int64_t value) { datatype = CellDataType::int64; i64 = value; }
        void set_uint8(uint8_t value) { datatype = CellDataType::uint8; ui8 = value; }
        void set_uint16(uint16_t value) { datatype = CellDataType::uint16; ui16 = value; }
        void set_uint32(uint32_t value) { datatype = CellDataType::uint32; ui32 = value; }
        void set_uint64(uint64_t value) { datatype = CellDataType::uint64; ui64 = value; }
        void set_float(float value) { datatype = CellDataType::num32; n32 = value; }
        void set_double(double value) { datatype = CellDataType::num64; n64 = value; }
        void set_date(uint16_t year, uint8_t month, uint8_t day) { datatype = CellDataType::date; ts_date = Date {year, month, day}; }
        void set_time(uint8_t hour, uint8_t minute, uint8_t second, uint16_t millisecond) { datatype = CellDataType::time; ts_time = Time {hour, minute, second, millisecond}; }
        void set_datetime(uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute, uint8_t second, uint16_t millisecond) { datatype = CellDataType::datetime; ts_datetime = Datetime {year, month, day, hour, minute, second, millisecond}; }
        void set_string(std::string value) { datatype = CellDataType::str; _string = value; }

        const bool& check_bool(bool &value) const {
            datatype == CellDataType::boolean ? value = b : (void)0;
            return datatype == CellDataType::boolean ? true : false;
        }

        const bool& check_int8(int8_t &value) const {
            datatype == CellDataType::int8 ? value = i8 : (void)0;
            return datatype == CellDataType::int8 ? true : false;
        }

        const bool& check_int16(int16_t &value) const {
            datatype == CellDataType::int16 ? value = i16 : (void)0;
            return datatype == CellDataType::int16 ? true : false;
        }

        const bool& check_int32(int32_t &value) const {
            datatype == CellDataType::int32 ? value = i32 : (void)0;
            return datatype == CellDataType::int32 ? true : false;
        }

        const bool& check_int64(int64_t &value) const {
            datatype == CellDataType::int64 ? value = i64 : (void)0;
            return datatype == CellDataType::int64 ? true : false;
        }

        const bool& check_uint8(uint8_t &value) const {
            datatype == CellDataType::uint8 ? value = ui8 : (void)0;
            return datatype == CellDataType::uint8 ? true : false;
        }

        const bool& check_uint16(uint16_t &value) const {
            datatype == CellDataType::uint16 ? value = ui16 : (void)0;
            return datatype == CellDataType::uint16 ? true : false;
        }

        const bool& check_uint32(uint32_t &value) const {
            datatype == CellDataType::uint32 ? value = ui32 : (void)0;
            return datatype == CellDataType::uint32 ? true : false;
        }

        const bool& check_uint64(uint64_t &value) const {
            datatype == CellDataType::uint64 ? value = ui64 : (void)0;
            return datatype == CellDataType::uint64 ? true : false;
        }

        const bool& check_float(float &value) const {
            datatype == CellDataType::num32 ? value = n32 : (void)0;
            return datatype == CellDataType::num32 ? true : false;
        }

        const bool& check_double(double &value) const {
            datatype == CellDataType::num64 ? value = n64 : (void)0;
            return datatype == CellDataType::num64 ? true : false;
        }

        const bool& check_date(Date &value) const {
            datatype == CellDataType::date ? value = ts_date : (void)0;
            return datatype == CellDataType::date ? true : false;
        }

        const bool& check_time(Time &value) const {
            datatype == CellDataType::time ? value = ts_time : (void)0;
            return datatype == CellDataType::time ? true : false;
        }

        const bool& check_datetime(Datetime &value) const {
            datatype == CellDataType::datetime ? value = ts_datetime : (void)0;
            return datatype == CellDataType::datetime ? true : false;
        }

        const bool& check_string(std::string &value) const {
            datatype == CellDataType::string ? value = _string : (void)0;
            return datatype == CellDataType::string ? true : false;
        }
    };

    enum class ResultType {
        vector,
        key
    };

    enum class BindingType {
        input,
        output,
        input_output
    };

    template<typename T>
    struct SqlBinding {
        T data;
        BindingType type;
        CellDataType datatype;
    };
    
    struct SqlColumn {
        std::string name;
        uint8_t position;
    };

}

#endif