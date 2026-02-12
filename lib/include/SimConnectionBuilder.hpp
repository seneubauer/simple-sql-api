#ifndef SimConnectionBuilder_header_h
#define SimConnectionBuilder_header_h

// SimQL stuff
#include <SimQL_Types.hpp>

// STL stuff
#include <string>
#include <cstdint>

namespace simql {
    class connection_string_builder {
    public:

        /* enums */
        enum class database_type : std::uint8_t {
            sql_server,
            postgresql
        };

        /* constructor/destructor */
        connection_string_builder(const database_type& db_type) : m_db_type(db_type) {}
        ~connection_string_builder() { destroy(); }

        /* functions */
        std::string get();
        void set_driver(const std::string& driver);
        void set_server(const std::string& server);
        void set_port(const std::uint16_t& port);
        void set_database(const std::string& database);
        void set_username(const std::string& username);
        void set_password(const std::string& password);
        void set_sslmode(const bool& sslmode);
        void set_mars(const bool& mars);
        void set_readonly(const bool& readonly);
        void set_trusted(const bool& trusted);
        void set_encrypt(const bool& encrypt);
        void destroy();

    private:

        /* internal functions */
        std::string assemble() const;

        /* members */
        database_type m_db_type;
        std::string m_driver{};
        std::string m_server{};
        std::uint16_t m_port{0};
        std::string m_database{};
        std::string m_username{};
        std::string m_password{};
        bool m_sslmode{false};
        bool m_mars{false};
        bool m_readonly{false};
        bool m_trusted{false};
        bool m_encrypt{false};
    };
}

#endif
