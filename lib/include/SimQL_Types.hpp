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

    enum class SQLDataType : std::uint8_t {
        UNKNOWN             = 0,
        STRING              = 1,
        LONG_STRING         = 2,
        FLOAT               = 3,
        DOUBLE              = 4,
        BOOLEAN             = 5,
        INT_8               = 6,
        INT_16              = 7,
        INT_32              = 8,
        INT_64              = 9,
        ODBC_GUID           = 10,
        GUID                = 11,
        DATETIME            = 12,
        DATE                = 13,
        TIME                = 14,
        BLOB                = 15,
        LONG_BLOB           = 16
    };
    constexpr std::uint8_t operator^(SQLDataType l, SQLDataType r) {
        return static_cast<std::uint8_t>(l) ^ static_cast<std::uint8_t>(r);
    }

    /* STRUCTS */

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
    concept simql_temporal = requires(T& t) {
        std::is_same_v<T, _Datetime> ||
        std::is_same_v<T, _Date> ||
        std::is_same_v<T, _Time>;
    };

    template<simql_temporal T>
    struct BaseTemporal {
    private:
        T m_temporal;
        bool m_is_utc;
    public:
        BaseTemporal(T&& t, const bool& is_utc) : m_temporal(std::move(t)), m_is_utc(is_utc) {}
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

    using SQLVariant = std::variant<
        std::string,
        std::wstring,
        float,
        double,
        bool,
        std::int8_t,
        std::int16_t,
        std::int32_t,
        std::int64_t,
        ODBC_GUID,
        GUID,
        Datetime,
        Date,
        Time,
        std::vector<std::uint8_t>>;

    struct SQL_Binding {
        std::string name;
        SQLVariant data;
        BindingType type;
        SQLDataType data_type;
        bool set_null;
        std::int64_t indicator;
        SQLVariant& ref_data() { return data; }
        std::int64_t& ref_indicator() { return indicator; }
    };

    struct SQL_Value {
        SQLVariant data;
        SQLDataType data_type;
        bool is_null;
    };

    struct SQL_Column {
        std::string name;
        std::uint8_t ordinal;
        SQLDataType data_type;
        std::uint64_t size;
        std::int16_t precision;
        NullRuleType null_type;
    };
}

#endif
