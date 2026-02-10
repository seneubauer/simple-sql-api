#ifndef SimStatement_header_h
#define SimStatement_header_h

// SimQL stuff
#include <SimConnection.hpp>
#include <SimQL_ReturnCodes.hpp>

// STL stuff
#include <cstdint>
#include <memory>

namespace SimpleSql {
    class Statement {
    public:

        /* enums */
        enum class CursorType : std::uint8_t {
            CX_FORWARD_ONLY,
            CX_STATIC,
            CX_DYNAMIC,
            CX_KEYSET_DRIVEN
        };

        /* handle options */
        struct Options {
            CursorType cursor_type = CursorType::CX_FORWARD_ONLY;
            std::uint32_t query_timeout = 0;
            std::uint64_t max_rows = 0;
        };

        /* constructor/destructor */
        explicit Statement(Connection& dbc, const Options& options);
        ~Statement();

        /* set move assignment */
        Statement(Statement&&) noexcept;
        Statement& operator=(Statement&&) noexcept;

        /* set copy assignment */
        Statement(const Statement&) = delete;
        Statement& operator=(const Statement&) = delete;

        /* functions */
        void prepare(std::string_view sql);
        void execute();
        void execute_direct(std::string_view sql);
        const SimQL_ReturnCodes::Code& return_code();

    private:
        struct handle;
        std::unique_ptr<handle> sp_handle;
    };
}

#endif