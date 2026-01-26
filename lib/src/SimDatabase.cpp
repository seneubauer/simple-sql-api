// SimQL stuff
#include <SimQL_Constants.hpp>
#include <SimQL_Types.hpp>
#include <SimDatabase.hpp>

// STL stuff
#include <memory>
#include <utility>
#include <functional>
#include <cstdint>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <future>

// for compiling on Windows (ew)
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
#define NONLS                       // All NLS defines and routines
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

bool SimpleSql::SimDatabase::remove_stmt_handle() {

    // remove handle from the vector
    std::lock_guard<std::mutex> lock(m_mutex);
    m_stmt_vector.erase(m_stmt_vector.begin() + m_stmt_index);
    if (m_stmt_index >= m_stmt_vector.size())
        m_stmt_index = 0;

    // run the statement pool listener
    std::uint8_t remaining_stmt_count = m_stmt_vector.size();
    (*mp_stmt_pool_listener)(std::move(remaining_stmt_count));

    return m_stmt_vector.size() > 0 ? true : false;
}

bool SimpleSql::SimDatabase::set_connection_pooling(const SimpleSqlTypes::ConnectionPoolingType& value) {

    SQLUINTEGER odbc_value;
    switch (value) {
    case SimpleSqlTypes::ConnectionPoolingType::OFF:
        odbc_value = SQL_CP_OFF;
        break;
    case SimpleSqlTypes::ConnectionPoolingType::ONE_PER_DRIVER:
        odbc_value = SQL_CP_ONE_PER_DRIVER;
        break;
    case SimpleSqlTypes::ConnectionPoolingType::ONE_PER_ENV:
        odbc_value = SQL_CP_ONE_PER_HENV;
        break;
    default:
        return false;
    }

    SQLRETURN sr = SQLSetEnvAttr(h_env.get(), SQL_ATTR_CONNECTION_POOLING, &odbc_value, 0);
    return sr != SQL_SUCCESS && sr != SQL_SUCCESS_WITH_INFO;
}

bool SimpleSql::SimDatabase::set_access_mode(const SimpleSqlTypes::AccessModeType& value) {

    SQLUINTEGER odbc_value;
    switch (value) {
    case SimpleSqlTypes::AccessModeType::READ_ONLY:
        odbc_value = SQL_MODE_READ_ONLY;
        break;
    case SimpleSqlTypes::AccessModeType::READ_WRITE:
        odbc_value = SQL_MODE_READ_WRITE;
        break;
    default:
        return false;
    }

    SQLRETURN sr = SQLSetConnectAttr(h_dbc.get(), SQL_ATTR_ACCESS_MODE, &odbc_value, 0);
    return sr != SQL_SUCCESS && sr != SQL_SUCCESS_WITH_INFO;
}

bool SimpleSql::SimDatabase::set_driver_async(const SimpleSqlTypes::AsyncModeType& value) {

    SQLUINTEGER odbc_value;
    switch (value) {
    case SimpleSqlTypes::AsyncModeType::ENABLED:
        odbc_value = SQL_ASYNC_ENABLE_ON;
        break;
    case SimpleSqlTypes::AsyncModeType::DISABLED:
        odbc_value = SQL_ASYNC_ENABLE_OFF;
        break;
    default:
        return false;
    }

    SQLRETURN sr = SQLSetConnectAttr(h_dbc.get(), SQL_ATTR_ASYNC_ENABLE, &odbc_value, 0);
    return sr != SQL_SUCCESS && sr != SQL_SUCCESS_WITH_INFO;
}

bool SimpleSql::SimDatabase::set_autocommit(const SimpleSqlTypes::AutocommitType& value) {

    SQLUINTEGER odbc_value;
    switch (value) {
    case SimpleSqlTypes::AutocommitType::ENABLED:
        odbc_value = SQL_AUTOCOMMIT_ON;
        break;
    case SimpleSqlTypes::AutocommitType::DISABLED:
        odbc_value = SQL_AUTOCOMMIT_OFF;
        break;
    default:
        return false;
    }

    SQLRETURN sr = SQLSetConnectAttr(h_dbc.get(), SQL_ATTR_AUTOCOMMIT, &odbc_value, 0);
    return sr != SQL_SUCCESS && sr != SQL_SUCCESS_WITH_INFO;
}

