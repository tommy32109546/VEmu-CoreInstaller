#define NOMINMAX

#include <windows.h>
#include <bcrypt.h>

#include <algorithm>
#include <chrono>
#include <cctype>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <mutex>
#include <sstream>
#include <string>
#include <vector>

#ifndef NT_SUCCESS
#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)
#endif

namespace fs = std::filesystem;

static const wchar_t* const EXPECTED_HASH = L"5F133297E11F91270B3F196BEDF3AF97E22D6847885EB037C18ED8168F21AAB9";
static std::once_flag g_once;

static std::wstring Trim(const std::wstring& value)
{
    size_t first = 0;
    while (first < value.size() && std::isspace(static_cast<unsigned char>(value[first])))
    {
        ++first;
    }

    size_t last = value.size();
    while (last > first && std::isspace(static_cast<unsigned char>(value[last - 1])))
    {
        --last;
    }

    return value.substr(first, last - first);
}

static std::wstring ExpandEnvironment(const std::wstring& value)
{
    DWORD needed = ExpandEnvironmentStringsW(value.c_str(), nullptr, 0);
    if (needed == 0)
    {
        return value;
    }

    std::vector<wchar_t> buffer(needed, L'\0');
    if (ExpandEnvironmentStringsW(value.c_str(), buffer.data(), needed) == 0)
    {
        return value;
    }

    return std::wstring(buffer.data());
}

static std::string Utf8FromWide(const std::wstring& value)
{
    int needed = WideCharToMultiByte(CP_UTF8, 0, value.c_str(), static_cast<int>(value.size()), nullptr, 0, nullptr, nullptr);
    if (needed <= 0)
    {
        std::string fallback;
        fallback.reserve(value.size());
        for (wchar_t character : value)
        {
            fallback.push_back(character <= 0x7F ? static_cast<char>(character) : '?');
        }
        return fallback;
    }

    std::string result(static_cast<size_t>(needed), '\0');
    WideCharToMultiByte(CP_UTF8, 0, value.c_str(), static_cast<int>(value.size()), &result[0], needed, nullptr, nullptr);
    return result;
}

static std::string Timestamp()
{
    std::time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::tm local{};
    localtime_s(&local, &now);

    char buffer[32]{};
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &local);
    return std::string(buffer);
}

static void WriteLog(const fs::path& path, const std::wstring& message)
{
    if (path.empty())
    {
        return;
    }

    std::ofstream output(path, std::ios::app | std::ios::binary);
    if (!output)
    {
        return;
    }

    output << "[" << Timestamp() << "] " << Utf8FromWide(message) << "\r\n";
}

static std::wstring BytesToHex(const std::vector<unsigned char>& bytes)
{
    std::wostringstream output;
    output << std::uppercase << std::hex << std::setfill(L'0');
    for (unsigned char byte : bytes)
    {
        output << std::setw(2) << static_cast<int>(byte);
    }
    return output.str();
}

