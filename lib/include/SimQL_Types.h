#ifndef SimQL_Types_header_h
#define SimQL_Types_header_h

// STL stuff
#include <cstdint>
#include <format>
#include <string>
#include <string_view>
#include <cstdlib>
#include <variant>

namespace SimpleSqlTypes {

    /* ENUMS */

    enum class NullRuleType : uint8_t {
        NOT_SET = 0,
        UNKNOWN,
        ALLOWED,
        NOT_ALLOWED
    };

    enum class ResultStorageType : uint8_t {
        NOT_SET = 0,
        VECTOR_BASED,
        KEY_BASED
    };

    enum class BindingType {
        INPUT_OUTPUT,
        INPUT,
        OUTPUT
    };

    enum class SQLDataType : uint8_t {
        NOT_SET = 0,
        CHAR,
        VARCHAR,
        LONG_VARCHAR,
        WCHAR,
        VARWCHAR,
        LONG_VARWCHAR,
        TINYINT,
        SMALLINT,
        INTEGER,
        BIGINT,
        FLOAT,
        DOUBLE,
        BIT,
        BINARY,
        VARBINARY
        LONG_VARBINARY,
        GUID,
        DATE,
        TIME,
        TIMESTAMP,
        UTC_DATETIME,
        UTC_TIME,
        INTERVAL_YEAR,
        INTERVAL_MONTH,
        INTERVAL_DAY,
        INTERVAL_HOUR,
        INTERVAL_MINUTE,
        INTERVAL_SECOND,
        INTERVAL_YEAR2MONTH,
        INTERVAL_DAY2HOUR,
        INTERVAL_DAY2MINUTE,
        INTERVAL_DAY2SECOND,
        INTERVAL_HOUR2MINUTE,
        INTERVAL_HOUR2SECOND,
        INTERVAL_MINUTE2SECOND
    };

    enum class CellDataType : uint8_t {
        ISNULL = 0,
        BOOLEAN,
        INT_8,
        INT_16,
        INT_32,
        INT_64,
        UINT_8,
        UINT_16,
        UINT_32,
        UINT_64,
        NUM_32,
        NUM_64,
        SMALL_STRING,
        BIG_STRING,
        DATE,
        TIME,
        DATETIME,
        BLOB
    };
    
    enum class IntervalSign : uint8_t {
        POSITIVE = 0,
        NEGATIVE
    };

    /* STRUCTS */

    struct ColumnMetadata {
        uint16_t position;
        std::string name;
        SQLDataType sql_data_type;
        uint64_t data_size;
        uint16_t precision;
        NullRuleType null_rule;
        ColumnMetadata(
            const uint16_t &i_position,
            const std::string &i_name,
            const SQLDataType &i_sql_data_type,
            const uint64_t &i_data_size,
            const uint16_t &i_precision,
            const NullRuleType &i_null_rule
        ) : position(i_position), name(i_name), sql_data_type(i_sql_data_type), data_size(i_data_size), precision(i_precision), null_rule(i_null_rule) {}
    };

    struct DiagnosticRecord {
        int16_t record_number;
        std::string sql_state;
        int32_t native_error;
        std::string message;
        DiagnosticRecord(
            const int16_t &i_record_number,
            const std::string &i_sql_state,
            const int32_t &i_native_error,
            const std::string &i_message
        ) : record_number(i_record_number), sql_state(i_sql_state), native_error(i_native_error), message(i_message) {}
    };

    struct SmallString {
        static constexpr size_t _n = 512;
        uint8_t size{};
        char buffer[_n];

        void assign(const char* s, size_t n) noexcept {
            size = static_cast<uint8_t>(n);
            std::memcpy(buffer, s, n);
        }
        std::string_view view() const noexcept { return {buffer, size}; }
        std::string to_string() const { return std::string(view()); }
    };

    struct BigString {
        size_t size{};
        size_t capacity{};
        char* p_buffer{};

        void free() noexcept {
            std::free(p_buffer);
            p_buffer = nullptr;
            size = 0;
            capacity = 0;
        }
        std::string_view view() const noexcept { return {p_buffer, size}; }
        std::string to_string() const { return std::string(view()); }
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

    struct Interval {
        IntervalSign interval_sign;
        union {
            Date        d;
            Time        t;
            Datetime    dt;
        } data;
    };

    struct Blob {
        uint8_t* data;
        size_t size;
        
        void free() noexcept {
            std::free(data);
            data = nullptr;
            size = 0;
        }
    };

