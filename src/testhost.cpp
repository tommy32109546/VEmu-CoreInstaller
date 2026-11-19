#include <windows.h>
#include <iostream>

int wmain(int argc, wchar_t** argv)
{
    if (argc < 2)
    {
        std::wcerr << L"Usage: testhost.exe <dll path>\n";
        return 1;
    }

    HMODULE module = LoadLibraryW(argv[1]);
    if (!module)
    {
        std::wcerr << L"LoadLibrary failed: " << GetLastError() << L"\n";
        return 2;
    }

    using StartType = void* (*)();
    auto start = reinterpret_cast<StartType>(GetProcAddress(module, "start_mod"));
    if (!start)
    {
        std::wcerr << L"GetProcAddress failed\n";
        return 3;
    }

    void* result = start();
    std::wcout << L"start_mod returned " << result << L"\n";
    return 0;
}
