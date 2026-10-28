#include "game_language_reader.hpp"
#include "game_language_reader_publication.hpp"

#include "../../hooks/signature_scanner.hpp"
#include "../../hooks/signatures/signature_definitions.hpp"
#include "../../plugin/runtime.hpp"

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

    constexpr wchar_t kGameImageName[] = L"Stalker2-Win64-Shipping.exe";

    using ReaderPublication = overlay::detail::GameLanguageReaderPublication<
        ReadLanguageFn, FreeGameBufferFn>;
    ReaderPublication g_readerFunctions;

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

    bool IsExecutableAddress(const void* address, std::size_t size) noexcept
    {
        MEMORY_BASIC_INFORMATION info{};
        if (!address || !size || VirtualQuery(address, &info, sizeof(info)) != sizeof(info) ||
            info.State != MEM_COMMIT || (info.Protect & (PAGE_GUARD | PAGE_NOACCESS)))
            return false;
        const DWORD protection = info.Protect & 0xFF;
        const bool executable = protection == PAGE_EXECUTE ||
            protection == PAGE_EXECUTE_READ || protection == PAGE_EXECUTE_READWRITE ||
            protection == PAGE_EXECUTE_WRITECOPY;
        const auto begin = reinterpret_cast<std::uintptr_t>(address);
        const auto end = begin + size;
        const auto regionEnd = reinterpret_cast<std::uintptr_t>(info.BaseAddress) + info.RegionSize;
        return executable && end >= begin && end <= regionEnd;
    }

    std::uint8_t* ResolveUniqueTarget(HMODULE image, std::uintptr_t imageSize,
        const char* signature, std::size_t signatureSize) noexcept
    {
        try {
            const auto matches = hooks::FindAll(image, signature);
            if (matches.size() != 1) return nullptr;
            auto* match = matches.front();
            const auto base = reinterpret_cast<std::uintptr_t>(image);
            const auto address = reinterpret_cast<std::uintptr_t>(match);
            if (address < base || address - base >= imageSize ||
                signatureSize > imageSize - (address - base) ||
                !IsReadable(match, signatureSize) || !IsExecutableAddress(match, signatureSize))
                return nullptr;
            return match;
        } catch (...) {
            return nullptr;
        }
    }

    std::uint8_t* ResolveUniqueFunctionFromAnchor(HMODULE image,
        std::uintptr_t imageSize, const char* anchor, std::size_t anchorSize) noexcept
    {
        try {
            auto* match = ResolveUniqueTarget(image, imageSize, anchor, anchorSize);
            if (!match) return nullptr;

            const auto base = reinterpret_cast<std::uintptr_t>(image);
            const auto matchAddress = reinterpret_cast<std::uintptr_t>(match);
            if (matchAddress < base || matchAddress - base > imageSize) return nullptr;
            const auto matchRva = matchAddress - base;

            IMAGE_DOS_HEADER dos{};
            if (!ReadSelf(image, &dos, sizeof(dos)) || dos.e_magic != IMAGE_DOS_SIGNATURE ||
                dos.e_lfanew <= 0 || static_cast<std::uintptr_t>(dos.e_lfanew) >
                    imageSize - sizeof(IMAGE_NT_HEADERS64))
                return nullptr;
            IMAGE_NT_HEADERS64 nt{};
            if (!ReadSelf(reinterpret_cast<const void*>(base + dos.e_lfanew), &nt, sizeof(nt)) ||
                nt.Signature != IMAGE_NT_SIGNATURE ||
                nt.OptionalHeader.NumberOfRvaAndSizes <= IMAGE_DIRECTORY_ENTRY_EXCEPTION)
                return nullptr;
            const auto directory = nt.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXCEPTION];
            if (!directory.VirtualAddress || directory.Size < sizeof(RUNTIME_FUNCTION) ||
                directory.VirtualAddress > imageSize ||
                directory.Size > imageSize - directory.VirtualAddress ||
                directory.Size % sizeof(RUNTIME_FUNCTION) != 0)
                return nullptr;

            const auto count = directory.Size / sizeof(RUNTIME_FUNCTION);
            const auto* entries = reinterpret_cast<const RUNTIME_FUNCTION*>(base + directory.VirtualAddress);
            std::size_t low = 0;
            std::size_t high = count;
            while (low < high) {
                const auto middle = low + (high - low) / 2;
                RUNTIME_FUNCTION entry{};
                if (!ReadSelf(entries + middle, &entry, sizeof(entry)) ||
                    entry.BeginAddress >= entry.EndAddress || entry.EndAddress > imageSize)
                    return nullptr;
                if (matchRva < entry.BeginAddress) {
                    high = middle;
                } else if (matchRva >= entry.EndAddress) {
                    low = middle + 1;
                } else {
                    constexpr std::uint8_t expectedPrologue[] = {
                        0x41, 0x57, 0x41, 0x56, 0x41, 0x55, 0x41, 0x54,
                        0x56, 0x57, 0x55, 0x53, 0x48, 0x83, 0xEC, 0x58,
                    };
                    auto* function = reinterpret_cast<std::uint8_t*>(base + entry.BeginAddress);
                    if (anchorSize > entry.EndAddress - matchRva ||
                        !IsReadable(function, sizeof(expectedPrologue)) ||
                        !IsExecutableAddress(function, sizeof(expectedPrologue)))
                        return nullptr;
                    std::array<std::uint8_t, sizeof(expectedPrologue)> actual{};
                    if (!ReadSelf(function, actual.data(), actual.size()) ||
                        std::memcmp(actual.data(), expectedPrologue, sizeof(expectedPrologue)) != 0)
                        return nullptr;
                    return function;
                }
            }
            return nullptr;
        } catch (...) {
            return nullptr;
        }
    }

    bool ValidateImage(HMODULE image, MODULEINFO& moduleInfo) noexcept
    {
        moduleInfo = {};
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
            nt.FileHeader.NumberOfSections > 96 ||
            nt.FileHeader.SizeOfOptionalHeader != sizeof(IMAGE_OPTIONAL_HEADER64) ||
            nt.OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC ||
            nt.OptionalHeader.SizeOfImage != moduleInfo.SizeOfImage)
            return false;
        const auto sectionOffset = static_cast<std::uintptr_t>(dos.e_lfanew) + sizeof(DWORD) +
            sizeof(IMAGE_FILE_HEADER) + nt.FileHeader.SizeOfOptionalHeader;
        const auto sectionBytes = static_cast<std::uintptr_t>(nt.FileHeader.NumberOfSections) *
            sizeof(IMAGE_SECTION_HEADER);
        if (sectionOffset > moduleInfo.SizeOfImage ||
            sectionBytes > moduleInfo.SizeOfImage - sectionOffset)
            return false;
        auto* sections = reinterpret_cast<IMAGE_SECTION_HEADER*>(base + sectionOffset);
        bool hasExecutableSection = false;
        for (WORD i = 0; i < nt.FileHeader.NumberOfSections; ++i) {
            IMAGE_SECTION_HEADER section{};
            if (!ReadSelf(sections + i, &section, sizeof(section))) return false;
            if ((section.Characteristics & IMAGE_SCN_MEM_EXECUTE) == 0) continue;
            const auto sectionSize = static_cast<std::uintptr_t>(section.Misc.VirtualSize);
            if (section.VirtualAddress > moduleInfo.SizeOfImage ||
                sectionSize > moduleInfo.SizeOfImage - section.VirtualAddress)
                return false;
            hasExecutableSection = hasExecutableSection || sectionSize != 0;
        }
        return hasExecutableSection;
    }

    bool InitializeOnce() noexcept
    {
        ReaderPublication::FunctionBundle published{};
        if (g_readerFunctions.TryRead(published)) return true;
        if (!g_readerFunctions.TryBeginInitialization()) return false;
        HMODULE image = GetModuleHandleW(kGameImageName);
        MODULEINFO moduleInfo{};
        if (!image || !ValidateImage(image, moduleInfo)) {
            g_readerFunctions.RetryAfterFailure();
            return false;
        }
        auto* reader = ResolveUniqueTarget(image, moduleInfo.SizeOfImage,
            hooks::signatures::GameLanguageReader,
            sizeof(hooks::signatures::GameLanguageReader) - 1);
        auto* freeBuffer = ResolveUniqueFunctionFromAnchor(image, moduleInfo.SizeOfImage,
            hooks::signatures::GameLanguageBufferFreeAnchor,
            sizeof(hooks::signatures::GameLanguageBufferFreeAnchor) - 1);
        if (!reader || !freeBuffer) {
            g_readerFunctions.RetryAfterFailure();
            return false;
        }
        const ReaderPublication::FunctionBundle functions{
            reinterpret_cast<ReadLanguageFn>(reader),
            reinterpret_cast<FreeGameBufferFn>(freeBuffer)};
        if (!g_readerFunctions.TryPublish(functions)) {
            g_readerFunctions.RetryAfterFailure();
            return false;
        }
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
        ReaderPublication::FunctionBundle functions{};
        if (!g_readerFunctions.TryRead(functions)) return false;
        FStringLayout value{};
        bool returned = false;
        __try { returned = functions.reader(&value) == &value; }
        __except (EXCEPTION_EXECUTE_HANDLER) { returned = false; }
        const bool plausible = returned && value.data && value.count >= 1 && value.count <= 64 &&
            value.capacity >= value.count && IsReadable(value.data,
                static_cast<std::size_t>(value.count) * sizeof(wchar_t));
        const bool copied = plausible && CopyLocale(value, output, capacity);
        if (plausible) {
            __try { functions.free(value.data); }
            __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
        }
        return copied;
    }
}

