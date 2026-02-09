#ifndef SimQL_Handles_header_h
#define SimQL_Handles_header_h

// STL stuff
#include <memory>
#include <cstdint>

namespace SimpleSqlHandles {

    class ENV {
    private:

        /* custom deleter */
        struct deleter;

    public:

        /* constructor/destructor */
        ENV() = default;
        ~ENV() { deallocate(); }

        /* functions */
        const std::uint8_t& allocate();
        void deallocate();
        void* handle();

    private:

        /* handle container */
        void* m_handle;

        /* return codes */
        static constexpr std::uint8_t _RC_SUCCESS               = 0;
        static constexpr std::uint8_t _RC_SUCCESS_WITH_INFO     = 1;
        static constexpr std::uint8_t _RC_FAILURE               = 2;
        static constexpr std::uint8_t _RC_ODBC_VERSION3         = 3;
    };

    class DBC {
    private:

        /* custom deleter */
        struct deleter;

    public:

        /* constructor/destructor */
        DBC() = default;
        ~DBC() { deallocate(); }

        /* functions */
        const std::uint8_t& allocate(void* h_env);
        void deallocate();
        std::unique_ptr<void, deleter>& handle();

    private:

        /* handle container */
        std::unique_ptr<void, deleter> m_handle;

        /* return codes */
        static constexpr std::uint8_t _RC_SUCCESS               = 0;
        static constexpr std::uint8_t _RC_SUCCESS_WITH_INFO     = 1;
        static constexpr std::uint8_t _RC_FAILURE               = 2;
    };

    class STMT {
    private:

        /* custom deleter */
        struct deleter;

    public:

        /* constructor/destructor */
        STMT() = default;
        ~STMT() { deallocate(); }

        /* functions */
        const std::uint8_t& allocate(void* h_dbc);
        const std::uint8_t& reset();
        void deallocate();
        std::unique_ptr<void, deleter>& handle();

    private:

        /* handle container */
        std::unique_ptr<void, deleter> m_handle;

        /* return codes */
        static constexpr std::uint8_t _RC_SUCCESS               = 0;
        static constexpr std::uint8_t _RC_SUCCESS_WITH_INFO     = 1;
        static constexpr std::uint8_t _RC_FAILURE               = 2;
        static constexpr std::uint8_t _RC_CLOSE_CURSOR          = 3;
        static constexpr std::uint8_t _RC_RESET_PARAMS          = 4;
        static constexpr std::uint8_t _RC_UNBIND_COLS           = 5;
    };
}

#endif