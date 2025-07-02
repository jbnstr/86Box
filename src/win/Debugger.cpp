#include <Windows.h>

#include <cstdint>
#include <sstream>
#include <string>
#include <memory>
// #include <iomanip> // for std::hex, std::uppercase, etc.

#include "win\win_error_message.h"
#include "win\Debugger.h"
#include "86Box\86box.h"

//#define STRINGIFY_A(identifier) #identifier
//// ANSI name of identifier.
//#define ANSI_NAME_OF(identifier) STRINGIFY_A(identifier)
//// To ANSI string.
//#define TO_ANSI_STRING(identifier) STRINGIFY_A(identifier)



//#define HANDLE_FATAL_ERROR(func)                                                           \
//    do {                                                                                   \
//        char szMsg[256] = { 0 };                                                           \
//        snprintf(szMsg, sizeof(szMsg), "%s(%d): '%s' failed.", __FILE__, __LINE__, #func); \
//        char *pszErrMsg = append_system_error_message_a(szMsg, ::GetLastError());          \
//        if (pszErrMsg != NULL) {                                                           \
//            fatal(pszErrMsg);                                                              \
//            ::LocalFree(pszErrMsg);                                                        \
//        } else {                                                                           \
//            fatal(szMsg);                                                                  \
//        }                                                                                  \
//    } while (0)


namespace Diagnostics {
struct ProcessInfo : public PROCESS_INFORMATION {

    ~ProcessInfo()
    {
        if (hThread != NULL)
            CloseHandle(hThread);

        if (hProcess != NULL)
            CloseHandle(hProcess);
    }
};

void
Debugger::Launch()
{
    // Get System directory, typically c:\windows\system32
    std::wstring systemDir(static_cast<size_t>(MAX_PATH + 1), L'\0');
    UINT         nChars = ::GetSystemDirectoryW(&systemDir[0], static_cast<UINT>(systemDir.length()));

    if (nChars == 0) {
        HANDLE_WINAPI_ERROR(::GetSystemDirectoryW);

        //char szMsg[256] = { 0 };
        //snprintf(szMsg, sizeof(szMsg), "%s(%d): '%s' failed.", __FILE__, __LINE__, ANSI_NAME_OF(::GetSystemDirectoryW));
        //char *pszErrMsg = append_system_error_message_a(szMsg, ::GetLastError());
        //if (pszErrMsg != NULL) {
        //    fatal(pszErrMsg);
        //    ::LocalFree(pszErrMsg);
        //} else {
        //    fatal(szMsg);
        //}
    }

    systemDir.resize(nChars);

    // Get process ID and create the command line
    DWORD               pid = ::GetCurrentProcessId();
    std::wostringstream s;
    s << systemDir << L"\\vsjitdebugger.exe -p " << pid;
    std::wstring cmdLine = s.str();

    // Start debugger process
    STARTUPINFOW si = {};
    ProcessInfo  pi = {};
    if (!::CreateProcessW(NULL, &cmdLine[0], NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        HANDLE_WINAPI_ERROR_2(::CreateProcessW, GetLastError());
    }

    // Wait for the debugger to attach
    while (!::IsDebuggerPresent())
        Sleep(100);

    // Stop execution so the debugger can take over
    DebugBreak();
}
}