void overlay::SynchronizeAutoLanguageOnOverlayOpen(HWND window,
    bool overlayVisible) noexcept
{
    if (!ShouldSynchronizeAutoLanguage(overlayVisible,
            plugin::GetRuntimeSettingsSnapshot().overlayLocaleAuto)) return;
    if (!RequestAutoLanguageSynchronization(window))
        plugin::SetDetectedOverlayLocale("en");
}

bool overlay::ReadGameLanguageLocale(char* output,
    std::size_t capacity) noexcept
{
    if (!output || capacity < 3) return false;
    output[0] = '\0';
    if (!InitializeOnce() || !ReadLocale(output, capacity)) {
        output[0] = 'e';
        output[1] = 'n';
        output[2] = '\0';
        return false;
    }
    return true;
}

void overlay::ApplyDetectedGameLanguageLocale(const char* localeCode,
    bool readSucceeded) noexcept
{
    if (!plugin::GetRuntimeSettingsSnapshot().overlayLocaleAuto) return;
    plugin::SetDetectedOverlayLocale(readSucceeded && localeCode
        ? localeCode : "en");
}

bool overlay::RequestAutoLanguageSynchronization(HWND window) noexcept
{
    return window && PostMessageW(window, AutoLanguageSyncMessage, 0, 0) != FALSE;
}

bool overlay::RequestStartupGameDataProbe(HWND window) noexcept
{
    return window && PostMessageW(window, StartupGameDataProbeMessage, 0, 0) != FALSE;
}