bool SimpleSql::SimDatabase::set_login_timeout(const std::uint32_t& value) {
    SQLUINTEGER odbc_value = static_cast<SQLUINTEGER>(value);
    SQLRETURN sr = SQLSetConnectAttr(h_dbc.get(), SQL_ATTR_LOGIN_TIMEOUT, &odbc_value, 0);
    return sr != SQL_SUCCESS && sr != SQL_SUCCESS_WITH_INFO;
}

bool SimpleSql::SimDatabase::set_connection_timeout(const std::uint32_t& value) {
    SQLUINTEGER odbc_value = static_cast<SQLUINTEGER>(value);
    SQLRETURN sr = SQLSetConnectAttr(h_dbc.get(), SQL_ATTR_CONNECTION_TIMEOUT, &odbc_value, 0);
    return sr != SQL_SUCCESS && sr != SQL_SUCCESS_WITH_INFO;
}

bool SimpleSql::SimDatabase::get_connection_pooling(SimpleSqlTypes::ConnectionPoolingType& value) {
    SQLUINTEGER odbc_value;
    SQLRETURN sr = SQLGetEnvAttr(h_env.get(), SQL_ATTR_CONNECTION_POOLING, &odbc_value, 0, nullptr);
    if (sr != SQL_SUCCESS && sr != SQL_SUCCESS_WITH_INFO)
        return false;

    switch (odbc_value) {
    case SQL_CP_OFF:
        value = SimpleSqlTypes::ConnectionPoolingType::OFF;
        break;
    case SQL_CP_ONE_PER_DRIVER:
        value = SimpleSqlTypes::ConnectionPoolingType::ONE_PER_DRIVER;
        break;
    case SQL_CP_ONE_PER_HENV:
        value = SimpleSqlTypes::ConnectionPoolingType::ONE_PER_ENV;
        break;
    default:
        return false;
    }
    return true;
}

bool SimpleSql::SimDatabase::get_access_mode(SimpleSqlTypes::AccessModeType& value) {
    SQLUINTEGER odbc_value;
    SQLRETURN sr = SQLGetConnectAttr(h_dbc.get(), SQL_ATTR_ACCESS_MODE, &odbc_value, 0, nullptr);
    if (sr != SQL_SUCCESS && sr != SQL_SUCCESS_WITH_INFO)
        return false;

    switch (odbc_value) {
    case SQL_MODE_READ_ONLY:
        value = SimpleSqlTypes::AccessModeType::READ_ONLY;
        break;
    case SQL_MODE_READ_WRITE:
        value = SimpleSqlTypes::AccessModeType::READ_WRITE;
        break;
    default:
        return false;
    }
    return true;
}

bool SimpleSql::SimDatabase::get_driver_async(SimpleSqlTypes::AsyncModeType& value) {
    SQLUINTEGER odbc_value;
    SQLRETURN sr = SQLGetConnectAttr(h_dbc.get(), SQL_ATTR_ASYNC_ENABLE, &odbc_value, 0, nullptr);
    if (sr != SQL_SUCCESS && sr != SQL_SUCCESS_WITH_INFO)
        return false;

    switch (odbc_value) {
    case SQL_ASYNC_ENABLE_ON:
        value = SimpleSqlTypes::AsyncModeType::ENABLED;
        break;
    case SQL_ASYNC_ENABLE_OFF:
        value = SimpleSqlTypes::AsyncModeType::DISABLED;
        break;
    default:
        return false;
    }
    return true;
}

bool SimpleSql::SimDatabase::get_autocommit(SimpleSqlTypes::AutocommitType& value) {
    SQLUINTEGER odbc_value;
    SQLRETURN sr = SQLGetConnectAttr(h_dbc.get(), SQL_ATTR_AUTOCOMMIT, &odbc_value, 0, nullptr);
    if (sr != SQL_SUCCESS && sr != SQL_SUCCESS_WITH_INFO)
        return false;

    switch (odbc_value) {
    case SQL_AUTOCOMMIT_ON:
        value = SimpleSqlTypes::AutocommitType::ENABLED;
        break;
    case SQL_AUTOCOMMIT_OFF:
        value = SimpleSqlTypes::AutocommitType::DISABLED;
        break;
    default:
        return false;
    }
    return true;
}