    using BindingValue = std::variant<
        std::string,
        std::wstring,
        float,
        double,
        bool,
        int8_t,
        int16_t,
        int32_t,
        int64_t,
        Date,
        Time,
        Datetime,
        Interval,
        std::vector<uint8_t>>;
    struct SQLBinding {
        BindingValue data;
        BindingType binding_type;
        SQLDataType sql_data_type;
        bool set_null;
    };

    struct Cell {
        CellDataType data_type { CellDataType::ISNULL };
        union {
            bool        b;              // BOOLEAN
            int8_t      i8;             // INT_8
            int16_t     i16;            // INT_16
            int32_t     i32;            // INT_32
            int64_t     i64;            // INT_64
            uint8_t     ui8;            // UINT_8
            uint16_t    ui16;           // UINT_16
            uint32_t    ui32;           // UINT_32
            uint64_t    ui64;           // UINT_64
            float       n32;            // NUM_32
            double      n64;            // NUM_64
            SmallString sstring;        // SMALL_STRING
            BigString   bstring;        // BIG_STRING
            Date        date;           // DATE
            Time        time;           // TIME
            Datetime    datetime;       // DATETIME
            Blob        blob;           // BLOB
        } data;

        // lifecycle
        Cell() noexcept : datatype(CellDataType::ISNULL) {}
        ~Cell() { destroy(); }
        Cell(const Cell &cell) { copy_from(cell); }
        Cell(Cell &&cell) noexcept { move_from(std::move(cell)); }
        Cell& operator=(Cell cell) noexcept {
            this->swap(cell);
            return *this;
        }
        void swap(Cell &cell) noexcept {
            if (this == &cell)
                return;

            Cell temp;
            temp.move_from(std::move(cell));
            cell.move_from(std::move(*this));
            this->move_from(std::move(temp));
        }

        // setters
        void set_null() noexcept { destroy(); data_type = CellDataType::ISNULL; }
        void set_bool(bool value) { destroy(); data_type = CellDataType::boolean; b = value; }
        void set_int8(int8_t value) { destroy(); data_type = CellDataType::int8; i8 = value; }
        void set_int16(int16_t value) { destroy(); data_type = CellDataType::int16; i16 = value; }
        void set_int32(int32_t value) { destroy(); data_type = CellDataType::int32; i32 = value; }
        void set_int64(int64_t value) { destroy(); data_type = CellDataType::int64; i64 = value; }
        void set_uint8(uint8_t value) { destroy(); data_type = CellDataType::uint8; ui8 = value; }
        void set_uint16(uint16_t value) { destroy(); data_type = CellDataType::uint16; ui16 = value; }
        void set_uint32(uint32_t value) { destroy(); data_type = CellDataType::uint32; ui32 = value; }
        void set_uint64(uint64_t value) { ddestroy(); ata_type = CellDataType::uint64; ui64 = value; }
        void set_float(float value) { destroy(); data_type = CellDataType::num32; n32 = value; }
        void set_double(double value) { destroy(); data_type = CellDataType::num64; n64 = value; }
        void set_string(const char* s, size_t n) {
            if (n < sstring._n) {
                destroy();
                data_type = CellDataType::SMALL_STRING;
                new (&sstring) SmallString{};
                sstring.assign(s, n);
            } else {
                if (data_type == CellDataType::BIG_STRING && bstring.capacity >= n) {
                    std::memcpy(bstring.p_buffer, s, n);
                    bstring.size = n;
                } else {
                    size_t new_capacity = grow_capacity(n);
                    char* p = static_cast<char*>(std::malloc(new_capacity));
                    std::memcpy(p, s, n);
                    destroy();
                    data_type = CellDataType::BIG_STRING;
                    new (&bstring) BigString{};
                    bstring.p_buffer = p;
                    bstring.size = n;
                    bstring.capacity;
                }
            }
        }
        void set_string(std::string_view strv) { set_string(strv.data(), strv.size()); }
        void set_date(uint16_t year, uint8_t month, uint8_t day) { destroy(); data_type = CellDataType::date; date = Date {year, month, day}; }
        void set_time(uint8_t hour, uint8_t minute, uint8_t second, uint16_t millisecond) { destroy(); data_type = CellDataType::time; time = Time {hour, minute, second, millisecond}; }
        void set_datetime(uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute, uint8_t second, uint16_t millisecond) { destroy(); data_type = CellDataType::datetime; datetime = Datetime {year, month, day, hour, minute, second, millisecond}; }
        void set_blob(const void* data, size_t n) {
            if (data_type == CellDataType::BLOB && blob.size >= n) {
                std::memcpy(blob.data, data, n);
                blob.size = n;
                return;
            }

            uint8_t* p = static_cast<uint8_t*>(std::malloc(n));
            std::memcpy(p, data, n);

            destroy();
            new (&blob) Blob{};
            blob.data = p;
            blob.size = n;
        }

