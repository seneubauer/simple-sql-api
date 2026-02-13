// SimQL stuff
#include <simql_strings.hpp>

// STL stuff
#include <string>
#include <string_view>
#include <vector>

// Windows stuff
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOGDICAPMASKS               // CC_*, LC_*, PC_*, CP_*, TC_*, RC_
#define NOVIRTUALKEYCODES           // VK_*
#define NOWINMESSAGES               // WM_*, EM_*, LB_*, CB_*
#define NOWINSTYLES                 // WS_*, CS_*, ES_*, LBS_*, SBS_*, CBS_*
#define NOSYSMETRICS                // SM_*
#define NOMENUS                     // MF_*
#define NOICONS                     // IDI_*
#define NOKEYSTATES                 // MK_*
#define NOSYSCOMMANDS               // SC_*
#define NORASTEROPS                 // Binary and Tertiary raster ops
#define NOSHOWWINDOW                // SW_*
#define OEMRESOURCE                 // OEM Resource values
#define NOATOM                      // Atom Manager routines
#define NOCLIPBOARD                 // Clipboard routines
#define NOCOLOR                     // Screen colors
#define NOCTLMGR                    // Control and Dialog routines
#define NODRAWTEXT                  // DrawText() and DT_*
#define NOGDI                       // All GDI defines and routines
#define NOKERNEL                    // All KERNEL defines and routines
#define NOUSER                      // All USER defines and routines
// #define NONLS                    // All NLS defines and routines, required for string conversion
#define NOMB                        // MB_* and MessageBox()
#define NOMEMMGR                    // GMEM_*, LMEM_*, GHND, LHND, associated routines
#define NOMETAFILE                  // typedef METAFILEPICT
// #define NOMINMAX                 // Macros min(a,b) and max(a,b) || already set in mingw os_defines.h
#define NOMSG                       // typedef MSG and associated routines
#define NOOPENFILE                  // OpenFile(), OemToAnsi, AnsiToOem, and OF_*
#define NOSCROLL                    // SB_* and scrolling routines
#define NOSERVICE                   // All Service Controller routines, SERVICE_ equates, etc.
#define NOSOUND                     // Sound driver routines
#define NOTEXTMETRIC                // typedef TEXTMETRIC and associated routines
#define NOWH                        // SetWindowsHook and WH_*
#define NOWINOFFSETS                // GWL_*, GCL_*, associated routines
#define NOCOMM                      // COMM driver routines
#define NOKANJI                     // Kanji support stuff.
#define NOHELP                      // Help engine interface.
#define NOPROFILER                  // Profiler interface.
#define NODEFERWINDOWPOS            // DeferWindowPos routines
#define NOMCX                       // Modem Configuration Extensions
#include <windows.h>
#endif

// ODBC stuff
#include <sqltypes.h>
#include <sqlext.h>
#include <sql.h>

// std::wstring utf8_to_utf16(std::string_view utf8) {
//     if (utf8.empty())
//         return {};

//     int length = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, utf8.data(), static_cast<int>(utf8.size()), nullptr, 0);
//     if (length <= 0)
//         return {};

//     std::wstring output(length, L'\0');
//     if (MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, utf8.data(), static_cast<int>(utf8.size()), output.data(), length) <= 0)
//         return {};

//     return output;
// }

// std::string utf16_to_utf8(std::wstring_view utf16) {
//     if (utf16.empty())
//         return {};
    
//     int length = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, utf16.data(), static_cast<int>(utf16.size()), nullptr, 0, nullptr, nullptr);
//     if (length <= 0)
//         return {};

//     std::string output(length, '\0');
//     if (WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, utf16.data(), static_cast<int>(utf16.size()), output.data(), length, nullptr, nullptr) <= 0)
//         return {};

//     return output;
// }

// #else

// std::u32string utf8_to_utf32(std::string_view utf8) {
//     std::u32string output;
//     size_t i = 0;

//     while (i < utf8.size()) {
//         char32_t c32 = 0;
//         unsigned char uc = utf8[i];

//         if (c32 < 0x80) {
//             c32 = uc;
//             i++;
//         } else if ((uc & 0xE0) == 0xC0) {
//             if (i + 1>= utf8.size())
//                 return {};

//             c32 = ((uc & 0x1F) << 6) | (utf8[i + 1] & 0x3F);
//             i += 2;
//         } else if ((uc & 0xF0) == 0xE0) {
//             if (i + 2 >= utf8.size())
//                 return {};
            
//             c32 = ((uc & 0x0F) << 12) | ((utf8[i + 1] & 0x3F) << 6) | (utf8[i + 2] & 0x3F);
//             i += 3;
//         } else if ((uc & 0xF8) == 0xF0) {
//             if (i + 2 >= utf8.size())
//                 return {};
            
