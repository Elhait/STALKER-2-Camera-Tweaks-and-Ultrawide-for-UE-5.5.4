#include "game_language_reader.hpp"

#include "../localization/locale_registry.hpp"
#include "../plugin/runtime.hpp"

#include <Windows.h>
#include <Psapi.h>

#include <array>
#include <cstdint>
#include <cstring>

namespace
{
    struct FStringLayout
    {
        wchar_t* data{};
        std::int32_t count{};
        std::int32_t capacity{};
    };

    using ReadLanguageFn = FStringLayout* (__fastcall*)(FStringLayout*);
    using FreeGameBufferFn = void (__fastcall*)(void*);

    constexpr std::uintptr_t kReaderRva = 0x057AF550;
    constexpr std::uintptr_t kFreeRva = 0x020C40BA;
    constexpr std::uint8_t kReaderPrologue[]{
        0x41, 0x56, 0x56, 0x57, 0x53, 0x48, 0x83, 0xEC, 0x28, 0x48, 0x89, 0xCE
    };
    constexpr std::uint8_t kFreePrologue[]{
        0x41, 0x57, 0x41, 0x56, 0x41, 0x55, 0x41, 0x54,
        0x56, 0x57, 0x55, 0x53, 0x48, 0x83, 0xEC, 0x58
    };
    constexpr wchar_t kGameImageName[] = L"Stalker2-Win64-Shipping.exe";

    ReadLanguageFn g_reader{};
    FreeGameBufferFn g_free{};
    bool g_initialized{};

    bool ReadSelf(const void* source, void* destination, std::size_t size) noexcept
    {
        SIZE_T read{};
        return source && destination && size &&
            ReadProcessMemory(GetCurrentProcess(), source, destination, size, &read) && read == size;
    }

    bool IsReadable(const void* address, std::size_t size) noexcept
    {
        MEMORY_BASIC_INFORMATION info{};
        if (!address || !size || VirtualQuery(address, &info, sizeof(info)) != sizeof(info) ||
            info.State != MEM_COMMIT || (info.Protect & (PAGE_GUARD | PAGE_NOACCESS)))
            return false;
        const auto begin = reinterpret_cast<std::uintptr_t>(address);
        const auto end = begin + size;
        const auto regionEnd = reinterpret_cast<std::uintptr_t>(info.BaseAddress) + info.RegionSize;
        return end >= begin && end <= regionEnd;
    }

    bool ValidateTarget(HMODULE image, std::uintptr_t imageSize, std::uintptr_t rva,
        const std::uint8_t* expected, std::size_t size) noexcept
    {
        if (!image || rva < 0x1000 || rva >= imageSize || size > imageSize - rva)
            return false;
        const auto* address = reinterpret_cast<const std::uint8_t*>(image) + rva;
        std::array<std::uint8_t, 16> actual{};
        return size <= actual.size() && IsReadable(address, size) &&
            ReadSelf(address, actual.data(), size) && std::memcmp(actual.data(), expected, size) == 0;
    }

    bool ValidateImage(HMODULE image) noexcept
    {
        MODULEINFO moduleInfo{};
        if (!image || !GetModuleInformation(GetCurrentProcess(), image,
                &moduleInfo, sizeof(moduleInfo)))
            return false;
        auto* base = reinterpret_cast<std::uint8_t*>(image);
        IMAGE_DOS_HEADER dos{};
        if (!ReadSelf(base, &dos, sizeof(dos)) || dos.e_magic != IMAGE_DOS_SIGNATURE ||
            dos.e_lfanew <= 0 || dos.e_lfanew > 0x100000 ||
            moduleInfo.SizeOfImage < sizeof(IMAGE_NT_HEADERS64) ||
            static_cast<std::uintptr_t>(dos.e_lfanew) > moduleInfo.SizeOfImage - sizeof(IMAGE_NT_HEADERS64))
            return false;
        IMAGE_NT_HEADERS64 nt{};
        if (!ReadSelf(base + dos.e_lfanew, &nt, sizeof(nt)) || nt.Signature != IMAGE_NT_SIGNATURE ||
            nt.FileHeader.Machine != IMAGE_FILE_MACHINE_AMD64 || nt.FileHeader.NumberOfSections == 0 ||
            nt.FileHeader.NumberOfSections > 96 || nt.FileHeader.SizeOfOptionalHeader != sizeof(IMAGE_OPTIONAL_HEADER64))
            return false;
        const auto sectionOffset = static_cast<std::uintptr_t>(dos.e_lfanew) + sizeof(DWORD) +
            sizeof(IMAGE_FILE_HEADER) + nt.FileHeader.SizeOfOptionalHeader;
        const auto sectionBytes = static_cast<std::uintptr_t>(nt.FileHeader.NumberOfSections) *
            sizeof(IMAGE_SECTION_HEADER);
        if (sectionOffset > moduleInfo.SizeOfImage ||
            sectionBytes > moduleInfo.SizeOfImage - sectionOffset)
            return false;
        auto* sections = reinterpret_cast<IMAGE_SECTION_HEADER*>(base + sectionOffset);
        bool textMatches = false;
        for (WORD i = 0; i < nt.FileHeader.NumberOfSections; ++i) {
            IMAGE_SECTION_HEADER section{};
            if (!ReadSelf(sections + i, &section, sizeof(section))) return false;
            if (std::memcmp(section.Name, ".text", 5) != 0) continue;
            textMatches = (section.Characteristics & IMAGE_SCN_MEM_EXECUTE) != 0 &&
                section.VirtualAddress == 0x1000 && section.SizeOfRawData == 0x7CCC200;
            break;
        }
        return textMatches &&
            ValidateTarget(image, moduleInfo.SizeOfImage, kReaderRva,
                kReaderPrologue, sizeof(kReaderPrologue)) &&
            ValidateTarget(image, moduleInfo.SizeOfImage, kFreeRva,
                kFreePrologue, sizeof(kFreePrologue));
    }

