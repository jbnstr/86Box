#include <Windows.h>

#include <cstdint>
#include <sstream>
#include <string>
#include <memory>

#include "win\win_error_message.h"
#include "win\Debugger.h"
#include "86Box\86box.h"

namespace Diagnostics 
{
    struct ProcessInfo : public PROCESS_INFORMATION 
    {

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
        // Get System directory (typically C:\Windows\System32).
        std::wstring systemDir(static_cast<size_t>(MAX_PATH + 1), L'\0');
        UINT         nChars = ::GetSystemDirectoryW(&systemDir[0], static_cast<UINT>(systemDir.length()));

        if (nChars == 0) {
            HANDLE_WINAPI_ERROR(::GetSystemDirectoryW, GetLastError());
        }

        systemDir.resize(nChars);

        // Get process ID and create the command line.
        DWORD               pid = ::GetCurrentProcessId();
        std::wostringstream s;
        s << systemDir << L"\\vsjitdebugger.exe -p " << pid;
        std::wstring cmdLine = s.str();

        // Start debugger process
        STARTUPINFOW si = {};
        ProcessInfo  pi = {};
        if (!::CreateProcessW(NULL, &cmdLine[0], NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
            HANDLE_WINAPI_ERROR(::CreateProcessW, GetLastError());
        }

        // Wait for the debugger to attach.
        while (!::IsDebuggerPresent())
            Sleep(100);

        // Stop execution so the debugger can take over.
        DebugBreak();
    }
}