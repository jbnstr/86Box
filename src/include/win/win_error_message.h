#include <windows.h>

extern char *get_system_error_message_a(DWORD const errorCode);
extern char *append_system_error_message_a(char const *const pszMessage, DWORD const errorCode);

extern wchar_t *to_wide_string(char const *const pszSource);
extern wchar_t *get_system_error_message_w(DWORD const errorCode);
extern wchar_t *append_system_error_message_w(wchar_t const *const pwszMessage, DWORD const errorCode);