bool SimpleSql::SimDatabase::get_login_timeout(std::uint32_t& value) {
    SQLUINTEGER odbc_value;
    SQLRETURN sr = SQLGetConnectAttr(h_dbc.get(), SQL_ATTR_LOGIN_TIMEOUT, &odbc_value, 0, nullptr);
    if (sr != SQL_SUCCESS && sr != SQL_SUCCESS_WITH_INFO)
        return false;

    value = static_cast<std::uint32_t>(odbc_value);
    return true;
}

bool SimpleSql::SimDatabase::get_connection_timeout(std::uint32_t& value) {
    SQLUINTEGER odbc_value;
    SQLRETURN sr = SQLGetConnectAttr(h_dbc.get(), SQL_ATTR_CONNECTION_TIMEOUT, &odbc_value, 0, nullptr);
    if (sr != SQL_SUCCESS && sr != SQL_SUCCESS_WITH_INFO)
        return false;

    value = static_cast<std::uint32_t>(odbc_value);
    return true;
}

bool SimpleSql::SimDatabase::get_connection_state(bool& connected) {
    SQLUINTEGER odbc_value;
    SQLRETURN sr = SQLGetConnectAttr(h_dbc.get(), SQL_ATTR_CONNECTION_TIMEOUT, &odbc_value, 0, nullptr);
    if (sr != SQL_SUCCESS && sr != SQL_SUCCESS_WITH_INFO)
        return false;

    switch (odbc_value) {
    case SQL_CD_TRUE:
        connected = true;
        break;
    case SQL_CD_FALSE:
        connected = false;
        break;
    default:
        return false;
    }
    return true;
}

bool SimpleSql::SimDatabase::open_transaction() {
    return set_autocommit(SimpleSqlTypes::AutocommitType::DISABLED);
}

bool SimpleSql::SimDatabase::rollback_transaction() {
    SQLRETURN sr = SQLEndTran(SQL_HANDLE_DBC, h_dbc.get(), SQL_ROLLBACK);
    return sr != SQL_SUCCESS && sr != SQL_SUCCESS_WITH_INFO;
}

bool SimpleSql::SimDatabase::commit_transaction() {
    SQLRETURN sr = SQLEndTran(SQL_HANDLE_DBC, h_dbc.get(), SQL_COMMIT);
    return sr != SQL_SUCCESS && sr != SQL_SUCCESS_WITH_INFO;
}

std::uint8_t SimpleSql::SimDatabase::connect(std::string& conn_str) {

    std::uint8_t rc;
    SQLRETURN sr;
    SQLCHAR* conn_str_in = const_cast<SQLCHAR*>(reinterpret_cast<const SQLCHAR*>(conn_str.c_str()));
    unsigned char conn_str_out[1024];

    SQLHANDLE env;
    sr = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &env);
    if (sr != SQL_SUCCESS && sr != SQL_SUCCESS_WITH_INFO) {
        rc = SimpleSqlConstants::ReturnCodes::D_ENV_HANDLE_ALLOC;
        goto end_of_function;
    }
    h_env = SimpleSqlTypes::ENV_HANDLE(env);

    sr = SQLSetEnvAttr(h_env.get(), SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3, 0);
    if (sr != SQL_SUCCESS && sr != SQL_SUCCESS_WITH_INFO) {
        rc = SimpleSqlConstants::ReturnCodes::D_ODBC_VERSION3;
        goto free_env_handle;
    }

    SQLHANDLE dbc;
    sr = SQLAllocHandle(SQL_HANDLE_DBC, h_env.get(), &dbc);
    if (sr != SQL_SUCCESS && sr != SQL_SUCCESS_WITH_INFO) {
        rc = SimpleSqlConstants::ReturnCodes::D_DBC_HANDLE_ALLOC;
        goto free_dbc_handle;
    }
    h_dbc = SimpleSqlTypes::DBC_HANDLE(dbc);

    SQLSMALLINT conn_str_out_len;
    sr = SQLDriverConnect(h_dbc.get(), nullptr, conn_str_in, SQL_NTS, conn_str_out, sizeof(conn_str_out), &conn_str_out_len, SQL_DRIVER_NOPROMPT);
    if (sr != SQL_SUCCESS && sr != SQL_SUCCESS_WITH_INFO) {
        rc = SimpleSqlConstants::ReturnCodes::D_CONNECTION;
        goto free_dbc_handle;
    }

    for (std::uint8_t i = 0; i < m_stmt_count; ++i) {
        SQLHANDLE h;
        sr = SQLAllocHandle(SQL_HANDLE_STMT, h_dbc.get(), &h);

        if (sr != SQL_SUCCESS && sr != SQL_SUCCESS_WITH_INFO) {
            m_skipped++;
            continue;
        }
        m_stmt_vector.push_back(SimpleSqlTypes::STMT_HANDLE(h));
    }

    end_of_function:
    return SimpleSqlConstants::ReturnCodes::SUCCESS;

    free_dbc_handle:
    SQLFreeHandle(SQL_HANDLE_DBC, h_dbc.get());

    free_env_handle:
    SQLFreeHandle(SQL_HANDLE_ENV, h_env.get());

    return rc;
}

