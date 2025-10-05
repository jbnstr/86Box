#include "win\win_error_message.h"
#include <stdio.h>

wchar_t *
to_wide_string(char const *const pszSource)
{
    int wideCharCount = MultiByteToWideChar(CP_ACP, 0, pszSource, -1, NULL, 0);
    if (wideCharCount == 0)
        return NULL;

    wchar_t *pwszResult = (wchar_t *) LocalAlloc(LPTR, wideCharCount * sizeof(wchar_t));
    if (pwszResult == NULL)
        return NULL;

    int result = MultiByteToWideChar(CP_ACP, 0, pszSource, -1, pwszResult, wideCharCount);
    if (result == 0) {
        LocalFree(pwszResult);
        return NULL;
    }

    return pwszResult;
}

char *
trim_newline_a(char *const pszStr)
{
    if (pszStr == NULL)
        return NULL;

    size_t len = strlen(pszStr);
    if (len >= 2 && pszStr[len - 2] == '\r' && pszStr[len - 1] == '\n') {
        pszStr[len - 2] = '\0';
    } else if (len >= 1 && (pszStr[len - 1] == '\n' || pszStr[len - 1] == '\r')) {
        pszStr[len - 1] = '\0';
    }

    return pszStr;
}

static wchar_t *
trim_newline_w(wchar_t *const pwszStr)
{
    if (pwszStr == NULL)
        return pwszStr;

    size_t len = wcslen(pwszStr);
    if (len >= 2 && pwszStr[len - 2] == L'\r' && pwszStr[len - 1] == L'\n') {
        pwszStr[len - 2] = L'\0';
    } else if (len >= 1 && (pwszStr[len - 1] == L'\n' || pwszStr[len - 1] == L'\r')) {
        pwszStr[len - 1] = L'\0';
    }

    return pwszStr;
}

char *
get_system_error_message_a(DWORD const errorCode)
{
    char *pszResult    = NULL;
    DWORD charsWritten = FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        errorCode,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPSTR) &pszResult, // Buffer is allocated by FormatMessageA. Caller must free with LocalFree()
        0,
        NULL);

    if (charsWritten == 0) {
        const int BUFFER_SIZE = 64;
        DWORD     lastErr     = GetLastError();
        if (lastErr == ERROR_MR_MID_NOT_FOUND) {
            // Unknown error code, allocate a default message.
            pszResult = (char *) LocalAlloc(LPTR, BUFFER_SIZE);
            if (pszResult != NULL) {
                snprintf(pszResult, BUFFER_SIZE, "Unknown system error (0x%08X)", errorCode);
            }
        }
    }

    return pszResult;
}

size_t
get_system_error_message_a_2(char *const pszBuffer, size_t const bufferSizeInChars, DWORD const errorCode)
{
    size_t charsWritten = 0;

    if (pszBuffer == NULL || bufferSizeInChars == 0 || bufferSizeInChars > INT_MAX)
        return charsWritten;
    //
    // Returns number of chars stored in the output buffer, excluding the 
    // terminating null character (or 0 on failure).
    //
    charsWritten = (size_t) FormatMessageA(
        FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        errorCode,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        pszBuffer,
        (DWORD) bufferSizeInChars,
        NULL);

    if (charsWritten > 0)
        return charsWritten;

    DWORD lastErr = GetLastError();
    if (lastErr == ERROR_MR_MID_NOT_FOUND) {
        snprintf(pszBuffer, bufferSizeInChars, "Unknown system error (0x%08X)", errorCode);
        pszBuffer[bufferSizeInChars - 1] = '\0';
        charsWritten                     = strlen(pszBuffer);
    }

    return charsWritten;
}

wchar_t *
get_system_error_message_w(DWORD const errorCode)
{
    wchar_t *pwszResult = NULL;

    char *pszSysErrMsg = get_system_error_message_a(errorCode);
    if (pszSysErrMsg == NULL)
        return NULL;

    pwszResult = to_wide_string(pszSysErrMsg);

    LocalFree(pszSysErrMsg);
    pszSysErrMsg = NULL;

    return pwszResult;
}

char *
append_system_error_message_a(char const *const pszMessage, DWORD const errorCode)
{
    char *pszFinalMsg  = NULL;
    char *pszSysErrMsg = NULL;

    pszSysErrMsg = trim_newline_a(get_system_error_message_a(errorCode));

    if (pszMessage == NULL && pszSysErrMsg != NULL) {
        return pszSysErrMsg;
    }

    if (pszSysErrMsg != NULL) {
        size_t lenSysErrMsg    = strlen(pszSysErrMsg);
        size_t lenMessage      = strlen(pszMessage);
        size_t lenFinalMessage = lenMessage + 2 + lenSysErrMsg + 1; // Account for \r\n and \0

        pszFinalMsg = (char *) LocalAlloc(LPTR, lenFinalMessage);
        if (pszFinalMsg != NULL) {
            snprintf(pszFinalMsg, lenFinalMessage, "%s\r\n%s", pszMessage, pszSysErrMsg);
        }
    }

    if (pszSysErrMsg != NULL) {
        LocalFree(pszSysErrMsg);
        pszSysErrMsg = NULL;
    }

    return pszFinalMsg;
}

wchar_t *
append_system_error_message_w(wchar_t const *const pwszMessage, DWORD const errorCode)
{
    wchar_t *pwszFinalMsg  = NULL;
    wchar_t *pwszSysErrMsg = NULL;

    pwszSysErrMsg = trim_newline_w(get_system_error_message_w(errorCode));

    if (pwszMessage == NULL && pwszSysErrMsg != NULL) {
        return pwszSysErrMsg;
    }

    if (pwszSysErrMsg != NULL) {
        size_t lenSysErrMsg    = wcslen(pwszSysErrMsg);
        size_t lenMessage      = wcslen(pwszMessage);
        size_t lenFinalMessage = lenMessage + 2 + lenSysErrMsg + 1; // Account for \r\n and \0

        pwszFinalMsg = (wchar_t *) LocalAlloc(LPTR, lenFinalMessage * sizeof(wchar_t));
        if (pwszFinalMsg != NULL) {
            swprintf(pwszFinalMsg, lenFinalMessage, L"%ls\r\n%ls", pwszMessage, pwszSysErrMsg);
        }
    }

    if (pwszSysErrMsg != NULL) {
        LocalFree(pwszSysErrMsg);
        pwszSysErrMsg = NULL;
    }

    return pwszFinalMsg;
}