//             c32 = ((uc & 0x07) << 18) | ((utf8[i + 1] & 0x3F) << 12) | ((utf8[i + 2] & 0x3F) << 6) | (utf8[i + 3] & 0x3F);
//             i += 4;
//         } else {
//             return {};
//         }
//         output.push_back(c32);
//     }
//     return output;
// }

// std::string utf32_to_utf8(std::u32string_view utf32) {
//     std::string output;
//     for (char32_t c32 : utf32) {
//         if (c32 <= 0x7F) {
//             output.push_back(static_cast<char>(c32));
//         } else if (c32 <= 0x7FF) {
//             output.push_back(static_cast<char>(0xC0 | (c32 >> 6)));
//             output.push_back(static_cast<char>(0x80 | (c32 & 0x3F)));
//         } else if (c32 <= 0xFFFF) {
//             output.push_back(static_cast<char>(0xE0 | (c32 >> 12)));
//             output.push_back(static_cast<char>(0x80 | ((c32 >> 6) & 0x3F)));
//             output.push_back(static_cast<char>(0x80 | (c32 & 0x3F)));
//         } else {
//             output.push_back(static_cast<char>(0xF0 | (c32 >> 18)));
//             output.push_back(static_cast<char>(0x80 | ((c32 >> 12) & 0x3F)));
//             output.push_back(static_cast<char>(0x80 | ((c32 >> 6) & 0x3F)));
//             output.push_back(static_cast<char>(0x80 | (c32 & 0x3F)));
//         }
//     }
//     return output;
// }

// #endif

// check OS
#if defined(_WIN32) || defined(_WIN64)
    #define WINDOWS
#elif defined(__APPLE__) && defined(__MACH__)
    #define MACOS
#elif defined(__linux__)
    #define LINUX
#else
    #define UNSUPPORTED_OS
#endif

namespace simql_strings {

    // std::basic_string<wchar_t> utf8_to_odbc(std::string_view s) {
    //     #ifdef _WIN32
    //         return utf8_to_utf16(s);
    //     #else
    //         auto u32 = utf8_to_utf32(s);
    //         return {u32.begin(), u32.end()};
    //     #endif
    // }

    // std::string odbc_to_utf8(std::basic_string<wchar_t> s) {
    //     #ifdef _WIN32
    //         return utf16_to_utf8(s);
    //     #else
    //         std::u32string u32(s.begin(), s.end());
    //         return utf32_to_utf8(u32);
    //     #endif
    // }

    /*

    SQLWCHAR equivalent...
    Windows     wchar_t             2 bytes
    Linux       unsigned short      2 bytes
    macOS       wchar_t             4 bytes

    SQLCHAR equivalent...
    Windows     char
    Linux       unsigned char
    macOS       unsigned char

    */

    std::basic_string<SQLWCHAR> to_odbc_w(std::basic_string_view<char> utf8) {
        if (utf8.empty())
            return {};

        #ifdef WINDOWS
            int length = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, utf8.data(), static_cast<int>(utf8.size()), nullptr, 0);
            if (length <= 0)
                return {};

            LPWSTR output_str;
            if (MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, utf8.data(), static_cast<int>(utf8.size()), output_str, length) <= 0)
                return {};

            return std::basic_string<SQLWCHAR>(reinterpret_cast<SQLWCHAR*>(output_str));
        #elifdef LINUX
        #elifdef MACOS
        #endif
    }

    std::basic_string<SQLCHAR> to_odbc_n(std::basic_string_view<char> utf8) {
        return std::basic_string<SQLCHAR>(reinterpret_cast<const SQLCHAR*>(utf8.data(), utf8.size()));
    }

    std::basic_string<char> from_odbc(std::basic_string_view<SQLWCHAR> odbc) {
        if (odbc.empty())
            return {};

        #ifdef WINDOWS
            std::wstring s(odbc);
            int length = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, s.data(), static_cast<int>(s.size()), nullptr, 0, nullptr, nullptr);
            if (length <= 0)
                return {};

            LPSTR output_str;
            if (WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, s.data(), static_cast<int>(s.size()), output_str, length, nullptr, nullptr) <= 0)
                return {};

            return std::basic_string<char>(reinterpret_cast<char*>(output_str));
        #elifdef LINUX
        #elifdef MACOS
        #endif

    }

    std::basic_string<char> from_odbc(std::basic_string_view<SQLCHAR> odbc) {
        return std::basic_string<char>(reinterpret_cast<const char*>(odbc.data(), odbc.size()));
    }
}