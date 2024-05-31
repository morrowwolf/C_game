
#include "../headers/helpers.h"

// TODO: Kill this as soon as possible
void GetCurrentPath(_Out_writes_(pathSize) WCHAR *const path, UINT pathSize)
{
    if (path == NULL)
    {
        OutputDebugString(TEXT("Assets path is NULL \n"));
        exit(EXIT_FAILURE);
    }

    DWORD size = GetModuleFileNameW(NULL, path, pathSize);
    if (size == 0 || size == pathSize)
    {
        exit(EXIT_FAILURE);
    }

    WCHAR *lastSlash = wcsrchr(path, L'\\');
    if (lastSlash)
    {
        *(lastSlash + 1) = L'\0';
    }
}
