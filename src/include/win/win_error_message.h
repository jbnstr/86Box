#include <windows.h>

#include <stdint.h>      // because of 86box.h
#include <stdio.h>       // because of 86box.h
#include "86box\86box.h" // because of fatal()

#define STRINGIFY_A(identifier)  #identifier
#define ANSI_NAME_OF(identifier) STRINGIFY_A(identifier)

#define HANDLE_WINAPI_ERROR(api_call, dwError)                                                                                  \
    do {                                                                                                                        \
        char szSysErrMsg[512] = { 0 };                                                                                          \
        char szFinalMsg[1024] = { 0 };                                                                                          \
        int  lenSysErrMsg     = get_system_error_message_a(szSysErrMsg, sizeof(szSysErrMsg) / sizeof(szSysErrMsg[0]), dwError); \
        if (lenSysErrMsg > 0) {                                                                                                 \
            trim_newline_a(szSysErrMsg);                                                                                        \
        }                                                                                                                       \
        snprintf(szFinalMsg, sizeof(szFinalMsg) / sizeof(szFinalMsg[0]), "%s(%d): '%s' failed: %s", __FILE__, __LINE__,         \
                 ANSI_NAME_OF(api_call),                                                                                        \
                 lenSysErrMsg > 0 ? szSysErrMsg : "System error message could not be retrieved.");                              \
        fatal(szFinalMsg);                                                                                                      \
    } while (0)

#ifdef __cplusplus
extern "C" {
#endif

extern size_t get_system_error_message_a(char *const pszBuffer, size_t const bufferSizeInChars, DWORD const errorCode);
extern char *trim_newline_a(char *const pszStr);

#ifdef __cplusplus
}
#endif