static bool Sha256File(const fs::path& path, std::wstring& hashOut)
{
    BCRYPT_ALG_HANDLE algorithm = nullptr;
    BCRYPT_HASH_HANDLE hash = nullptr;
    DWORD hashSize = 0;
    DWORD resultSize = 0;

    if (!NT_SUCCESS(BCryptOpenAlgorithmProvider(&algorithm, BCRYPT_SHA256_ALGORITHM, nullptr, 0)))
    {
        return false;
    }

    if (!NT_SUCCESS(BCryptGetProperty(algorithm, BCRYPT_HASH_LENGTH, reinterpret_cast<PUCHAR>(&hashSize), sizeof(hashSize), &resultSize, 0)))
    {
        BCryptCloseAlgorithmProvider(algorithm, 0);
        return false;
    }

    std::vector<unsigned char> hashObject(1024);
    if (!NT_SUCCESS(BCryptCreateHash(algorithm, &hash, hashObject.data(), static_cast<ULONG>(hashObject.size()), nullptr, 0, 0)))
    {
        BCryptCloseAlgorithmProvider(algorithm, 0);
        return false;
    }

    std::ifstream input(path, std::ios::binary);
    if (!input)
    {
        BCryptDestroyHash(hash);
        BCryptCloseAlgorithmProvider(algorithm, 0);
        return false;
    }

    std::vector<char> buffer(64 * 1024);
    while (input.read(buffer.data(), static_cast<std::streamsize>(buffer.size())) || input.gcount() > 0)
    {
        std::streamsize count = input.gcount();
        if (count <= 0)
        {
            break;
        }

        if (!NT_SUCCESS(BCryptHashData(hash, reinterpret_cast<PUCHAR>(buffer.data()), static_cast<ULONG>(count), 0)))
        {
            BCryptDestroyHash(hash);
            BCryptCloseAlgorithmProvider(algorithm, 0);
            return false;
        }
    }

    std::vector<unsigned char> digest(hashSize);
    bool ok = NT_SUCCESS(BCryptFinishHash(hash, digest.data(), static_cast<ULONG>(digest.size()), 0));
    if (ok)
    {
        hashOut = BytesToHex(digest);
    }

    BCryptDestroyHash(hash);
    BCryptCloseAlgorithmProvider(algorithm, 0);
    return ok;
}

static fs::path ReadUserDataDir(const fs::path& settingsPath)
{
    std::wifstream input(settingsPath);
    if (!input)
    {
        return fs::path();
    }

    std::wstring line;
    while (std::getline(input, line))
    {
        size_t key = line.find(L"user_data_dir");
        if (key == std::wstring::npos)
        {
            continue;
        }

        size_t equals = line.find(L'=', key);
        if (equals == std::wstring::npos)
        {
            continue;
        }

        std::wstring value = Trim(line.substr(equals + 1));
        if (value.size() >= 2 && ((value.front() == L'"' && value.back() == L'"') || (value.front() == L'\'' && value.back() == L'\'')))
        {
            value = value.substr(1, value.size() - 2);
        }

        value = Trim(ExpandEnvironment(value));
        if (value.empty())
        {
            return fs::path();
        }

        fs::path result(value);
        if (result.is_relative())
        {
            fs::path base = settingsPath.parent_path();
            if (base.is_relative())
            {
                base = fs::absolute(base);
            }
            result = (base / result).lexically_normal();
        }
        return result;
    }

    return fs::path();
}

static fs::path FindUserDataDirInTree(const fs::path& root)
{
    std::error_code error;
    if (!fs::is_directory(root, error) || error)
    {
        return fs::path();
    }

    for (fs::recursive_directory_iterator iterator(root, fs::directory_options::skip_permission_denied, error), end;
         iterator != end && !error;
         iterator.increment(error))
    {
        if (error)
        {
            error.clear();
            continue;
        }

        std::error_code fileError;
        if (!iterator->is_regular_file(fileError) || fileError)
        {
            continue;
        }

        if (iterator->path().filename() != L"settings.ini")
        {
            continue;
        }

        fs::path found = ReadUserDataDir(iterator->path());
        if (!found.empty())
        {
            return found;
        }
    }

    return fs::path();
}

static fs::path DefaultUserDataDir()
{
    wchar_t buffer[MAX_PATH]{};
    DWORD length = GetEnvironmentVariableW(L"LOCALAPPDATA", buffer, MAX_PATH);
    if (length == 0 || length >= MAX_PATH)
    {
        return fs::path();
    }

    return fs::path(buffer) / L"VEmu";
}