void SimpleSql::SimDatabase::disconnect() {

    for (SimpleSqlTypes::STMT_HANDLE& h : m_stmt_vector) {
        if (h.get() != SQL_NULL_HANDLE)
            SQLFreeHandle(SQL_HANDLE_STMT, h.get());
    }

    bool is_connected = false;
    get_connection_state(is_connected);
    if (!is_connected)
        return;

    SQLDisconnect(h_dbc.get());
    SQLFreeHandle(SQL_HANDLE_DBC, h_dbc.get());
    SQLFreeHandle(SQL_HANDLE_ENV, h_env.get());
}

bool SimpleSql::SimDatabase::extract_stmt_handle(SimpleSqlTypes::STMT_HANDLE& handle) {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_stmt_vector[m_stmt_index] == SQL_NULL_HSTMT || m_stmt_vector[m_stmt_index] == nullptr) {

        // the handle is null, so make a new one in its place
        SQLHANDLE h;
        SQLRETURN sr = SQLAllocHandle(SQL_HANDLE_STMT, h_dbc.get(), &h);
        if (sr != SQL_SUCCESS && sr != SQL_SUCCESS_WITH_INFO)
            if (!remove_stmt_handle())
                return false;

        // assign the new handle to the vector
        m_stmt_vector[m_stmt_index] = SimpleSqlTypes::STMT_HANDLE(h);
    }
    handle = std::move(m_stmt_vector[m_stmt_index]);

    // advance to the next index so the next assignment starts on the next handle
    m_stmt_index++;
    if (m_stmt_index >= m_stmt_vector.size())
        m_stmt_index = 0;

    return true;
}

void SimpleSql::SimDatabase::reclaim_stmt_handle(SimpleSqlTypes::STMT_HANDLE&& handle) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_stmt_vector[m_stmt_index] = std::move(handle);

    auto make_stmt_handle = [&](SQLHANDLE &h) -> bool {
        SQLRETURN sr;
        sr = SQLAllocHandle(SQL_HANDLE_STMT, h_dbc.get(), &h);
        if (sr != SQL_SUCCESS && sr != SQL_SUCCESS_WITH_INFO)
            return false;

        return true;
    };

    SQLRETURN sr;
    sr = SQLFreeStmt(m_stmt_vector[m_stmt_index].get(), SQL_CLOSE);
    if (sr != SQL_SUCCESS && sr != SQL_SUCCESS_WITH_INFO) {
        SQLHANDLE h;
        if (!make_stmt_handle(h)) {
            remove_stmt_handle();
            return;
        }
        m_stmt_vector[m_stmt_index].reset(h);
        return;
    }

    sr = SQLFreeStmt(m_stmt_vector[m_stmt_index].get(), SQL_RESET_PARAMS);
    if (sr != SQL_SUCCESS && sr != SQL_SUCCESS_WITH_INFO) {
        SQLHANDLE h;
        if (!make_stmt_handle(h)) {
            remove_stmt_handle();
            return;
        }
        m_stmt_vector[m_stmt_index].reset(h);
        return;
    }

    sr = SQLFreeStmt(m_stmt_vector[m_stmt_index].get(), SQL_UNBIND);
    if (sr != SQL_SUCCESS && sr != SQL_SUCCESS_WITH_INFO) {
        SQLHANDLE h;
        if (!make_stmt_handle(h)) {
            remove_stmt_handle();
            return;
        }
        m_stmt_vector[m_stmt_index].reset(h);
        return;
    }
}

void SimpleSql::SimDatabase::listen(std::shared_ptr<std::function<void(std::uint8_t&&)>> p_listener) {
    mp_stmt_pool_listener = std::move(p_listener);
}