        // getters
        const bool& is_null() const { return data_type == CellDataType::null; }
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
        const SmallString& as_smallstring() const { return sstring; }
        const BigString& as_bigstring() const { return bstring; }
        const Date& as_date() const { return date; }
        const Time& as_time() const { return time; }
        const Datetime& as_datetime() const { return datetime; }
        const Blob& as_blob() const { return blob; }

        // faultless getters
        const bool& try_bool(bool &value) { data_type == CellDataType::BOOLEAN ? ( value = b, return true ) : return false; }
        const bool& try_int8(int8_t &value) { data_type == CellDataType::INT_8 ? ( value = i8, return true ) : return false; }
        const bool& try_int16(int16_t &value) { data_type == CellDataType::INT_16 ? ( value = i16, return true ) : return false; }
        const bool& try_int32(int32_t &value) { data_type == CellDataType::INT_32 ? ( value = i32, return true ) : return false; }
        const bool& try_int64(int64_t &value) { data_type == CellDataType::INT_64 ? ( value = i64, return true ) : return false; }
        const bool& try_uint8(uint8_t &value) { data_type == CellDataType::UINT_8 ? ( value = ui8, return true ) : return false; }
        const bool& try_uint16(uint16_t &value) { data_type == CellDataType::UINT_16 ? ( value = ui16, return true ) : return false; }
        const bool& try_uint32(uint32_t &value) { data_type == CellDataType::UINT_32 ? ( value = ui32, return true ) : return false; }
        const bool& try_uint64(uint64_t &value) { data_type == CellDataType::UINT_64 ? ( value = ui64, return true ) : return false; }
        const bool& try_float(float &value) { data_type == CellDataType::NUM_32 ? ( value = n32, return true ) : return false; }
        const bool& try_double(double &value) { data_type == CellDataType::NUM_64 ? ( value = n64, return true ) : return false; }
        const bool& try_smallstring(SmallString &value) { data_type == CellDataType::SMALL_STRING ? ( value = sstring, return true ) : return false; }
        const bool& try_bigstring(BigString &value) { data_type == CellDataType::BIG_STRING ? ( value = bstring, return true ) : return false; }
        const bool& try_date(Date &value) { data_type == CellDataType::DATE ? ( value = date, return true ) : return false; }
        const bool& try_time(Time &value) { data_type == CellDataType::TIME ? ( value = time, return true ) : return false; }
        const bool& try_datetime(Datetime &value) { data_type == CellDataType::DATETIME ? ( value = datetime, return true ) : return false; }
        const bool& try_blob(Blob &value) { data_type == CellDataType::BLOB ? ( value = blob, return true ) : return false; }

    private:
        static constexpr size_t base_capacity = 32;

        void destroy() noexcept {
            switch (data_type) {
            case CellDataType::BIG_STRING:
                bstring.free();
                break;
            case CellDataType::BLOB:
                blob.free();
                break;
            default:
                break;
            }
        }

        static size_t grow_capacity(const size_t &need) noexcept {
            size_t capacity = base_capacity;
            while (capacity < need) capacity = capacity + capacity / 2;
            return capacity;
        }

