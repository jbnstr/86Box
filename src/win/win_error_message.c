#include "win\win_error_message.h"
#include <stdio.h>

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

size_t
get_system_error_message_a(char *const pszBuffer, size_t const bufferSizeInChars, DWORD const errorCode)
{
    size_t charsWritten = 0;

    if (pszBuffer == NULL || bufferSizeInChars == 0 || bufferSizeInChars > INT_MAX)
        return charsWritten;
    //
    // Returns number of chars stored in the output buffer, excluding the 
    // terminating null character (or returns 0 on failure).
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
