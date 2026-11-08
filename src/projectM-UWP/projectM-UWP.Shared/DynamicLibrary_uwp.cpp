// UWP replacement for projectM's Renderer/Platform/DynamicLibrary_win32.cpp.
//
// The upstream file uses LoadLibraryEx, GetModuleHandle and GetSystemDirectory,
// none of which are available to UWP apps. A UWP process can only load DLLs that
// ship in its own package, which is where the ANGLE libEGL.dll and libGLESv2.dll
// are deployed, so LoadPackagedLibrary is all that is needed.

#include "Renderer/Platform/DynamicLibrary.hpp"
#include "Renderer/Platform/PlatformLibraryNames.hpp"

#include <array>
#include <cstring>
#include <string>
#include <windows.h>

namespace libprojectM {
namespace Renderer {
namespace Platform {

namespace {

auto ProcToSymbol(FARPROC proc) noexcept -> void*
{
    void* symbol = nullptr;
    if (proc != nullptr && sizeof(proc) == sizeof(void*))
    {
        std::memcpy(&symbol, &proc, sizeof(void*));
    }
    return symbol;
}

auto TryLoadPackaged(const char* name) -> HMODULE
{
    // Library names are plain ASCII, so a direct widening is enough.
    const std::string narrow(name);
    const std::wstring wide(narrow.begin(), narrow.end());
    return ::LoadPackagedLibrary(wide.c_str(), 0);
}

} // namespace

auto DynamicLibrary::Open(const char* const* names, std::string& reason) -> bool
{
    Close();

    if (names == nullptr)
    {
        reason = "No library names provided";
        return false;
    }

    for (const char* const* namePtr = names; *namePtr != nullptr; ++namePtr)
    {
        const HMODULE handle = TryLoadPackaged(*namePtr);
        if (handle != nullptr)
        {
            m_handle = static_cast<LibHandle>(handle);
            m_loadedName = *namePtr;
            return true;
        }

        reason = "LoadPackagedLibrary failed for ";
        reason += *namePtr;
        reason += " (";
        reason += std::to_string(static_cast<unsigned long>(::GetLastError()));
        reason += ")";
    }

    return false;
}

void DynamicLibrary::Close()
{
    if (m_handle == nullptr)
    {
        return;
    }

    ::FreeLibrary(static_cast<HMODULE>(m_handle));
    m_handle = nullptr;
    m_loadedName.clear();
}

auto DynamicLibrary::GetSymbol(const char* name) const -> void*
{
    if (m_handle == nullptr || name == nullptr)
    {
        return nullptr;
    }

    return ProcToSymbol(::GetProcAddress(static_cast<HMODULE>(m_handle), name));
}

auto DynamicLibrary::FindGlobalSymbol(const char* name) -> void*
{
    if (name == nullptr)
    {
        return nullptr;
    }

    // UWP cannot enumerate already loaded modules with GetModuleHandle. Loading a
    // packaged DLL again only adds a reference, and these stay loaded for the
    // life of the process, matching the upstream cache.
    static const std::array<HMODULE, 2> modules{TryLoadPackaged("libEGL.dll"),
                                                TryLoadPackaged("libGLESv2.dll")};

    for (const HMODULE module : modules)
    {
        if (module != nullptr)
        {
            if (void* symbol = ProcToSymbol(::GetProcAddress(module, name)))
            {
                return symbol;
            }
        }
    }

    return nullptr;
}

} // namespace Platform
} // namespace Renderer
} // namespace libprojectM