        void copy_from(const Cell &cell) {
            switch (cell.data_type) {
            case CellDataType::ISNULL:
                data_type = cell.data_type;
                break;
            case CellDataType::BOOLEAN:
                data_type = cell.data_type;
                b = cell.b;
                break;
            case CellDataType::INT_8:
                data_type = cell.data_type;
                i8 = cell.i8;
                break;
            case CellDataType::INT_16:
                data_type = cell.data_type;
                i16 = cell.i16;
                break;
            case CellDataType::INT_32:
                data_type = cell.data_type;
                i32 = cell.i32;
                break;
            case CellDataType::INT_64:
                data_type = cell.data_type;
                i64 = cell.i64;
                break;
            case CellDataType::UINT_8:
                data_type = cell.data_type;
                ui8 = cell.ui8;
                break;
            case CellDataType::UINT_16:
                data_type = cell.data_type;
                ui16 = cell.ui16;
                break;
            case CellDataType::UINT_32:
                data_type = cell.data_type;
                ui32 = cell.ui32;
                break;
            case CellDataType::UINT_64:
                data_type = cell.data_type;
                ui64 = cell.ui64;
                break;
            case CellDataType::NUM_32:
                data_type = cell.data_type;
                n32 = cell.n32;
                break;
            case CellDataType::NUM_64:
                data_type = cell.data_type;
                n64 = cell.n64;
                break;
            case CellDataType::SMALL_STRING:
                data_type = cell.data_type;
                new (&sstring) SmallString{};
                sstring.assign(cell.sstring.buffer, cell.sstring.size);
                break;
            case CellDataType::BIG_STRING:
                data_type = cell.data_type;
                new (&bstring) BigString{};
                if (cell.bstring.size) {
                    size_t capacity = cell.bstring.size;
                    bstring.p_buffer = static_cast<char*>(std::malloc(capacity));
                    std::memcpy(bstring.p_buffer, cell.bstring.p_buffer, cell.bstring.size);
                    bstring.size = cell.bstring.size;
                    bstring.capacity = cell.bstring.capacity;
                } else {
                    bstring.p_buffer = nullptr;
                    bstring.size = 0;
                    bstring.capacity = 0;
                }
                break;
            case CellDataType::DATE:
                data_type = cell.data_type;
                date = cell.date;
                break;
            case CellDataType::TIME:
                data_type = cell.data_type;
                time = cell.time;
                break;
            case CellDataType::DATETIME:
                data_type = cell.data_type;
                datetime = cell.datetime;
                break;
            case CellDataType::BLOB:
                data_type = cell.data_type;
                new (&blob) Blob{};
                if (cell.blob.size) {
                    blob.data = static_cast<uint8_t*>(std::malloc(cell.blob.size));
                    std::memcpy(blob.data, cell.blob.data, cell.blob.size);
                    blob.size = cell.blob.size;
                } else {
                    blob.data = nullptr;
                    blob.size = 0;
                }
                break;
            }
        }

        void move_from(Cell &&cell) noexcept {
            destroy();
            data_type = cell.data_type;
            switch (cell.data_type) {
            case CellDataType::ISNULL:
                data_type = cell.data_type;
                break;
            case CellDataType::BOOLEAN:
                data_type = cell.data_type;
                b = cell.b;
                break;
            case CellDataType::INT_8:
                data_type = cell.data_type;
                i8 = cell.i8;
                break;
            case CellDataType::INT_16:
                data_type = cell.data_type;
                i16 = cell.i16;
                break;
            case CellDataType::INT_32:
                data_type = cell.data_type;
                i32 = cell.i32;
                break;
            case CellDataType::INT_64:
                data_type = cell.data_type;
                i64 = cell.i64;
                break;
            case CellDataType::UINT_8:
                data_type = cell.data_type;
                ui8 = cell.ui8;
                break;
            case CellDataType::UINT_16:
                data_type = cell.data_type;
                ui16 = cell.ui16;
                break;
            case CellDataType::UINT_32:
                data_type = cell.data_type;
                ui32 = cell.ui32;
                break;
            case CellDataType::UINT_64:
                data_type = cell.data_type;
                ui64 = cell.ui64;
                break;
            case CellDataType::NUM_32:
                data_type = cell.data_type;
                n32 = cell.n32;
                break;
            case CellDataType::NUM_64:
                data_type = cell.data_type;
                n64 = cell.n64;
                break;
            case CellDataType::SMALL_STRING:
                data_type = cell.data_type;
                new (&sstring) SmallString{};
                sstring.assign(cell.sstring.buffer, cell.sstring.size);
                break;
            case CellDataType::BIG_STRING:
                data_type = cell.data_type;
                new (&bstring) BigString{};
                bstring.p_buffer = cell.bstring.p_buffer;
                bstring.size = cell.bstring.size;
                bstring.capacity = cell.bstring.capacity;
                cell.bstring.p_buffer = nullptr;
                cell.bstring.size = 0;
                cell.bstring.capacity = 0;
                break;
            case CellDataType::DATE:
                data_type = cell.data_type;
                date = cell.date;
                break;
            case CellDataType::TIME:
                data_type = cell.data_type;
                time = cell.time;
                break;
            case CellDataType::DATETIME:
                data_type = cell.data_type;
                datetime = cell.datetime;
                break;
            case CellDataType::BLOB:
                data_type = cell.data_type;
                new (&blob) Blob{};
                blob.data = cell.blob.data;
                blob.size = cell.blob.size;
                cell.blob.data = nullptr;
                cell.blob.size = 0;
                break;
            }
            cell.data_type = CellDataType::ISNULL;
        }
    };
}

#endif