    bool InitializeOnce() noexcept
    {
        if (g_initialized) return g_reader != nullptr;
        HMODULE image = GetModuleHandleW(kGameImageName);
        if (!image || !ValidateImage(image)) return false;
        g_reader = reinterpret_cast<ReadLanguageFn>(
            reinterpret_cast<std::uintptr_t>(image) + kReaderRva);
        g_free = reinterpret_cast<FreeGameBufferFn>(
            reinterpret_cast<std::uintptr_t>(image) + kFreeRva);
        g_initialized = true;
        return true;
    }

    bool CopyLocale(const FStringLayout& value, char* output, std::size_t capacity) noexcept
    {
        if (!output || capacity < 2 || value.count < 1 || value.count > 64 ||
            value.capacity < value.count || !value.data)
            return false;
        const auto byteCount = static_cast<std::size_t>(value.count) * sizeof(wchar_t);
        if (!IsReadable(value.data, byteCount)) return false;
        std::array<wchar_t, 64> wide{};
        if (!ReadSelf(value.data, wide.data(), byteCount)) return false;
        std::size_t length = static_cast<std::size_t>(value.count);
        while (length && wide[length - 1] == L'\0') --length;
        if (!length || length >= capacity) return false;
        for (std::size_t i = 0; i < length; ++i) {
            if (wide[i] < 0x21 || wide[i] > 0x7e) return false;
            output[i] = static_cast<char>(wide[i]);
        }
        output[length] = '\0';
        return true;
    }

    bool ReadLocale(char* output, std::size_t capacity) noexcept
    {
        FStringLayout value{};
        bool returned = false;
        __try { returned = g_reader(&value) == &value; }
        __except (EXCEPTION_EXECUTE_HANDLER) { returned = false; }
        const bool plausible = returned && value.data && value.count >= 1 && value.count <= 64 &&
            value.capacity >= value.count && IsReadable(value.data,
                static_cast<std::size_t>(value.count) * sizeof(wchar_t));
        const bool copied = plausible && CopyLocale(value, output, capacity);
        if (plausible) {
            __try { g_free(value.data); }
            __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
        }
        return copied;
    }
}

void overlay::SynchronizeAutoLanguageOnOverlayOpen(bool overlayVisible) noexcept
{
    if (!ShouldSynchronizeAutoLanguage(overlayVisible,
            plugin::GetRuntimeSettingsSnapshot().overlayLocaleAuto)) return;
    SynchronizeAutoLanguageIfConfigured();
}

void overlay::SynchronizeAutoLanguageIfConfigured() noexcept
{
    if (!plugin::GetRuntimeSettingsSnapshot().overlayLocaleAuto) return;
    char locale[64]{};
    if (!InitializeOnce() || !ReadLocale(locale, sizeof(locale))) {
        plugin::SetDetectedOverlayLocale("en");
        return;
    }
    plugin::SetDetectedOverlayLocale(locale);
}

bool overlay::RequestAutoLanguageSynchronization(HWND window) noexcept
{
    return window && PostMessageW(window, AutoLanguageSyncMessage, 0, 0) != FALSE;
}