static void InstallCore()
{
    fs::path logPath;

    try
    {
        HMODULE module = nullptr;
        if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                                reinterpret_cast<LPCWSTR>(&InstallCore),
                                &module))
        {
            return;
        }

        wchar_t modulePath[MAX_PATH]{};
        if (GetModuleFileNameW(module, modulePath, MAX_PATH) == 0)
        {
            return;
        }

        fs::path dllPath(modulePath);
        fs::path dllsDir = dllPath.parent_path();
        fs::path modRoot = dllsDir.parent_path();

        wchar_t exePath[MAX_PATH]{};
        DWORD exeLength = GetModuleFileNameW(nullptr, exePath, MAX_PATH);
        if (exeLength == 0 || exeLength >= MAX_PATH)
        {
            return;
        }

        fs::path exeDir = fs::path(exePath).parent_path();
        logPath = exeDir / L"VEmu-CoreInstaller.log";

        {
            std::ofstream test(logPath, std::ios::app | std::ios::binary);
            if (!test)
            {
                logPath = modRoot / L"VEmu-CoreInstaller.log";
            }
        }

        WriteLog(logPath, L"VEmu Core Installer v1.0.0 started");

        fs::path source = modRoot / L"core" / L"kytyps5_libretro.dll";
        if (!fs::exists(source))
        {
            WriteLog(logPath, std::wstring(L"ERROR: bundled core not found: ") + source.wstring());
            return;
        }

        std::wstring sourceHash;
        if (!Sha256File(source, sourceHash))
        {
            WriteLog(logPath, L"ERROR: could not hash bundled core");
            return;
        }

        if (sourceHash != EXPECTED_HASH)
        {
            WriteLog(logPath, std::wstring(L"ERROR: bundled core hash mismatch: ") + sourceHash);
            return;
        }

        WriteLog(logPath, std::wstring(L"Bundled core hash OK: ") + sourceHash);

        fs::path userData = FindUserDataDirInTree(modRoot.parent_path());
        if (userData.empty())
        {
            userData = FindUserDataDirInTree(exeDir / L"Mods");
        }

        if (userData.empty())
        {
            userData = DefaultUserDataDir();
            WriteLog(logPath, L"No user_data_dir setting found; using default LOCALAPPDATA\\VEmu");
        }
        else
        {
            WriteLog(logPath, std::wstring(L"Using VEmu user_data_dir: ") + userData.wstring());
        }

        if (userData.empty())
        {
            WriteLog(logPath, L"ERROR: could not determine VEmu user data directory");
            return;
        }

        fs::path targetDir = userData / L"cores";
        fs::path target = targetDir / L"kytyps5_libretro.dll";

        std::error_code createError;
        fs::create_directories(targetDir, createError);
        if (createError)
        {
            WriteLog(logPath, std::wstring(L"ERROR: could not create cores directory: ") + targetDir.wstring());
            return;
        }

        if (fs::exists(target))
        {
            std::wstring targetHash;
            if (Sha256File(target, targetHash) && targetHash == EXPECTED_HASH)
            {
                WriteLog(logPath, std::wstring(L"Already installed, no changes made: ") + target.wstring());
            }
            else
            {
                WriteLog(logPath, std::wstring(L"DIFFERENT core already exists; not overwriting: ") + target.wstring());
            }
            return;
        }

        if (!CopyFileW(source.c_str(), target.c_str(), TRUE))
        {
            WriteLog(logPath, std::wstring(L"ERROR: copy failed: ") + target.wstring());
            return;
        }

        std::wstring installedHash;
        if (Sha256File(target, installedHash) && installedHash == EXPECTED_HASH)
        {
            WriteLog(logPath, std::wstring(L"SUCCESS: installed ") + target.wstring());
        }
        else
        {
            WriteLog(logPath, std::wstring(L"ERROR: installed file hash mismatch: ") + target.wstring());
        }
    }
    catch (const std::exception& error)
    {
        std::string message(error.what());
        WriteLog(logPath, std::wstring(L"EXCEPTION: ") + std::wstring(message.begin(), message.end()));
    }
    catch (...)
    {
        WriteLog(logPath, L"UNKNOWN EXCEPTION");
    }
}

extern "C" __declspec(dllexport) void* start_mod()
{
    std::call_once(g_once, InstallCore);
    return nullptr;
}

extern "C" __declspec(dllexport) void uninstall_mod(void*)
{
}

BOOL APIENTRY DllMain(HMODULE, DWORD, LPVOID)
{
    return TRUE;
}
