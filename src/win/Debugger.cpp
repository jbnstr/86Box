#include <Windows.h>

#include <cstdint>
#include <sstream>
#include <string>
#include <memory>
#include <iomanip> // for std::hex, std::uppercase, etc.

#include "win\Debugger.h"
#include "86Box\86box.h"

#define STRINGIFY_A(identifier) #identifier
// ANSI name of identifier.
#define ANSI_NAME_OF(identifier) STRINGIFY_A(identifier)
// To ANSI string.
#define TO_ANSI_STRING(identifier) STRINGIFY_A(identifier)


namespace Diagnostics 
{
    struct LocalFreeDeleter {
        using pointer = char *;
        void operator()(pointer pBuffer) const noexcept
        {
            if (pBuffer != nullptr) {
                ::LocalFree(pBuffer);
                pBuffer = nullptr;
            }
        }
    };

    std::string
    FormatWin32Error(DWORD errorCode)
    {
        std::ostringstream oss;
        oss << "Unknown error (0x"
            << std::hex << std::uppercase << std::setw(8) << std::setfill('0')
            << errorCode << ")";
        return oss.str();
    }

    std::string
    FormatErrorMessage(DWORD errorCode)
    {
        std::unique_ptr<char, LocalFreeDeleter> lpMsgBuf(nullptr);

        auto charCount = ::FormatMessageA(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL,
            errorCode,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            reinterpret_cast<char *>(&lpMsgBuf),
            0, NULL);

        if (charCount > 0) {
            if ((lpMsgBuf.get())[charCount - 2] == '\r') {
                charCount -= 2;
            } else if ((lpMsgBuf.get())[charCount - 1] == '\n') {
                charCount -= 1;
            }
            return std::string(lpMsgBuf.get(), charCount);
        } else {
            return FormatWin32Error(errorCode);
        }
    }

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
        UINT nChars = ::GetSystemDirectoryW(&systemDir[0], static_cast<UINT>(systemDir.length()));

        if (nChars == 0) {
            fatal(ANSI_NAME_OF(::GetSystemDirectoryW));
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
            fatal(ANSI_NAME_OF(::CreateProcessW));
        }

        // Wait for the debugger to attach
        while (!::IsDebuggerPresent())
            Sleep(100);

        // Stop execution so the debugger can take over
        DebugBreak();
    }
}