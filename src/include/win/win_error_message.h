#include <windows.h>

#include <stdint.h>      // because of 86box.h
#include <stdio.h>       // because of 86box.h
#include "86box\86box.h" // because of fatal()

#define STRINGIFY_A(identifier)  #identifier
#define ANSI_NAME_OF(identifier) STRINGIFY_A(identifier)

#define HANDLE_WINAPI_ERROR(api_call)                                                                       \
    do {                                                                                                    \
        char szMsg[512] = { 0 };                                                                            \
        snprintf(szMsg, sizeof(szMsg), "%s(%d): '%s' failed.", __FILE__, __LINE__, ANSI_NAME_OF(api_call)); \
        char *pszErrMsg = append_system_error_message_a(szMsg, GetLastError());                             \
        if (pszErrMsg != NULL) {                                                                            \
            fatal(pszErrMsg);                                                                               \
            LocalFree(pszErrMsg);                                                                           \
        } else {                                                                                            \
            fatal(szMsg);                                                                                   \
        }                                                                                                   \
    } while (0)

#define HANDLE_WINAPI_ERROR_2(api_call, dwError)                                                                                  \
    do {                                                                                                                          \
        char szSysErrMsg[512] = { 0 };                                                                                            \
        char szFinalMsg[1024] = { 0 };                                                                                            \
        int  lenSysErrMsg     = get_system_error_message_a_2(szSysErrMsg, sizeof(szSysErrMsg) / sizeof(szSysErrMsg[0]), dwError); \
        if (lenSysErrMsg > 0) {                                                                                                   \
            trim_newline_a(szSysErrMsg);                                                                                          \
        }                                                                                                                         \
        snprintf(szFinalMsg, sizeof(szFinalMsg) / sizeof(szFinalMsg[0]), "%s(%d): '%s' failed: %s", __FILE__, __LINE__,           \
                 ANSI_NAME_OF(api_call),                                                                                          \
                 lenSysErrMsg > 0 ? szSysErrMsg : "System error message could not be retrieved.");                                \
        fatal(szFinalMsg);                                                                                                        \
    } while (0)

#ifdef __cplusplus
extern "C" {
#endif

extern size_t get_system_error_message_a_2(char *const pszBuffer, size_t const bufferSizeInChars, DWORD const errorCode);

extern char *get_system_error_message_a(DWORD const errorCode);
extern char *append_system_error_message_a(char const *const pszMessage, DWORD const errorCode);
extern char *trim_newline_a(char *const pszStr);

extern wchar_t *to_wide_string(char const *const pszSource);
extern wchar_t *get_system_error_message_w(DWORD const errorCode);
extern wchar_t *append_system_error_message_w(wchar_t const *const pwszMessage, DWORD const errorCode);

#ifdef __cplusplus
}
#endif