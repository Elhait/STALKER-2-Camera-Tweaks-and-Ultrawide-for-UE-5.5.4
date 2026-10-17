#include "../../src/config/config_repository.hpp"
#include "../../src/config/config_line_reader.hpp"
#include "../../src/config/config_template.hpp"
#include "canonical_ini_documentation.hpp"

#include <Windows.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <istream>
#include <string>
#include <string_view>
#include <streambuf>
#include <vector>

namespace
{
    class FailingReadBuffer final : public std::streambuf
    {
    public:
        explicit FailingReadBuffer(std::string content, std::size_t failAt)
            : content_(std::move(content)), failAt_(failAt) {}

    protected:
        int_type underflow() override
        {
            if (position_ >= failAt_) throw std::ios_base::failure("injected read failure");
            if (position_ >= content_.size()) return traits_type::eof();
            return traits_type::to_int_type(content_[position_]);
        }

        int_type uflow() override
        {
            const auto value = underflow();
            if (!traits_type::eq_int_type(value, traits_type::eof())) ++position_;
            return value;
        }

    private:
        std::string content_;
        std::size_t failAt_{};
        std::size_t position_{};
    };

    bool TestMidReadFailureIsRejected()
    {
        FailingReadBuffer buffer("[Overlay]\nPositionX=60\ncustom-tail=keep\n", 23);
        std::istream input(&buffer);
        std::vector<std::string> partial;
        const bool complete = config::detail::ReadConfigLines(input, partial);
        return !complete && partial.size() == 2 &&
            partial[0] == "[Overlay]" && partial[1] == "PositionX=60";
    }

    std::string ReadText(const std::filesystem::path& path)
    {
        std::ifstream input(path, std::ios::binary);
        return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
    }

    struct FileIdentityAndTime
    {
        DWORD volumeSerial{};
        DWORD fileIndexHigh{};
        DWORD fileIndexLow{};
        FILETIME lastWrite{};
    };

    bool ReadFileIdentityAndTime(const std::filesystem::path& path,
        FileIdentityAndTime& result)
    {
        const auto handle = CreateFileW(path.c_str(), FILE_READ_ATTRIBUTES,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
            OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (handle == INVALID_HANDLE_VALUE) return false;
        BY_HANDLE_FILE_INFORMATION information{};
        const bool success = GetFileInformationByHandle(handle, &information) != FALSE;
        CloseHandle(handle);
        if (!success) return false;
        result.volumeSerial = information.dwVolumeSerialNumber;
        result.fileIndexHigh = information.nFileIndexHigh;
        result.fileIndexLow = information.nFileIndexLow;
        result.lastWrite = information.ftLastWriteTime;
        return true;
    }

    bool SameFileIdentityAndTime(const FileIdentityAndTime& left,
        const FileIdentityAndTime& right)
    {
        return left.volumeSerial == right.volumeSerial &&
            left.fileIndexHigh == right.fileIndexHigh &&
            left.fileIndexLow == right.fileIndexLow &&
            left.lastWrite.dwHighDateTime == right.lastWrite.dwHighDateTime &&
            left.lastWrite.dwLowDateTime == right.lastWrite.dwLowDateTime;
    }

    bool HasAssignment(const std::string& text, std::string_view key,
        std::string_view value)
    {
        std::size_t start = 0;
        while (start < text.size()) {
            const auto end = text.find('\n', start);
            const auto lineEnd = end == std::string::npos ? text.size() : end;
            const auto line = config::Trim(text.substr(start, lineEnd - start));
            const auto separator = line.find('=');
            if (separator != std::string::npos &&
                config::Trim(line.substr(0, separator)) == key &&
                config::Trim(line.substr(separator + 1)) == value) return true;
            if (end == std::string::npos) break;
            start = end + 1;
        }
        return false;
    }

    bool HasSectionAssignment(const std::string& text, std::string_view section,
        std::string_view key, std::string_view value)
    {
        const std::string header = "[" + std::string(section) + "]";
        const auto start = text.find(header);
        if (start == std::string::npos) return false;
        const auto bodyStart = start + header.size();
        const auto nextSection = text.find("\n[", bodyStart);
        return HasAssignment(text.substr(bodyStart, nextSection == std::string::npos
            ? std::string::npos : nextSection - bodyStart), key, value);
    }

    std::size_t AssignmentLineStart(const std::string& text, std::string_view key)
    {
        std::size_t start = 0;
        while (start < text.size()) {
            const auto end = text.find('\n', start);
            const auto lineEnd = end == std::string::npos ? text.size() : end;
            const auto line = config::Trim(text.substr(start, lineEnd - start));
            const auto separator = line.find('=');
            if (separator != std::string::npos &&
                config::Trim(line.substr(0, separator)) == key) return start;
            if (end == std::string::npos) break;
            start = end + 1;
        }
        return std::string::npos;
    }

    bool HasBlankLineBeforeAssignment(const std::string& text, std::string_view key)
    {
        auto blockStart = AssignmentLineStart(text, key);
        if (blockStart == std::string::npos || blockStart == 0) return false;
        for (;;) {
            const auto previousLineEnd = blockStart - 1;
            const auto previousLineStart = text.rfind('\n', previousLineEnd == 0
                ? 0 : previousLineEnd - 1);
            const auto lineStart = previousLineStart == std::string::npos
                ? 0 : previousLineStart + 1;
            const auto previous = config::Trim(text.substr(
                lineStart, previousLineEnd - lineStart));
            if (previous.empty()) return true;
            if (previous.front() != '#' && previous.front() != ';') return false;
            if (lineStart == 0) return false;
            blockStart = lineStart;
        }
    }

    bool AssignmentsAreAdjacent(const std::string& text, std::string_view first,
        std::string_view second)
    {
        const auto firstStart = AssignmentLineStart(text, first);
        const auto secondStart = AssignmentLineStart(text, second);
        if (firstStart == std::string::npos || secondStart == std::string::npos) return false;
        const auto firstEnd = text.find('\n', firstStart);
        return firstEnd != std::string::npos && firstEnd + 1 == secondStart;
    }

    bool HasExactlyTwoBlankLinesBeforeSection(const std::string& text,
        std::string_view section)
    {
        const auto position = text.find(section);
        if (position == std::string::npos || position == 0) return false;
        auto cursor = position;
        std::size_t newlineCount = 0;
        while (cursor >= 2 && text[cursor - 1] == '\n' &&
            text[cursor - 2] == '\r') {
            ++newlineCount;
            cursor -= 2;
        }
        return newlineCount == 3 && cursor > 0 && text[cursor - 1] != '\n' &&
            text[cursor - 1] != '\r';
    }

    bool HasCompactAssignment(const std::string& text, std::string_view key,
        std::string_view value)
    {
        const auto start = AssignmentLineStart(text, key);
        if (start == std::string::npos) return false;
        const auto end = text.find('\n', start);
        if (end == std::string::npos) return false;
        auto line = config::Trim(text.substr(start, end - start));
        if (!line.empty() && line.back() == '\r') line.pop_back();
        return line == std::string(key) + " = " + std::string(value);
    }

    bool AllAssignmentsCompact(const std::string& text)
    {
        std::size_t start = 0;
        while (start < text.size()) {
            const auto end = text.find('\n', start);
            const auto lineEnd = end == std::string::npos ? text.size() : end;
            const auto line = config::Trim(text.substr(start, lineEnd - start));
            if (!line.empty() && line.front() != '#' && line.front() != ';' &&
                line.front() != '[') {
                const auto separator = line.find('=');
                if (separator != std::string::npos) {
                    const auto key = config::Trim(line.substr(0, separator));
                    const auto value = config::Trim(line.substr(separator + 1));
                    if (line != key + " = " + value) return false;
                }
            }
            if (end == std::string::npos) break;
            start = end + 1;
        }
        return true;
    }

    std::size_t CountAssignments(const std::string& text, std::string_view key,
        std::string_view value)
    {
        std::size_t count = 0;
        std::size_t start = 0;
        while (start < text.size()) {
            const auto end = text.find('\n', start);
            const auto lineEnd = end == std::string::npos ? text.size() : end;
            const auto line = config::Trim(text.substr(start, lineEnd - start));
            const auto separator = line.find('=');
            if (separator != std::string::npos &&
                config::Trim(line.substr(0, separator)) == key &&
                config::Trim(line.substr(separator + 1)) == value) ++count;
            if (end == std::string::npos) break;
            start = end + 1;
        }
        return count;
    }

    template <std::size_t Count>
    bool ContainsGeneratedComments(const std::string& text,
        const std::string_view (&comments)[Count])
    {
        for (const auto comment : comments)
            if (comment != ";" && text.find(comment) == std::string::npos) return false;
        return true;
    }

    template <std::size_t Count>
    bool HasTrueAndFalseDocumentation(const std::string_view (&comments)[Count])
    {
        std::size_t trueCount = 0;
        std::size_t falseCount = 0;
        for (const auto comment : comments) {
            if (comment.starts_with("# true - ")) ++trueCount;
            if (comment.starts_with("# false - ")) ++falseCount;
        }
        return trueCount == 1 && falseCount == 1;
    }

    bool WriteText(const std::filesystem::path& path, const std::string& value)
    {
        std::ofstream output(path, std::ios::binary | std::ios::trunc);
        output << value;
        return static_cast<bool>(output);
    }

    std::filesystem::path MakeTestDirectory()
    {
        const auto directory = std::filesystem::current_path() /
            L"build-artifacts" / L"tests" / L"temp" /
            (L"STALKER2ConfigPersistenceHarness-" + std::to_wstring(GetCurrentProcessId()));
        std::error_code error;
        std::filesystem::remove_all(directory, error);
        std::filesystem::create_directories(directory, error);
        return error ? std::filesystem::path{} : directory;
    }

    bool Persist(const std::filesystem::path& path, const char* value)
    {
        return config::PersistConfigValue(path, "Gameplay", "Enabled", value,
            [](std::string) {});
    }

    bool Synchronize(const std::filesystem::path& path)
    {
        return config::SynchronizeManagedConfigTemplate(path,
            [](std::string) {});
    }

    bool TestStagingNameOwnership(const std::filesystem::path& directory)
    {
        const auto path = directory / L"owned-staging.ini";
        const auto staging = std::filesystem::path(path.wstring() + L".tmp");
        const auto victim = directory / L"unrelated.txt";
        const std::string original = "[Gameplay]\nEnabled=true\n";
        const std::string sentinel = "unrelated data must survive";
        if (!WriteText(path, original) || !WriteText(staging, sentinel)) return false;
        const bool occupied = !Persist(path, "false") && !Synchronize(path) &&
            ReadText(path) == original && ReadText(staging) == sentinel;
        if (!DeleteFileW(staging.c_str()) || !WriteText(victim, sentinel) ||
            !CreateHardLinkW(staging.c_str(), victim.c_str(), nullptr)) {
            std::printf("hardlink_fixture_setup_error=%lu occupied=%d\n", GetLastError(), occupied);
            return false;
        }
        const bool linked = !Persist(path, "false") && !Synchronize(path) &&
            ReadText(path) == original && ReadText(victim) == sentinel &&
            ReadText(staging) == sentinel;
        // The normal log-open path must also refuse to truncate a hard link.
        platform::win32::NativeFile log;
        const bool writeRefused = !log.Open(staging.c_str(), true) &&
            ReadText(victim) == sentinel;
        std::printf("staging_ownership_details occupied=%d linked=%d write_refused=%d\n",
            occupied, linked, writeRefused);
        return occupied && linked && writeRefused;
    }

    bool TestOversizedConfigIsNotReadOrRewritten(const std::filesystem::path& directory)
    {
        const auto path = directory / L"oversized.ini";
        const HANDLE file = CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr,
            CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (file == INVALID_HANDLE_VALUE) return false;
        LARGE_INTEGER size{};
        size.QuadPart = 4u * 1024u * 1024u + 1u;
        const bool created = SetFilePointerEx(file, size, nullptr, FILE_BEGIN) &&
            SetEndOfFile(file);
        CloseHandle(file);
        FileIdentityAndTime before{}, after{};
        std::string retained = "unchanged";
        const bool refused = created && ReadFileIdentityAndTime(path, before) &&
            !platform::win32::ReadNativeFile(path.c_str(), retained) &&
            retained == "unchanged" && !Synchronize(path) &&
            !Persist(path, "false") && ReadFileIdentityAndTime(path, after) &&
            SameFileIdentityAndTime(before, after);
        return refused;
    }

    bool TestCanonicalInitialConfigDocumentation(const std::filesystem::path& path)
    {
        config::FeatureConfig configuration{};
        const auto synchronizer = [](const std::filesystem::path& target) {
            return Synchronize(target);
        };
        if (!config::LoadFeatureConfig(path, configuration, synchronizer,
            [](std::string) {})) return false;

        const auto initial = ReadText(path);
        const auto cinematicsStart = initial.find("[Cinematics]");
        const auto dialogueStart = initial.find("[Dialogue]");
        const auto diagnosticsStart = initial.find("[Diagnostics]");
        const auto cinematicComments = cinematicsStart != std::string::npos &&
            dialogueStart != std::string::npos
            ? initial.substr(cinematicsStart, dialogueStart - cinematicsStart) : std::string{};
        const auto dialogueComments = dialogueStart != std::string::npos &&
            diagnosticsStart != std::string::npos
            ? initial.substr(dialogueStart, diagnosticsStart - dialogueStart) : std::string{};
        const auto hotkeysStart = initial.find("[Hotkeys]");
        const auto overlayStart = initial.find("[Overlay]");
        const auto hotkeyComments = hotkeysStart != std::string::npos &&
            overlayStart != std::string::npos
            ? initial.substr(hotkeysStart, overlayStart - hotkeysStart) : std::string{};
        const auto overlaySettings = overlayStart != std::string::npos &&
            hotkeysStart != std::string::npos
            ? initial.substr(overlayStart, hotkeysStart - overlayStart) : std::string{};
        const auto dialogueCycle = config::NextDialogueZoomPolicy;
        const bool dialogueCycleContract =
            dialogueCycle(config::DialogueZoomPolicy::Native) ==
                config::DialogueZoomPolicy::Adaptive &&
            dialogueCycle(config::DialogueZoomPolicy::Adaptive) ==
                config::DialogueZoomPolicy::Reduced &&
            dialogueCycle(config::DialogueZoomPolicy::Reduced) ==
                config::DialogueZoomPolicy::Disabled &&
            dialogueCycle(config::DialogueZoomPolicy::Disabled) ==
                config::DialogueZoomPolicy::Adaptive;
        const bool canonical = initial.find(
                "# STALKER 2 CAMERA TWEAKS") != std::string::npos &&
            configuration.gameplayMode == config::GameplayMode::RuntimeCombine &&
            initial.find("Enabled = true") != std::string::npos &&
            initial.find("[Overlay]") != std::string::npos &&
            HasExactlyTwoBlankLinesBeforeSection(initial, "[Gameplay]") &&
            HasExactlyTwoBlankLinesBeforeSection(initial, "[Cinematics]") &&
            HasExactlyTwoBlankLinesBeforeSection(initial, "[Dialogue]") &&
            HasExactlyTwoBlankLinesBeforeSection(initial, "[Diagnostics]") &&
            HasExactlyTwoBlankLinesBeforeSection(initial, "[Overlay]") &&
            HasExactlyTwoBlankLinesBeforeSection(initial, "[Hotkeys]") &&
            AllAssignmentsCompact(initial) &&
            HasCompactAssignment(overlaySettings, "Enabled", "true") &&
            HasCompactAssignment(overlaySettings, "ToggleKey", "VK_2E") &&
            HasCompactAssignment(overlaySettings, "Language", "Auto") &&
            HasCompactAssignment(overlaySettings, "FontSize", "13") &&
            HasCompactAssignment(overlaySettings, "PositionX", "30") &&
            HasCompactAssignment(overlaySettings, "PositionY", "30") &&
            HasCompactAssignment(overlaySettings, "PositionMigrationComplete", "0") &&
            initial.find("# STALKER 2 Camera Tweaks and Ultrawide") == std::string::npos &&
            initial.find("STALKER2-MANAGED-DOC") == std::string::npos &&
            initial.find("# <Gameplay.Enabled>") != std::string::npos &&
            initial.find("# </Gameplay.Enabled>") != std::string::npos &&
            initial.find("\n;") == std::string::npos &&
            initial.find("v1.0.0") == std::string::npos &&
            ContainsGeneratedComments(initial,
                config::canonical_ini_documentation::GameplayEnabledComments) &&
            ContainsGeneratedComments(initial,
                config::canonical_ini_documentation::GameplayModeComments) &&
            ContainsGeneratedComments(initial,
                config::canonical_ini_documentation::CinematicAspectComments) &&
            ContainsGeneratedComments(initial,
                config::canonical_ini_documentation::CinematicEnabledComments) &&
            ContainsGeneratedComments(initial,
                config::canonical_ini_documentation::CinematicFovComments) &&
            ContainsGeneratedComments(initial,
                config::canonical_ini_documentation::DialogueEnabledComments) &&
            ContainsGeneratedComments(initial,
                config::canonical_ini_documentation::DialogueZoomComments) &&
            ContainsGeneratedComments(initial,
                config::canonical_ini_documentation::DiagnosticsEnabledComments) &&
            ContainsGeneratedComments(initial,
                config::canonical_ini_documentation::OverlayEnabledComments) &&
            HasTrueAndFalseDocumentation(
                config::canonical_ini_documentation::GameplayEnabledComments) &&
            HasTrueAndFalseDocumentation(
                config::canonical_ini_documentation::CinematicEnabledComments) &&
            HasTrueAndFalseDocumentation(
                config::canonical_ini_documentation::DialogueEnabledComments) &&
            HasTrueAndFalseDocumentation(
                config::canonical_ini_documentation::DiagnosticsEnabledComments) &&
            HasTrueAndFalseDocumentation(
                config::canonical_ini_documentation::HotkeysEnabledComments) &&
            HasTrueAndFalseDocumentation(
                config::canonical_ini_documentation::OverlayEnabledComments) &&
            initial.find("# Example: Example:") == std::string::npos &&
            cinematicComments.find("#   Native -") == std::string::npos &&
            dialogueComments.find("#   Native -") == std::string::npos &&
            dialogueCycleContract &&
            hotkeyComments.find("Adaptive -> Reduced -> Disabled -> Adaptive") !=
                std::string::npos &&
            hotkeyComments.find("Native -> Adaptive -> Reduced -> Disabled -> Native") ==
                std::string::npos &&
            HasBlankLineBeforeAssignment(initial, "Mode") &&
            HasBlankLineBeforeAssignment(initial, "FovMode") &&
            HasBlankLineBeforeAssignment(initial, "ToggleKey") &&
            HasBlankLineBeforeAssignment(initial, "Language") &&
            HasBlankLineBeforeAssignment(initial, "FontSize") &&
            HasBlankLineBeforeAssignment(initial, "PositionX") &&
            HasBlankLineBeforeAssignment(initial, "PositionMigrationComplete") &&
            AssignmentsAreAdjacent(initial, "PositionX", "PositionY") &&
            ContainsGeneratedComments(initial,
                config::canonical_ini_documentation::HotkeysEnabledComments) &&
            ContainsGeneratedComments(initial,
                config::canonical_ini_documentation::HotkeyGameplayCycleComments) &&
            ContainsGeneratedComments(initial,
                config::canonical_ini_documentation::HotkeyCinematicAspectCycleComments) &&
            ContainsGeneratedComments(initial,
                config::canonical_ini_documentation::HotkeyCinematicFovCycleComments) &&
            ContainsGeneratedComments(initial,
                config::canonical_ini_documentation::HotkeyDialogueCycleComments) &&
            ContainsGeneratedComments(initial,
                config::canonical_ini_documentation::OverlayToggleComments) &&
            std::size(config::canonical_ini_documentation::OverlayToggleComments) == 4 &&
            config::canonical_ini_documentation::OverlayToggleComments[0].substr(2).find(
                config::canonical_ini_documentation::Find("tooltip.overlay.always_active")) == 0 &&
            config::canonical_ini_documentation::OverlayToggleComments[1] ==
                config::canonical_ini_documentation::HotkeyGameplayCycleComments[1] &&
            config::canonical_ini_documentation::OverlayToggleComments[2] ==
                config::canonical_ini_documentation::HotkeyGameplayCycleComments[2];
        if (!canonical) std::printf("canonical_initial_contract=FAIL\n");
        const bool secondSyncChanged = Synchronize(path);
        if (secondSyncChanged) std::printf("canonical_initial_second_sync_changed=1\n");
        if (!canonical || secondSyncChanged) return false;
        const auto final = ReadText(path);
        if (final != initial) std::printf("canonical_initial_bytes_changed=1\n");
        return final == initial;
    }

    bool TestBooleanDocumentationResyncPreservesValues(
        const std::filesystem::path& path)
    {
        const std::string oldDocumentation =
            "[Gameplay]\n# <STALKER2-MANAGED-DOC Gameplay.Enabled>\n"
            "# old boolean help\n# </STALKER2-MANAGED-DOC Gameplay.Enabled>\n"
            "Enabled=false\n"
            "[Cinematics]\n# <STALKER2-MANAGED-DOC Cinematics.Enabled>\n"
            "# old boolean help\n# </STALKER2-MANAGED-DOC Cinematics.Enabled>\n"
            "Enabled=false\n"
            "[Dialogue]\n# <STALKER2-MANAGED-DOC Dialogue.Enabled>\n"
            "# old boolean help\n# </STALKER2-MANAGED-DOC Dialogue.Enabled>\n"
            "Enabled=false\n"
            "[Diagnostics]\n# <STALKER2-MANAGED-DOC Diagnostics.Enabled>\n"
            "# old boolean help\n# </STALKER2-MANAGED-DOC Diagnostics.Enabled>\n"
            "Enabled=true\n"
            "[Overlay]\n# <STALKER2-MANAGED-DOC Overlay.Enabled>\n"
            "# old boolean help\n# </STALKER2-MANAGED-DOC Overlay.Enabled>\n"
            "Enabled=false\n"
            "[Hotkeys]\n# <STALKER2-MANAGED-DOC Hotkeys.Enabled>\n"
            "# old boolean help\n# </STALKER2-MANAGED-DOC Hotkeys.Enabled>\n"
            "Enabled=true\n";
        if (!WriteText(path, oldDocumentation) || !Synchronize(path)) return false;

        const auto migrated = ReadText(path);
        const bool valuesPreserved =
            HasSectionAssignment(migrated, "Gameplay", "Enabled", "false") &&
            HasSectionAssignment(migrated, "Cinematics", "Enabled", "false") &&
            HasSectionAssignment(migrated, "Dialogue", "Enabled", "false") &&
            HasSectionAssignment(migrated, "Diagnostics", "Enabled", "true") &&
            HasSectionAssignment(migrated, "Overlay", "Enabled", "false") &&
            HasSectionAssignment(migrated, "Hotkeys", "Enabled", "true");
        const bool docsReplaced = migrated.find("old boolean help") == std::string::npos &&
            ContainsGeneratedComments(migrated,
                config::canonical_ini_documentation::GameplayEnabledComments) &&
            ContainsGeneratedComments(migrated,
                config::canonical_ini_documentation::CinematicEnabledComments) &&
            ContainsGeneratedComments(migrated,
                config::canonical_ini_documentation::DialogueEnabledComments) &&
            ContainsGeneratedComments(migrated,
                config::canonical_ini_documentation::DiagnosticsEnabledComments) &&
            ContainsGeneratedComments(migrated,
                config::canonical_ini_documentation::HotkeysEnabledComments) &&
            ContainsGeneratedComments(migrated,
                config::canonical_ini_documentation::OverlayEnabledComments);
        const bool compactMarkers = migrated.find("STALKER2-MANAGED-DOC") ==
                std::string::npos &&
            migrated.find("# <Gameplay.Enabled>") != std::string::npos &&
            migrated.find("# </Gameplay.Enabled>") != std::string::npos &&
            migrated.find("# <Cinematics.Enabled>") != std::string::npos &&
            migrated.find("# </Cinematics.Enabled>") != std::string::npos &&
            migrated.find("# <Dialogue.Enabled>") != std::string::npos &&
            migrated.find("# </Dialogue.Enabled>") != std::string::npos &&
            migrated.find("# <Diagnostics.Enabled>") != std::string::npos &&
            migrated.find("# </Diagnostics.Enabled>") != std::string::npos &&
            migrated.find("# <Overlay.Enabled>") != std::string::npos &&
            migrated.find("# </Overlay.Enabled>") != std::string::npos &&
            migrated.find("# <Hotkeys.Enabled>") != std::string::npos &&
            migrated.find("# </Hotkeys.Enabled>") != std::string::npos;
        FileIdentityAndTime before{}, after{};
        const bool metadataBefore = ReadFileIdentityAndTime(path, before);
        const bool secondSyncNoOp = !Synchronize(path);
        const bool metadataAfter = ReadFileIdentityAndTime(path, after);
        const bool idempotent = secondSyncNoOp && metadataBefore && metadataAfter &&
            SameFileIdentityAndTime(before, after) && ReadText(path) == migrated;
        return valuesPreserved && docsReplaced && compactMarkers && idempotent;
    }

    bool TestNoOpStartupAndPersistenceDoNotTouchFile(
        const std::filesystem::path& path)
    {
        std::error_code error;
        std::filesystem::remove(path, error);
        if (error) return false;
        const auto synchronizer = [](const std::filesystem::path& target) {
            return config::SynchronizeManagedConfigTemplate(target,
                [](std::string) {});
        };
        config::FeatureConfig configuration{};
        if (!config::LoadFeatureConfig(path, configuration, synchronizer,
            [](std::string) {})) return false;

        const auto bytesBefore = ReadText(path);
        const auto handle = CreateFileW(path.c_str(), FILE_WRITE_ATTRIBUTES,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
            OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (handle == INVALID_HANDLE_VALUE) return false;
        FILETIME fixedTime{0x12345678, 0x01DABCDF};
        const bool timestampSet = SetFileTime(handle, nullptr, nullptr, &fixedTime) != FALSE;
        CloseHandle(handle);
        if (!timestampSet) return false;

        FileIdentityAndTime before{}, after{};
        if (!ReadFileIdentityAndTime(path, before)) return false;
        const bool syncNoOp = !config::SynchronizeManagedConfigTemplate(path,
            [](std::string) {});
        const bool syncMetadata = ReadFileIdentityAndTime(path, after);
        if (!syncNoOp || !syncMetadata || !SameFileIdentityAndTime(before, after) ||
            ReadText(path) != bytesBefore) {
            const auto bytesAfter = ReadText(path);
            std::size_t firstDifference = 0;
            while (firstDifference < bytesBefore.size() && firstDifference < bytesAfter.size() &&
                bytesBefore[firstDifference] == bytesAfter[firstDifference]) ++firstDifference;
            std::printf("no_op_sync no_op=%d metadata=%d same=%d bytes=%d\n",
                syncNoOp, syncMetadata,
                syncMetadata && SameFileIdentityAndTime(before, after),
                ReadText(path) == bytesBefore);
            std::printf("no_op_diff index=%zu before_size=%zu after_size=%zu before_byte=%u after_byte=%u\n",
                firstDifference, bytesBefore.size(), bytesAfter.size(),
                firstDifference < bytesBefore.size() ?
                    static_cast<unsigned char>(bytesBefore[firstDifference]) : 999u,
                firstDifference < bytesAfter.size() ?
                    static_cast<unsigned char>(bytesAfter[firstDifference]) : 999u);
            std::printf("no_op_before_excerpt=%s\nno_op_after_excerpt=%s\n",
                bytesBefore.substr(firstDifference > 80 ? firstDifference - 80 : 0, 260).c_str(),
                bytesAfter.substr(firstDifference > 80 ? firstDifference - 80 : 0, 260).c_str());
            return false;
        }

        const bool persistResult = config::PersistConfigValue(path, "Gameplay", "Mode",
            "RuntimeCombine", [](std::string) {});
        const bool persistMetadata = ReadFileIdentityAndTime(path, after);
        if (!persistResult || !persistMetadata || !SameFileIdentityAndTime(before, after) ||
            ReadText(path) != bytesBefore) {
            std::printf("no_op_persist result=%d metadata=%d same=%d bytes=%d\n",
                persistResult, persistMetadata,
                persistMetadata && SameFileIdentityAndTime(before, after),
                ReadText(path) == bytesBefore);
            return false;
        }
        return true;
    }

    bool TestPersistenceChangesOnlyRequestedAssignment(
        const std::filesystem::path& path)
    {
        const std::string original =
            "[Overlay]\r\n"
            "Language = Auto\r\n"
            "; custom note\r\n"
            "FontSize=017\r\n"
            "future_key = keep\r\n"
            "[Custom]\r\n"
            "; custom section comment\r\n"
            "X=Y\r\n";
        if (!WriteText(path, original) ||
            !config::PersistConfigValue(path, "Overlay", "Language", "uk",
                [](std::string) {})) return false;
        const std::string expected =
            "[Overlay]\r\n"
            "Language = uk\r\n"
            "; custom note\r\n"
            "FontSize=017\r\n"
            "future_key = keep\r\n"
            "[Custom]\r\n"
            "; custom section comment\r\n"
            "X=Y\r\n";
        const auto actual = ReadText(path);
        if (actual != expected) std::printf("isolated_persistence_actual=%s\n", actual.c_str());
        if (actual != expected) return false;

        const std::string lfWithoutFinalNewline =
            "[Overlay]\nLanguage=Auto\nfuture_key = keep";
        const std::string lfExpected =
            "[Overlay]\nLanguage=uk\nfuture_key = keep";
        return WriteText(path, lfWithoutFinalNewline) &&
            config::PersistConfigValue(path, "Overlay", "Language", "uk",
                [](std::string) {}) && ReadText(path) == lfExpected;
    }

    bool TestNormalTemplateSynchronization(const std::filesystem::path& path,
        const std::string& original)
    {
        if (!WriteText(path, original)) return false;
        if (!Synchronize(path)) return false;
        const auto synchronized = ReadText(path);
        return synchronized.find("[Hotkeys]") != std::string::npos &&
            synchronized.find("[Diagnostics]") != std::string::npos &&
            HasAssignment(synchronized, "Enabled", "false") &&
            synchronized.find(config::canonical_ini_documentation::GameplayEnabledComments[0]) !=
                std::string::npos;
    }

    bool TestTemplateStagingFailurePreservesOriginal(const std::filesystem::path& path,
        const std::string& original)
    {
        if (!WriteText(path, original)) return false;
        const auto staging = std::filesystem::path(path.wstring() + L".tmp");
        std::error_code error;
        std::filesystem::create_directory(staging, error);
        if (error) return false;
        const bool result = Synchronize(path);
        std::filesystem::remove(staging, error);
        return !result && ReadText(path) == original;
    }

    bool TestTemplateReplacementFailurePreservesOriginal(const std::filesystem::path& path,
        const std::string& original)
    {
        if (!WriteText(path, original)) return false;
        const auto handle = CreateFileW(path.c_str(), GENERIC_READ | GENERIC_WRITE, 0,
            nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (handle == INVALID_HANDLE_VALUE) return false;

        const bool result = Synchronize(path);
        CloseHandle(handle);
        const auto staging = std::filesystem::path(path.wstring() + L".tmp");
        std::error_code error;
        std::filesystem::remove(staging, error);
        return !result && ReadText(path) == original;
    }

    bool TestTemplateRepairPreservesValuesAndDropsNotes(const std::filesystem::path& path)
    {
        const std::string damaged =
            "; user header\n"
            "[Gameplay]\n"
            "Enabled=false\n"
            "; user gameplay note\n"
            "UnknownGameplay=keep\n"
            "\n"
            "[Dialogue]\n"
            "Zoom=Potato\n";
        if (!WriteText(path, damaged)) return false;

        if (!Synchronize(path)) return false;
        const auto repaired = ReadText(path);
        const bool preserved =
            repaired.find("user header") == std::string::npos &&
            repaired.find("user gameplay note") == std::string::npos &&
            HasAssignment(repaired, "Enabled", "false") &&
            HasAssignment(repaired, "UnknownGameplay", "keep") &&
            repaired.find("# <Gameplay.Enabled>") != std::string::npos &&
            repaired.find("STALKER2-MANAGED-DOC") == std::string::npos &&
            repaired.find("Zoom=Potato") == std::string::npos &&
            HasAssignment(repaired, "Enabled", "true") &&
            HasAssignment(repaired, "Mode", "Adaptive") &&
            repaired.find("[Cinematics]") != std::string::npos &&
            repaired.find("[Overlay]") != std::string::npos &&
            HasAssignment(repaired, "FontSize", "13") &&
            repaired.find("[Hotkeys]") != std::string::npos &&
            repaired.find("Mode=Native") == std::string::npos;
        if (!preserved) std::printf("template_repair_drops_notes=%d keeps_values=%d keeps_unknown=%d restores_docs=%d removes_invalid=%d\n",
            repaired.find("user header") == std::string::npos &&
                repaired.find("user gameplay note") == std::string::npos,
            HasAssignment(repaired, "Enabled", "false"),
            HasAssignment(repaired, "UnknownGameplay", "keep"),
            repaired.find("# <Gameplay.Enabled>") != std::string::npos,
            repaired.find("Zoom=Potato") == std::string::npos);
        if (!preserved) return false;

        const auto secondBefore = repaired;
        if (Synchronize(path)) return false;
        return ReadText(path) == secondBefore;
    }

    bool TestUnknownConfigRecoveryBacksUpAndPreservesReadableValues(
        const std::filesystem::path& path)
    {
        const std::string damaged =
            "unrecognized preamble\r\n"
            "[Gameplay]\r\nMode=HorPlus\r\nbroken assignment\r\n"
            "[Cinematics]\r\nAspectRatio=21:9\r\n"
            "[Dialogue]\r\nZoom=Reduced\r\n"
            "[Diagnostics]\r\nEnabled=true\r\n"
            "[Overlay]\r\nEnabled=false\r\n"
            "[Hotkeys]\r\nOverlayToggle=F8\r\n"
            "[Custom]\r\nKeep=readable\r\n";
        if (!WriteText(path, damaged)) return false;
        if (!Synchronize(path)) {
            std::printf("unknown_recovery_sync_failed source=%s\n", ReadText(path).c_str());
            return false;
        }

        std::filesystem::path backup;
        const auto backupPrefix = path.filename().wstring() + L".backup-";
        std::error_code error;
        for (std::filesystem::directory_iterator iterator(path.parent_path(), error), end;
            !error && iterator != end; iterator.increment(error)) {
            const auto name = iterator->path().filename().wstring();
            if (name.starts_with(backupPrefix)) {
                backup = iterator->path();
                break;
            }
        }
        if (error || backup.empty() || ReadText(backup) != damaged) {
            std::printf("unknown_recovery_backup_failed found=%d error=%d\n",
                backup.empty() ? 0 : 1, error.value());
            return false;
        }

        const auto recovered = ReadText(path);
        if (recovered.find("unrecognized preamble") != std::string::npos ||
            recovered.find("broken assignment") != std::string::npos ||
            !HasAssignment(recovered, "Mode", "HorPlus") ||
            !HasAssignment(recovered, "Mode", "21:9") ||
            !HasAssignment(recovered, "Mode", "Reduced") ||
            !HasAssignment(recovered, "Enabled", "false") ||
            !HasAssignment(recovered, "ToggleKey", "F8") ||
            !HasAssignment(recovered, "Keep", "readable")) {
            std::printf("unknown_recovery_content_failed text=%s\n", recovered.c_str());
            return false;
        }

        config::FeatureConfig configuration{};
        if (!config::LoadFeatureConfig(path, configuration,
            [](const std::filesystem::path&) { return false; }, [](std::string) {})) {
            std::printf("unknown_recovery_config_load_failed\n");
            return false;
        }
        const bool valid = configuration.gameplayMode == config::GameplayMode::HorPlus &&
            configuration.cinematicAspectPolicy == config::CinematicAspectPolicy::Forced21x9 &&
            configuration.dialogueZoomPolicy == config::DialogueZoomPolicy::Reduced &&
            configuration.diagnosticsEnabled && !configuration.overlayEnabled &&
            configuration.overlayToggleKey == VK_F8 && !Synchronize(path);
        if (!valid) std::printf("unknown_recovery_values_failed gameplay=%d cinematic=%d dialogue=%d diagnostics=%d overlay=%d toggle=%d\n",
            static_cast<int>(configuration.gameplayMode),
            static_cast<int>(configuration.cinematicAspectPolicy),
            static_cast<int>(configuration.dialogueZoomPolicy),
            configuration.diagnosticsEnabled, configuration.overlayEnabled,
            configuration.overlayToggleKey);
        if (!valid) return false;

        const auto foreignPath = path.parent_path() / "foreign.ini";
        const std::string foreign = "[ForeignProfile]\nPreset=custom\n";
        if (!WriteText(foreignPath, foreign) || !Synchronize(foreignPath)) return false;
        backup.clear();
        const auto foreignPrefix = foreignPath.filename().wstring() + L".backup-";
        error.clear();
        for (std::filesystem::directory_iterator iterator(path.parent_path(), error), end;
            !error && iterator != end; iterator.increment(error)) {
            if (iterator->path().filename().wstring().starts_with(foreignPrefix)) {
                backup = iterator->path();
                break;
            }
        }
        const auto foreignRecovered = ReadText(foreignPath);
        return !error && !backup.empty() && ReadText(backup) == foreign &&
            foreignRecovered.find("[ForeignProfile]") != std::string::npos &&
            HasAssignment(foreignRecovered, "Preset", "custom") &&
            HasAssignment(foreignRecovered, "Mode", "RuntimeCombine");
    }

    bool TestUtf8BomLegacyConfigMigration(const std::filesystem::path& path)
    {
        const std::string legacy = "\xEF\xBB\xBF[Gameplay]\r\nMode=HorPlus\r\n"
            "[Overlay]\r\nEnabled=false\r\n[Hotkeys]\r\nOverlayToggle=F8\r\n";
        if (!WriteText(path, legacy) || !Synchronize(path)) return false;
        const auto migrated = ReadText(path);
        if (!HasAssignment(migrated, "Mode", "HorPlus") ||
            !HasAssignment(migrated, "Enabled", "false") ||
            !HasAssignment(migrated, "ToggleKey", "F8") ||
            migrated.find("OverlayToggle=") != std::string::npos ||
            migrated.find("\xEF\xBB\xBF") != std::string::npos) return false;

        std::error_code error;
        const auto backupPrefix = path.filename().wstring() + L".backup-";
        for (std::filesystem::directory_iterator iterator(path.parent_path(), error), end;
            !error && iterator != end; iterator.increment(error)) {
            if (iterator->path().filename().wstring().starts_with(backupPrefix)) return false;
        }
        config::FeatureConfig configuration{};
        return !error && config::LoadFeatureConfig(path, configuration,
            [](const std::filesystem::path&) { return false; }, [](std::string) {}) &&
            configuration.gameplayMode == config::GameplayMode::HorPlus &&
            !configuration.overlayEnabled && configuration.overlayToggleKey == VK_F8;
    }

    bool TestNormalPersistence(const std::filesystem::path& path, const std::string& original)
    {
        if (!WriteText(path, original)) return false;
        const bool wrote = Persist(path, "false");
        const auto after = ReadText(path);
        return wrote && HasAssignment(after, "Enabled", "false");
    }

    bool TestDuplicateManagedOccurrencesConverge(const std::filesystem::path& path)
    {
        const std::string duplicate =
            "[Gameplay]\nEnabled=true\n\n"
            "[Gameplay]\nEnabled=true\n";
        if (!WriteText(path, duplicate) || !Persist(path, "false")) return false;
        config::FeatureConfig configuration{};
        if (!config::LoadFeatureConfig(path, configuration,
            [](const std::filesystem::path&) { return false; },
            [](std::string) {}) || configuration.gameplayEnabled)
            return false;
        const auto text = ReadText(path);
        return CountAssignments(text, "Enabled", "false") >= 2;
    }

    bool TestDuplicateOverlayPlacementRoundTrips(const std::filesystem::path& path)
    {
        const std::string original =
            "[Overlay]\nPositionX=30\nPositionY=31\nfirst-note=keep\n"
            "[Gameplay]\nEnabled=true\n"
            "[Overlay]\nPositionX=60\nPositionY=61\nsecond-note=keep\n";
        if (!WriteText(path, original) ||
            !config::PersistOverlayPlacement(path, 100, 200)) return false;
        const auto saved = ReadText(path);
        int x = -1, y = -1;
        bool migrated = false;
        const bool loaded = config::LoadOverlayPlacement(path, {}, x, y, migrated);
        const bool contentOkay = saved.find("first-note=keep") != std::string::npos &&
            saved.find("second-note=keep") != std::string::npos &&
            CountAssignments(saved, "PositionX", "100") == 2 &&
            CountAssignments(saved, "PositionY", "200") == 2;
        const auto afterLoad = ReadText(path);
        FileIdentityAndTime beforeSave{}, afterSave{};
        const bool metadataBefore = ReadFileIdentityAndTime(path, beforeSave);
        const bool stableSave = config::PersistOverlayPlacement(path, 100, 200) &&
            ReadText(path) == afterLoad && metadataBefore &&
            ReadFileIdentityAndTime(path, afterSave) &&
            SameFileIdentityAndTime(beforeSave, afterSave);
        if (!loaded || x != 100 || y != 200 || migrated || !contentOkay || !stableSave)
            std::printf("duplicate_overlay_details loaded=%d x=%d y=%d migrated=%d content=%d stable=%d\n",
                loaded, x, y, migrated, contentOkay, stableSave);
        return loaded && x == 100 && y == 200 && !migrated && contentOkay && stableSave;
    }

    bool TestCanonicalRebuildPreservesLogicalSettings(const std::filesystem::path& path)
    {
        const char* inputs[]{
            "[Overlay]\nEnabled=false\nLanguage=uk\nToggleKey=F8\n"
            "[Gameplay]\nEnabled=false\nMode=HorPlus\n"
            "[Overlay]\nPositionX=100\nPositionY=200\n"
            "[Gameplay]\ncustom=retained\n",
            "[Features]\nGameplayAspectFix=false\n",
            "[Features]\nGameplayAspectFix=true\n[Gameplay]\nEnabled=false\n",
            "[Gameplay]\nEnabled=false\n[Features]\nGameplayAspectFix=true\n",
            "[Gameplay]\nEnabled=true\n[Gameplay]\nEnabled=false\nMode=HorPlus\n"
        };
        for (const auto* input : inputs) {
            if (!WriteText(path, input) || !Synchronize(path)) return false;
            config::FeatureConfig loaded{};
            if (!config::LoadFeatureConfig(path, loaded,
                    [](const std::filesystem::path&) { return false; },
                    [](std::string) {}) || loaded.gameplayEnabled) return false;
            if (std::string(input).find("PositionX") != std::string::npos &&
                (loaded.overlayEnabled || loaded.overlayLocaleCode != "uk" ||
                    loaded.overlayToggleKey != VK_F8 ||
                    loaded.gameplayMode != config::GameplayMode::HorPlus ||
                    ReadText(path).find("custom = retained") == std::string::npos))
                return false;
            const auto bytes = ReadText(path);
            FileIdentityAndTime before{}, after{};
            if (!ReadFileIdentityAndTime(path, before) || Synchronize(path) ||
                ReadText(path) != bytes || !ReadFileIdentityAndTime(path, after) ||
                !SameFileIdentityAndTime(before, after)) return false;
        }
        return true;
    }

    bool TestMultiValuePersistenceIsAtomic(const std::filesystem::path& path)
    {
        const std::string original =
            "[Overlay]\nLanguage=Auto\nFontSize=13\nuser-note=keep\n";
        if (!WriteText(path, original) || !config::PersistConfigValues(path,
            {{"Overlay", "Language", "uk"}, {"Overlay", "FontSize", "17"}},
            [](std::string) {})) return false;
        const auto lastKnownGood = ReadText(path);
        if (!HasAssignment(lastKnownGood, "Language", "uk") ||
            !HasAssignment(lastKnownGood, "FontSize", "17") ||
            lastKnownGood.find("user-note=keep") == std::string::npos) return false;

        const auto handle = CreateFileW(path.c_str(), GENERIC_READ | GENERIC_WRITE, 0,
            nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (handle == INVALID_HANDLE_VALUE) return false;
        const bool failed = !config::PersistConfigValues(path,
            {{"Overlay", "Language", "en"}, {"Overlay", "FontSize", "19"}},
            [](std::string) {});
        CloseHandle(handle);
        return failed && ReadText(path) == lastKnownGood;
    }

    bool TestStagingFailurePreservesOriginal(const std::filesystem::path& path,
        const std::string& original)
    {
        if (!WriteText(path, original)) return false;
        const auto staging = std::filesystem::path(path.wstring() + L".tmp");
        std::error_code error;
        std::filesystem::create_directory(staging, error);
        if (error) return false;
        const bool result = Persist(path, "false");
        std::filesystem::remove(staging, error);
        return !result && ReadText(path) == original;
    }

    bool TestReplacementFailurePreservesOriginal(const std::filesystem::path& path,
        const std::string& original)
    {
        if (!WriteText(path, original)) return false;
        const auto handle = CreateFileW(path.c_str(), GENERIC_READ | GENERIC_WRITE, 0,
            nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (handle == INVALID_HANDLE_VALUE) return false;

        const bool result = Persist(path, "false");
        CloseHandle(handle);
        const auto staging = std::filesystem::path(path.wstring() + L".tmp");
        std::error_code error;
        std::filesystem::remove(staging, error);
        return !result && ReadText(path) == original;
    }

    bool TestReadOnlyFilePreservesOriginal(const std::filesystem::path& path,
        const std::string& original)
    {
        if (!WriteText(path, original) ||
            !SetFileAttributesW(path.c_str(), FILE_ATTRIBUTE_READONLY)) return false;
        const bool persisted = config::PersistConfigValue(path, "Gameplay", "Enabled",
            "false", [](std::string) {});
        const bool restoredAttributes = SetFileAttributesW(path.c_str(), FILE_ATTRIBUTE_NORMAL) != FALSE;
        const auto staging = std::filesystem::path(path.wstring() + L".tmp");
        std::error_code error;
        std::filesystem::remove(staging, error);
        return !persisted && restoredAttributes && ReadText(path) == original;
    }

    bool TestDiagnosticsConfig(const std::filesystem::path& path)
    {
        if (!WriteText(path, "[Diagnostics]\nEnabled=true\n")) return false;
        config::FeatureConfig configuration{};
        return config::LoadFeatureConfig(path, configuration,
            [](const std::filesystem::path&) { return false; },
            [](std::string) {}) && configuration.diagnosticsEnabled;
    }

    bool TestOverlayLocaleCodeConfig(const std::filesystem::path& path)
    {
        const auto synchronizer = [](const std::filesystem::path&) { return false; };
        const auto log = [](std::string) {};
        config::FeatureConfig configuration{};
        if (!WriteText(path, "[Overlay]\nLanguage=Auto\n") ||
            !config::LoadFeatureConfig(path, configuration, synchronizer, log) ||
            !configuration.overlayLocaleAuto || configuration.overlayLocaleCode != "en")
            return false;
        if (!config::PersistConfigValue(path, "Overlay", "Language", "Auto", log))
            return false;
        configuration = {};
        if (!config::LoadFeatureConfig(path, configuration, synchronizer, log) ||
            !configuration.overlayLocaleAuto || configuration.overlayLocaleCode != "en")
            return false;
        if (!WriteText(path, "[Overlay]\nLanguage=uk\n") ||
            !config::LoadFeatureConfig(path, configuration, synchronizer, log) ||
            configuration.overlayLocaleCode != "uk" || configuration.overlayLocaleAuto)
            return false;
        if (!WriteText(path, "[Gameplay]\nEnabled=true\n") ||
            !config::LoadFeatureConfig(path, configuration, synchronizer, log) ||
            configuration.overlayLocaleCode != "en")
            return false;
        if (!WriteText(path, "[Overlay]\nLanguage=unsupported\n") ||
            !config::LoadFeatureConfig(path, configuration, synchronizer, log) ||
            configuration.overlayLocaleCode != "en")
            return false;
        if (!WriteText(path, "[Overlay]\nLanguage=UK\n") ||
            !config::LoadFeatureConfig(path, configuration, synchronizer, log) ||
            configuration.overlayLocaleCode != "uk" ||
            configuration.overlayLocaleAuto ||
            configuration.overlayFontSize != 15)
            return false;
        if (!config::PersistConfigValue(path, "Overlay", "Language", "uk", log))
            return false;
        configuration = {};
        if (!config::LoadFeatureConfig(path, configuration, synchronizer, log) ||
            configuration.overlayLocaleCode != "uk")
            return false;

        for (const auto& locale : localization::LocaleRegistry) {
            std::string inputCode(locale.code);
            std::transform(inputCode.begin(), inputCode.end(), inputCode.begin(),
                [](unsigned char value) { return static_cast<char>(std::tolower(value)); });
            if (!WriteText(path, "[Overlay]\nLanguage=" + inputCode + "\n") ||
                !config::LoadFeatureConfig(path, configuration, synchronizer, log) ||
                configuration.overlayLocaleCode != locale.code ||
                configuration.overlayFontSize != locale.initialFontSize ||
                !config::PersistConfigValue(path, "Overlay", "Language",
                    std::string(locale.code), log))
                return false;
            configuration = {};
            if (!config::LoadFeatureConfig(path, configuration, synchronizer, log) ||
                configuration.overlayLocaleCode != locale.code ||
                configuration.overlayFontSize != locale.initialFontSize)
                return false;
        }
        return true;
    }

    bool TestOverlayEnabledConfig(const std::filesystem::path& path)
    {
        const auto synchronizer = [](const std::filesystem::path&) { return false; };
        const auto log = [](std::string) {};
        config::FeatureConfig configuration{};
        if (!WriteText(path, "[Gameplay]\nEnabled=true\n") ||
            !config::LoadFeatureConfig(path, configuration, synchronizer, log) ||
            !configuration.overlayEnabled)
            return false;
        if (!WriteText(path, "[Overlay]\nEnabled=false\n") ||
            !config::LoadFeatureConfig(path, configuration, synchronizer, log) ||
            configuration.overlayEnabled)
            return false;
        if (!config::PersistConfigValue(path, "Overlay", "Enabled", "true", log))
            return false;
        configuration = {};
        if (!config::LoadFeatureConfig(path, configuration, synchronizer, log) ||
            !configuration.overlayEnabled) {
            return false;
        }
        if (!WriteText(path, "[Overlay]\nEnabled=malformed\n") ||
            !config::LoadFeatureConfig(path, configuration, synchronizer, log) ||
            !configuration.overlayEnabled)
            return false;
        return true;
    }

    bool TestOverlayFontSizeConfig(const std::filesystem::path& path)
    {
        const auto synchronizer = [](const std::filesystem::path&) { return false; };
        const auto log = [](std::string) {};
        config::FeatureConfig configuration{};
        if (!WriteText(path, "[Overlay]\nFontSize=20\n") ||
            !config::LoadFeatureConfig(path, configuration, synchronizer, log) ||
            configuration.overlayFontSize != 20)
            return false;
        if (!WriteText(path, "[Gameplay]\nEnabled=true\n") ||
            !config::LoadFeatureConfig(path, configuration, synchronizer, log) ||
            configuration.overlayFontSize != config::OverlayFontSizeDefault)
            return false;
        if (!WriteText(path, "[Overlay]\nFontSize=25\n") ||
            !config::LoadFeatureConfig(path, configuration, synchronizer, log) ||
            configuration.overlayFontSize != config::OverlayFontSizeDefault)
            return false;
        if (!WriteText(path, "[Overlay]\nLanguage=ja\nFontSize=25\n") ||
            !config::LoadFeatureConfig(path, configuration, synchronizer, log) ||
            configuration.overlayFontSize !=
                localization::FindLocaleDescriptor("ja")->initialFontSize)
            return false;
        if (!config::PersistConfigValue(path, "Overlay", "FontSize", "12", log))
            return false;
        configuration = {};
        return config::LoadFeatureConfig(path, configuration, synchronizer, log) &&
            configuration.overlayFontSize == config::OverlayFontSizeMin &&
            config::ParseOverlayFontSize(" 24 ", configuration.overlayFontSize) &&
            configuration.overlayFontSize == config::OverlayFontSizeMax &&
            !config::ParseOverlayFontSize("11", configuration.overlayFontSize) &&
            !config::ParseOverlayFontSize("16px", configuration.overlayFontSize);
    }

    bool TestCinematicFovModeConfig(const std::filesystem::path& path)
    {
        if (!WriteText(path, "[Cinematics]\nFovMode=GameplayHorPlus\n")) return false;
        config::FeatureConfig configuration{};
        if (!config::LoadFeatureConfig(path, configuration,
            [](const std::filesystem::path&) { return false; },
            [](std::string) {})) return false;
        if (configuration.cinematicFovMode != config::CinematicFovMode::GameplayHorPlus)
            return false;
        if (!WriteText(path, "[Cinematics]\nFovMode=invalid\n")) return false;
        configuration = {};
        if (!config::LoadFeatureConfig(path, configuration,
            [](const std::filesystem::path&) { return false; },
            [](std::string) {})) return false;
        if (configuration.cinematicFovMode != config::CinematicFovMode::GameplayHorPlus)
            return false;
        if (config::NextCinematicFovMode(config::CinematicFovMode::NativeHorPlus) !=
                config::CinematicFovMode::GameplayHorPlus ||
            config::NextCinematicFovMode(config::CinematicFovMode::GameplayHorPlus) !=
                config::CinematicFovMode::NativeHorPlus)
            return false;
        const std::array<config::CinematicAspectPolicy, 4> aspectPolicies{
            config::CinematicAspectPolicy::Auto,
            config::CinematicAspectPolicy::Forced16x9,
            config::CinematicAspectPolicy::Forced21x9,
            config::CinematicAspectPolicy::Forced32x9};
        auto policy = aspectPolicies.front();
        for (std::size_t index = 1; index <= aspectPolicies.size(); ++index) {
            policy = config::NextCinematicAspectPolicy(policy);
            if (policy != aspectPolicies[index % aspectPolicies.size()]) return false;
        }
        const auto load = [&](const std::string& text,
            config::FeatureConfig& result) {
            result = {};
            return WriteText(path, text) && config::LoadFeatureConfig(path, result,
                [](const std::filesystem::path&) { return false; },
                [](std::string) {});
        };
        if (!load("[Cinematics]\nEnabled=false\nMode=21:9\nFovMode=NativeHorPlus\n",
            configuration) || configuration.cinematicsEnabled ||
            configuration.cinematicAspectPolicy != config::CinematicAspectPolicy::Forced21x9 ||
            configuration.cinematicFovMode != config::CinematicFovMode::NativeHorPlus)
            return false;
        if (!load("[Cinematics]\nMode=Native\n", configuration) ||
            configuration.cinematicsEnabled ||
            configuration.cinematicAspectPolicy != config::CinematicAspectPolicy::Auto)
            return false;
        if (!load("[Cinematics]\nMode=32:9\n", configuration) ||
            !configuration.cinematicsEnabled ||
            configuration.cinematicAspectPolicy != config::CinematicAspectPolicy::Forced32x9)
            return false;
        if (!load("[Cinematics]\nAspectRatio=Native\n", configuration) ||
            configuration.cinematicsEnabled ||
            configuration.cinematicAspectPolicy != config::CinematicAspectPolicy::Auto)
            return false;
        if (!load("[Cinematics]\nAspectFix=false\n", configuration) ||
            configuration.cinematicsEnabled)
            return false;
        if (!WriteText(path, "[Hotkeys]\nCinematicFovCycle=F12\n")) return false;
        configuration = {};
        if (!config::LoadFeatureConfig(path, configuration,
            [](const std::filesystem::path&) { return false; },
            [](std::string) {})) return false;
        return configuration.cinematicFovCycleKey == VK_F12;
    }

    bool TestCinematicMigrationRoundTrips(const std::filesystem::path& path)
    {
        struct MigrationCase
        {
            const char* name;
            const char* source;
            bool expectedEnabled;
            config::CinematicAspectPolicy expectedPolicy;
        };
        constexpr MigrationCase cases[] = {
            {"legacy_valid_native_aspect", "[Cinematics]\nAspectRatio=Native\n",
                false, config::CinematicAspectPolicy::Auto},
            {"legacy_valid_non_native_aspect", "[Cinematics]\nAspectRatio=21:9\n",
                true, config::CinematicAspectPolicy::Forced21x9},
            {"new_valid_native_mode", "[Cinematics]\nMode=Native\n",
                false, config::CinematicAspectPolicy::Auto},
            {"new_valid_non_native_mode", "[Cinematics]\nMode=32:9\n",
                true, config::CinematicAspectPolicy::Forced32x9},
            {"malformed_aspect_preserves_legacy_disabled",
                "[Cinematics]\nAspectFix=false\nAspectRatio=garbage\n",
                false, config::CinematicAspectPolicy::Auto},
            {"malformed_mode_preserves_legacy_disabled",
                "[Cinematics]\nMode=garbage\nAspectFix=false\n",
                false, config::CinematicAspectPolicy::Auto},
            {"explicit_legacy_disabled", "[Cinematics]\nAspectFix=false\n",
                false, config::CinematicAspectPolicy::Auto},
            {"explicit_new_disabled_wins",
                "[Cinematics]\nEnabled=false\nMode=21:9\n",
                false, config::CinematicAspectPolicy::Forced21x9},
            {"explicit_new_enabled_wins",
                "[Cinematics]\nEnabled=true\nMode=Native\n",
                true, config::CinematicAspectPolicy::Auto},
        };
        const auto noSynchronization = [](const std::filesystem::path&) { return false; };
        const auto noLog = [](std::string) {};
        for (const auto& test : cases) {
            config::FeatureConfig before{};
            if (!WriteText(path, test.source) ||
                !config::LoadFeatureConfig(path, before, noSynchronization, noLog)) {
                std::printf("cinematic_migration_%s=FAIL\n", test.name);
                return false;
            }

            if (!Synchronize(path)) {
                std::printf("cinematic_migration_%s=FAIL\n", test.name);
                return false;
            }

            config::FeatureConfig after{};
            const bool reloaded = config::LoadFeatureConfig(path, after,
                noSynchronization, noLog);
            const bool matchesExpected = before.cinematicsEnabled == test.expectedEnabled &&
                after.cinematicsEnabled == test.expectedEnabled &&
                before.cinematicAspectPolicy == test.expectedPolicy &&
                after.cinematicAspectPolicy == test.expectedPolicy &&
                before.cinematicFovMode == after.cinematicFovMode;
            const bool idempotent = !Synchronize(path);
            const bool passed = reloaded && matchesExpected && idempotent;
            if (!passed) std::printf("cinematic_migration_details before_enabled=%d after_enabled=%d before_policy=%d after_policy=%d reload=%d expected_enabled=%d expected_policy=%d idempotent=%d\n",
                before.cinematicsEnabled, after.cinematicsEnabled,
                static_cast<int>(before.cinematicAspectPolicy),
                static_cast<int>(after.cinematicAspectPolicy), reloaded,
                test.expectedEnabled, static_cast<int>(test.expectedPolicy), idempotent);
            std::printf("cinematic_migration_%s=%s\n", test.name,
                passed ? "PASS" : "FAIL");
            if (!passed) return false;
        }
        return true;
    }

    bool TestDialogueMigrationRoundTrips(const std::filesystem::path& path)
    {
        struct MigrationCase
        {
            const char* name;
            const char* source;
            bool expectedEnabled;
            config::DialogueZoomPolicy expectedMode;
        };
        constexpr MigrationCase cases[] = {
            {"legacy_native", "[Dialogue]\nZoom=Native\n", false,
                config::DialogueZoomPolicy::Adaptive},
            {"legacy_adaptive", "[Dialogue]\nZoom=Adaptive\n", true,
                config::DialogueZoomPolicy::Adaptive},
            {"legacy_reduced", "[Dialogue]\nZoom=Reduced\n", true,
                config::DialogueZoomPolicy::Reduced},
            {"legacy_disabled", "[Dialogue]\nZoom=Disabled\n", true,
                config::DialogueZoomPolicy::Disabled},
            {"malformed_legacy", "[Dialogue]\nZoom=bad\n", true,
                config::DialogueZoomPolicy::Adaptive},
            {"malformed_mode_native_legacy", "[Dialogue]\nMode=bad\nZoom=Native\n", false,
                config::DialogueZoomPolicy::Adaptive},
            {"explicit_enabled_true", "[Dialogue]\nEnabled=true\nZoom=Native\n", true,
                config::DialogueZoomPolicy::Adaptive},
            {"explicit_enabled_false", "[Dialogue]\nEnabled=false\nZoom=Reduced\n", false,
                config::DialogueZoomPolicy::Reduced},
            {"canonical_mode_precedence", "[Dialogue]\nZoom=Reduced\nMode=Adaptive\n", true,
                config::DialogueZoomPolicy::Adaptive},
            {"canonical_all_precedence", "[Dialogue]\nEnabled=false\nZoom=Native\nMode=Disabled\n", false,
                config::DialogueZoomPolicy::Disabled},
            {"canonical_only", "[Dialogue]\nEnabled=false\nMode=Reduced\n", false,
                config::DialogueZoomPolicy::Reduced},
        };
        const auto noSynchronization = [](const std::filesystem::path&) { return false; };
        const auto noLog = [](std::string) {};
        for (const auto& test : cases) {
            // Every case starts with an independent configuration object.
            config::FeatureConfig before{};
            if (!WriteText(path, test.source) ||
                !config::LoadFeatureConfig(path, before, noSynchronization, noLog))
                return false;
            if (!Synchronize(path)) return false;
            const auto canonical = ReadText(path);
            config::FeatureConfig after{};
            const bool reloaded = config::LoadFeatureConfig(path, after,
                noSynchronization, noLog);
            const bool canonicalized = canonical.find("Zoom=") == std::string::npos &&
            HasAssignment(canonical, "Enabled", test.expectedEnabled ? "true" : "false") &&
                HasAssignment(canonical, "Mode",
                    config::DialogueZoomPolicyName(test.expectedMode));
            const bool equivalent = before.dialogueEnabled == test.expectedEnabled &&
                before.dialogueZoomPolicy == test.expectedMode && reloaded &&
                after.dialogueEnabled == before.dialogueEnabled &&
                after.dialogueZoomPolicy == before.dialogueZoomPolicy;
            const bool idempotent = !Synchronize(path);
            const bool passed = canonicalized && equivalent && idempotent;
            if (!passed) std::printf("dialogue_migration_details before_enabled=%d after_enabled=%d before_mode=%d after_mode=%d reload=%d canonicalized=%d equivalent=%d idempotent=%d\n",
                before.dialogueEnabled, after.dialogueEnabled,
                static_cast<int>(before.dialogueZoomPolicy),
                static_cast<int>(after.dialogueZoomPolicy), reloaded,
                canonicalized, equivalent, idempotent);
            std::printf("dialogue_migration_%s=%s\n", test.name,
                passed ? "PASS" : "FAIL");
            if (!passed) return false;
        }

        const std::string legacyNative = "[Dialogue]\nZoom=Native\n";
        if (!WriteText(path, legacyNative)) return false;
        const auto staging = std::filesystem::path(path.wstring() + L".tmp");
        std::error_code error;
        std::filesystem::create_directory(staging, error);
        if (error) return false;
        config::FeatureConfig fallback{};
        const bool loaded = config::LoadFeatureConfig(path, fallback,
            [](const std::filesystem::path& target) {
                return config::SynchronizeManagedConfigTemplate(target,
                    [](std::string) {});
            }, noLog);
        std::filesystem::remove(staging, error);
        return loaded && !fallback.dialogueEnabled &&
            fallback.dialogueZoomPolicy == config::DialogueZoomPolicy::Adaptive &&
            ReadText(path) == legacyNative;
    }

    bool TestHotkeyValidationAndPersistence(const std::filesystem::path& path)
    {
        if (!WriteText(path,
            "[Hotkeys]\nEnabled=false\nGameplayCycle=F9\nCinematicCycle=F10\n"
            "CinematicFovCycle=F11\nDialogueCycle=F12\n")) return false;
        config::FeatureConfig configuration{};
        const auto log = [](std::string) {};
        if (!config::LoadFeatureConfig(path, configuration,
            [](const std::filesystem::path&) { return false; }, log)) return false;
        const bool validated = !configuration.hotkeysEnabled &&
            configuration.overlayToggleKey == VK_DELETE &&
            config::IsSupportedHotkey(VK_F24) && config::IsSupportedHotkey('A') &&
            config::IsSupportedHotkey('7') && config::IsSupportedHotkey(VK_INSERT) &&
            config::IsSupportedHotkey(VK_HOME) && config::IsSupportedHotkey(VK_NUMPAD7) &&
            config::IsSupportedHotkey(VK_OEM_1) &&
            std::strcmp(config::HotkeyName('A'), "A") == 0 &&
            std::strcmp(config::HotkeyName('7'), "7") == 0 &&
            std::strcmp(config::HotkeyName(VK_F24), "F24") == 0 &&
            std::strcmp(config::HotkeyName(VK_NUMPAD7), "Num 7") == 0 &&
            std::strcmp(config::HotkeyName(VK_INSERT), "Insert") == 0 &&
            std::strcmp(config::HotkeyName(VK_DELETE), "Delete") == 0 &&
            std::strcmp(config::HotkeyName(VK_HOME), "Home") == 0 &&
            !config::IsSupportedHotkey(VK_ESCAPE) &&
            !config::IsSupportedHotkey(VK_CONTROL) &&
            !config::IsSupportedHotkey(VK_LBUTTON) &&
            !config::IsSupportedHotkey(VK_PACKET) &&
            config::HasHotkeyConflict(configuration, VK_F10,
                config::HotkeyBindingId::GameplayMode) &&
            config::HasHotkeyConflict(configuration, VK_DELETE,
                config::HotkeyBindingId::GameplayMode) &&
            !config::HasHotkeyConflict(configuration, VK_F6,
                config::HotkeyBindingId::GameplayMode);
        const int previousGameplayKey = configuration.gameplayCycleKey;
        // A conflicting candidate must be rejected without changing the current binding.
        const bool conflictPreserved =
            configuration.gameplayCycleKey == previousGameplayKey;
        if (!validated || !conflictPreserved) {
            std::printf("hotkey_validation_details enabled=%d supported=%d conflict=%d free=%d preserved=%d keys=%d,%d,%d,%d\n",
                configuration.hotkeysEnabled, config::IsSupportedHotkey(VK_F6),
                config::HasHotkeyConflict(configuration, VK_F10,
                    config::HotkeyBindingId::OverlayToggle),
                !config::HasHotkeyConflict(configuration, VK_F6,
                    config::HotkeyBindingId::OverlayToggle), conflictPreserved,
                configuration.gameplayCycleKey, configuration.cinematicCycleKey,
                configuration.cinematicFovCycleKey, configuration.dialogueCycleKey);
            return false;
        }
        if (!config::PersistConfigValue(path, "Hotkeys", "GameplayCycle",
            config::HotkeyConfigValue(VK_OEM_1), log)) {
            std::printf("hotkey_persist=FAIL\n");
            return false;
        }
        configuration = {};
        const bool loaded = config::LoadFeatureConfig(path, configuration,
            [](const std::filesystem::path&) { return false; }, log) &&
            configuration.gameplayCycleKey == VK_OEM_1 &&
            configuration.cinematicCycleKey == VK_F10 &&
            configuration.overlayToggleKey == VK_DELETE;
        if (!loaded)
            std::printf("hotkey_reload_details keys=%d,%d,%d,%d\n",
                configuration.gameplayCycleKey, configuration.cinematicCycleKey,
                configuration.cinematicFovCycleKey, configuration.dialogueCycleKey);
        if (!loaded) return false;

        struct OemKeyLabel { int key; const char* label; };
        constexpr OemKeyLabel oemKeys[] = {
            {VK_OEM_1, ";"}, {VK_OEM_PLUS, "="}, {VK_OEM_COMMA, ","},
            {VK_OEM_MINUS, "-"}, {VK_OEM_PERIOD, "."}, {VK_OEM_2, "/"},
            {VK_OEM_3, "`"}, {VK_OEM_4, "["}, {VK_OEM_5, "\\"},
            {VK_OEM_6, "]"}, {VK_OEM_7, "'"}, {VK_OEM_8, "OEM 8"},
            {VK_OEM_102, "\\"}, {VK_OEM_AX, "OEM AX"}};
        for (const auto& [key, expectedLabel] : oemKeys) {
            int parsedKey = 0;
            const char* displayName = config::HotkeyName(key);
            if (!config::ParseHotkey(config::HotkeyConfigValue(key), parsedKey) ||
                parsedKey != key || !displayName ||
                std::strcmp(displayName, expectedLabel) != 0 ||
                !config::PersistConfigValue(path, "Hotkeys", "GameplayCycle",
                    config::HotkeyConfigValue(key), log))
                return false;
            configuration = {};
            if (!config::LoadFeatureConfig(path, configuration,
                [](const std::filesystem::path&) { return false; }, log) ||
                configuration.gameplayCycleKey != key ||
                std::strcmp(config::HotkeyName(configuration.gameplayCycleKey),
                    expectedLabel) != 0)
                return false;
        }
        if (!WriteText(path, "[Hotkeys]\nOverlayToggle=VK_46\nGameplayCycle=VK_46\n"))
            return false;
        configuration = {};
        return config::LoadFeatureConfig(path, configuration,
            [](const std::filesystem::path&) { return false; }, log) &&
            configuration.overlayToggleKey == 'F' &&
            configuration.gameplayCycleKey == VK_F9 &&
            !config::HasHotkeyConflict(configuration,
                configuration.overlayToggleKey,
                config::HotkeyBindingId::OverlayToggle);
    }

    bool TestHotkeyBindingRegistryRoundTrip(const std::filesystem::path& path)
    {
        constexpr config::HotkeyBindingId expectedIds[] = {
            config::HotkeyBindingId::OverlayToggle,
            config::HotkeyBindingId::GameplayMode,
            config::HotkeyBindingId::CinematicAspect,
            config::HotkeyBindingId::CinematicFov,
            config::HotkeyBindingId::DialogueZoom};
        if (config::HotkeyBindingRegistry.size() != std::size(expectedIds) ||
            !WriteText(path, "[Overlay]\nToggleKey=VK_2E\n[Hotkeys]\n"))
            return false;

        config::FeatureConfig defaults{};
        for (std::size_t index = 0; index < config::HotkeyBindingRegistry.size(); ++index) {
            const auto& descriptor = config::HotkeyBindingRegistry[index];
            if (descriptor.id != expectedIds[index] ||
                config::FindHotkeyBinding(descriptor.id) != &descriptor ||
                config::FindHotkeyBinding(descriptor.section, descriptor.configKey) != &descriptor ||
                config::HotkeyBindingValue(defaults, descriptor.id) !=
                    descriptor.defaultVirtualKey)
                return false;
            for (std::size_t other = index + 1;
                other < config::HotkeyBindingRegistry.size(); ++other) {
                const auto& candidate = config::HotkeyBindingRegistry[other];
                if (descriptor.id == candidate.id ||
                    (std::strcmp(descriptor.section, candidate.section) == 0 &&
                        std::strcmp(descriptor.configKey, candidate.configKey) == 0))
                    return false;
            }
            const int roundTripKey = 'A' + static_cast<int>(index);
            config::SetHotkeyBindingValue(defaults, descriptor.id, roundTripKey);
            if (config::HotkeyBindingValue(defaults, descriptor.id) != roundTripKey ||
                !config::PersistConfigValue(path, descriptor.section,
                    descriptor.configKey, config::HotkeyConfigValue(roundTripKey),
                    [](std::string) {}))
                return false;
        }

        config::FeatureConfig loaded{};
        if (!config::LoadFeatureConfig(path, loaded,
                [](const std::filesystem::path&) { return false; },
                [](std::string) {})) return false;
        for (std::size_t index = 0; index < config::HotkeyBindingRegistry.size(); ++index) {
            if (config::HotkeyBindingValue(loaded,
                    config::HotkeyBindingRegistry[index].id) !=
                'A' + static_cast<int>(index)) return false;
            for (std::size_t other = 0; other < config::HotkeyBindingRegistry.size(); ++other) {
                if (other == index) continue;
                if (!config::HasHotkeyConflict(loaded,
                        config::HotkeyBindingValue(loaded,
                            config::HotkeyBindingRegistry[other].id),
                        config::HotkeyBindingRegistry[index].id)) return false;
            }
        }
        return true;
    }

    bool TestLegacyOverlayToggleMigration(const std::filesystem::path& path)
    {
        const std::string legacy =
            "[Overlay]\nLanguage=Auto\n\n"
            "[Hotkeys]\nEnabled=false\nOverlayToggle=VK_41\n";
        if (!WriteText(path, legacy) || !Synchronize(path)) return false;
        const auto migratedText = ReadText(path);
        if (!HasAssignment(migratedText, "ToggleKey", "VK_41") ||
            migratedText.find("OverlayToggle=") != std::string::npos) {
            std::printf("legacy_overlay_text=FAIL\n");
            return false;
        }
        config::FeatureConfig configuration{};
        if (!config::LoadFeatureConfig(path, configuration,
            [](const std::filesystem::path&) { return false; }, [](std::string) {})) {
            std::printf("legacy_overlay_load=FAIL\n");
            return false;
        }
        if (configuration.overlayToggleKey != 'A') {
            std::printf("legacy_overlay_key=%d\n", configuration.overlayToggleKey);
            return false;
        }
        const bool syncChanged = Synchronize(path);
        const auto secondText = ReadText(path);
        const bool noOp = !syncChanged && secondText == migratedText;
        if (!noOp) {
            std::size_t diff = 0;
            while (diff < migratedText.size() && diff < secondText.size() &&
                migratedText[diff] == secondText[diff]) ++diff;
            std::printf("legacy_overlay_not_idempotent changed=%d diff=%zu before_size=%zu after_size=%zu\n",
                syncChanged, diff, migratedText.size(), secondText.size());
            std::printf("legacy_before=%s\nlegacy_after=%s\n",
                migratedText.substr(diff > 80 ? diff - 80 : 0, 220).c_str(),
                secondText.substr(diff > 80 ? diff - 80 : 0, 220).c_str());
        }
        return noOp;
    }

    bool TestLegacyDocumentationMigration(const std::filesystem::path& path)
    {
        const std::string legacy =
            "[Gameplay]\n"
            "; Enables ultrawide aspect-ratio correction during gameplay.\n"
            "Enabled=true\n\n"
            "[Cinematics]\n"
            "; Controls cinematic framing on ultrawide displays.\n"
            "; Auto   - use the automatic display-aspect policy.\n"
            "; Native - keep the game's original cinematic behavior.\n"
            "; 16:9   - force the native 16:9 cinematic frame.\n"
            "; 21:9   - force a 21:9 cinematic frame.\n"
            "; 32:9   - force a 32:9 cinematic frame.\n"
            "AspectRatio=Native\n\n"
            "[Dialogue]\n"
            "; Controls the native dialogue camera zoom.\n"
            "; Native   - use the game's original dialogue zoom, currently targeting 70°.\n"
            "; Adaptive - preserve the native optical zoom strength relative to the current gameplay FOV.\n"
            "; Reduced  - apply half of the Adaptive optical zoom strength.\n"
            ";            Example: 110° gameplay FOV -> Adaptive ≈90°, Reduced ≈100°.\n"
            "; Disabled - keep the current gameplay FOV during dialogue.\n"
            "Zoom=Native\n\n"
            "[Hotkeys]\n"
            "; Enables or disables all runtime hotkeys.\n"
            "Enabled=false\n"
            "; Supported keys: F1-F12, 0-9 and A-Z.\n"
            "; Key used to cycle the cinematic mode for the next cinematic.\n"
            "; Auto -> Native -> 16:9 -> 21:9 -> 32:9 -> Auto.\n"
            "; Does not affect a cinematic that is already playing.\n"
            "CinematicCycle=F9\n"
            "; Key used to cycle the dialogue zoom mode for the next dialogue.\n"
            "; Native -> Adaptive -> Reduced -> Disabled -> Native.\n"
            "; Does not affect a dialogue that is already in progress.\n"
            "DialogueCycle=F10\n";
        if (!WriteText(path, legacy) || !Synchronize(path)) return false;
        const auto migrated = ReadText(path);
        const bool cleaned =
            migrated.find("; Enables ultrawide aspect-ratio correction during gameplay.") == std::string::npos &&
            migrated.find("; Controls cinematic framing on ultrawide displays.") == std::string::npos &&
            migrated.find("; Controls the native dialogue camera zoom.") == std::string::npos &&
            migrated.find("; Supported keys: F1-F12, 0-9 and A-Z.") == std::string::npos &&
            HasAssignment(migrated, "Enabled", "false") &&
            HasAssignment(migrated, "Mode", "Adaptive") &&
            HasAssignment(migrated, "Mode", "Auto") &&
            migrated.find("AspectRatio=") == std::string::npos &&
            migrated.find("Zoom=") == std::string::npos &&
            HasAssignment(migrated, "CinematicCycle", "F9") &&
            HasAssignment(migrated, "DialogueCycle", "F10") &&
            ContainsGeneratedComments(migrated,
                config::canonical_ini_documentation::HotkeyCinematicAspectCycleComments);
        if (!cleaned || Synchronize(path)) return false;
        return ReadText(path) == migrated;
    }

    bool TestUserCommentsAreNotPreserved(const std::filesystem::path& path)
    {
        const std::string legacy =
            "[UserNotes]\n"
            "; my note: use this for screenshots\n"
            "Keep=true\n"
            "[Cinematics]\n"
            "; Controls cinematic framing on ultrawide displays.\n"
            "; Auto   - use the automatic display-aspect policy.\n"
            "; Native - keep the game's original cinematic behavior.\n"
            "; 16:9   - force the native 16:9 cinematic frame.\n"
            "; 21:9   - force a 21:9 cinematic frame.\n"
            "; 32:9   - force a 32:9 cinematic frame.\n"
            "AspectRatio=Native\n";
        if (!WriteText(path, legacy) || !Synchronize(path)) return false;
        const auto migrated = ReadText(path);
        const bool syncChanged = Synchronize(path);
        const auto secondText = ReadText(path);
        const bool result = migrated.find("my note: use this for screenshots") == std::string::npos &&
            migrated.find("# <Cinematics.Mode>") != std::string::npos &&
            migrated.find("STALKER2-MANAGED-DOC") == std::string::npos &&
            HasAssignment(migrated, "Enabled", "false") &&
            HasAssignment(migrated, "Mode", "Auto") &&
            HasAssignment(migrated, "Keep", "true") &&
            migrated.find("[UserNotes]") != std::string::npos &&
            migrated.find("AspectRatio=") == std::string::npos &&
            ContainsGeneratedComments(migrated,
                config::canonical_ini_documentation::CinematicAspectComments) &&
            !syncChanged && secondText == migrated;
        if (!result) std::printf("user_comments_removed note=%d canonical=%d key=%d section=%d docs=%d\n",
            migrated.find("my note: use this for screenshots") == std::string::npos,
            migrated.find("# <Cinematics.Mode>") != std::string::npos,
            HasAssignment(migrated, "Keep", "true"),
            migrated.find("[UserNotes]") != std::string::npos,
            ContainsGeneratedComments(migrated,
                config::canonical_ini_documentation::CinematicAspectComments));
        if (!result) std::printf("user_comments_remaining enabled_false=%d mode_auto=%d removes_legacy=%d stable=%d bytes_equal=%d\n",
            HasAssignment(migrated, "Enabled", "false"),
            HasAssignment(migrated, "Mode", "Auto"),
            migrated.find("AspectRatio=") == std::string::npos,
            !syncChanged, secondText == migrated);
        if (!result) {
            std::size_t diff = 0;
            while (diff < migrated.size() && diff < secondText.size() &&
                migrated[diff] == secondText[diff]) ++diff;
            std::printf("user_comments_idempotence changed=%d diff=%zu before_size=%zu after_size=%zu\n",
                syncChanged, diff, migrated.size(), secondText.size());
            std::printf("user_comments_before=%s\nuser_comments_after=%s\n",
                migrated.substr(diff > 80 ? diff - 80 : 0, 220).c_str(),
                secondText.substr(diff > 80 ? diff - 80 : 0, 220).c_str());
        }
        return result;
    }

    bool TestModifiedLegacyBlockIsReplaced(const std::filesystem::path& path)
    {
        const std::string modified =
            "[Cinematics]\n"
            "; Controls cinematic framing on ultrawide displays.\n"
            "; Auto - my custom aspect note.\n"
            "; Native - keep the game's original cinematic behavior.\n"
            "; 16:9   - force the native 16:9 cinematic frame.\n"
            "; 21:9   - force a 21:9 cinematic frame.\n"
            "; 32:9   - force a 32:9 cinematic frame.\n"
            "AspectRatio=Native\n";
        if (!WriteText(path, modified) || !Synchronize(path)) return false;
        const auto synchronized = ReadText(path);
        if (synchronized.find("# Auto - my custom aspect note.") != std::string::npos ||
            synchronized.find("# <Cinematics.Mode>") == std::string::npos ||
            !HasAssignment(synchronized, "Enabled", "false") ||
            !HasAssignment(synchronized, "Mode", "Auto") ||
            synchronized.find("AspectRatio=") != std::string::npos ||
            !ContainsGeneratedComments(synchronized,
                config::canonical_ini_documentation::CinematicAspectComments) ||
            Synchronize(path)) return false;

        auto damagedManagedBlock = synchronized;
        const auto begin = damagedManagedBlock.find(
            "# <Cinematics.Mode>");
        const auto end = damagedManagedBlock.find(
            "# </Cinematics.Mode>", begin);
        if (begin == std::string::npos || end == std::string::npos) return false;
        const auto endOfLine = damagedManagedBlock.find('\n', end);
        damagedManagedBlock.replace(begin,
            (endOfLine == std::string::npos ? damagedManagedBlock.size() : endOfLine + 1) - begin,
            "# <Cinematics.Mode>\n# altered documentation\n"
            "# </Cinematics.Mode>\n");
        if (!WriteText(path, damagedManagedBlock) || !Synchronize(path)) return false;
        const auto repaired = ReadText(path);
        return repaired.find("# altered documentation") == std::string::npos &&
            repaired.find("# <Cinematics.Mode>") != std::string::npos &&
            ContainsGeneratedComments(repaired,
                config::canonical_ini_documentation::CinematicAspectComments) &&
            !Synchronize(path) && ReadText(path) == repaired;
    }

    bool TestLegacyStyleNormalizationPreservesSettings(const std::filesystem::path& path)
    {
        const std::string oldStyle =
            "; STALKER 2 Camera Tweaks and Ultrawide for UE 5.5.4\n"
            "; Author: Elhait\n;\n"
            "[Gameplay]\n; Enables ultrawide aspect-ratio correction during gameplay.\n"
            "Enabled=true\nMode=RuntimeCombine\n"
            "\n[Dialogue]\n"
            "; Adaptive - Preserve the game's original optical zoom strength relative to current Gameplay FOV.\n"
            "; Examples - Gameplay FOV → dialogue FOV:\n; 90° → 70°.\n"
            "; 110° → approximately 90°.\n;\n"
            "; Adaptive - Preserve the game's original optical zoom strength relative to current Gameplay FOV.\n"
            "; Examples - Gameplay FOV → dialogue FOV:\n; 90° → 70°.\n"
            "; 110° → approximately 90°.\n;\n"
            "; Reduced - Use half of the Adaptive optical zoom strength.\n"
            "; Examples - Gameplay FOV → dialogue FOV:\n; 90° → approximately 80°.\n"
            "; 110° → approximately 100°.\n;\n"
            "; Disabled - Disable dialogue zoom and keep current Gameplay FOV.\n"
            "; Examples - Gameplay FOV → dialogue FOV:\n; 90° → 90°.\n"
            "; 110° → 110°.\nMode=Adaptive\n"
            "\n[Overlay]\nToggleKey=VK_2E\nLanguage=Auto\nFontSize=13\n"
            "PositionX=1073\nPositionY=37\nPositionMigrationComplete=1\n"
            "; Controls the optional Overlay subsystem.\n"
            "; Startup-only. Restart the game after changing this value.\n"
            "; false - disable the Overlay completely; camera corrections remain active.\n"
            "; true - enable the Overlay.\nEnabled=false\n";
        if (!WriteText(path, oldStyle) || !Synchronize(path)) return false;

        const auto normalized = ReadText(path);
        const auto overlay = normalized.find("[Overlay]");
        const auto enabled = normalized.find("Enabled", overlay);
        const auto toggle = normalized.find("ToggleKey", overlay);
        const auto language = normalized.find("Language", overlay);
        const auto font = normalized.find("FontSize", overlay);
        const auto service = normalized.find("PositionX", overlay);
        const auto nextSection = normalized.find("\n[Hotkeys]", overlay);
        const auto overlaySettings = overlay != std::string::npos &&
            nextSection != std::string::npos
            ? normalized.substr(overlay, nextSection - overlay) : std::string{};
        const auto duplicateAdaptive = normalized.find(
            "Adaptive - Preserve the game's original optical zoom strength",
            normalized.find("Adaptive - Preserve the game's original optical zoom strength") + 1);
        const bool layout = overlay != std::string::npos && enabled < toggle &&
            toggle < language && language < font && font < service &&
            nextSection != std::string::npos;
        if (!layout || normalized.find("\n;") != std::string::npos ||
            duplicateAdaptive != std::string::npos ||
            !HasBlankLineBeforeAssignment(normalized, "ToggleKey") ||
            !HasBlankLineBeforeAssignment(normalized, "Language") ||
            !HasBlankLineBeforeAssignment(normalized, "FontSize") ||
            !HasBlankLineBeforeAssignment(normalized, "PositionX") ||
            !HasBlankLineBeforeAssignment(normalized, "PositionMigrationComplete") ||
            !AssignmentsAreAdjacent(normalized, "PositionX", "PositionY") ||
            !HasAssignment(normalized, "PositionX", "1073") ||
            !HasAssignment(normalized, "PositionY", "37") ||
            !AllAssignmentsCompact(normalized) ||
            !HasCompactAssignment(overlaySettings, "Enabled", "false") ||
            !HasCompactAssignment(overlaySettings, "ToggleKey", "VK_2E") ||
            !HasCompactAssignment(overlaySettings, "Language", "Auto") ||
            !HasCompactAssignment(overlaySettings, "FontSize", "13") ||
            !HasCompactAssignment(overlaySettings, "PositionX", "1073") ||
            !HasCompactAssignment(overlaySettings, "PositionY", "37") ||
            !HasCompactAssignment(overlaySettings, "PositionMigrationComplete", "1")) {
            return false;
        }

        config::FeatureConfig configuration{};
        if (!config::LoadFeatureConfig(path, configuration,
            [](const std::filesystem::path&) { return false; }, [](std::string) {}) ||
            !configuration.gameplayEnabled || configuration.overlayEnabled ||
            configuration.gameplayMode != config::GameplayMode::RuntimeCombine) {
            return false;
        }
        if (!config::PersistConfigValue(path, "Overlay", "Enabled", "true",
            [](std::string) {}) || Synchronize(path)) return false;
        const auto persisted = ReadText(path);
        config::FeatureConfig persistedConfig{};
        const bool final = persisted.find("Enabled", persisted.find("[Overlay]")) != std::string::npos &&
            persisted.find("\n;") == std::string::npos && !Synchronize(path) &&
            AllAssignmentsCompact(persisted) &&
            ReadText(path) == persisted &&
            HasBlankLineBeforeAssignment(persisted, "ToggleKey") &&
            HasBlankLineBeforeAssignment(persisted, "Language") &&
            HasBlankLineBeforeAssignment(persisted, "FontSize") &&
            AssignmentsAreAdjacent(persisted, "PositionX", "PositionY") &&
            config::LoadFeatureConfig(path, persistedConfig,
                [](const std::filesystem::path&) { return false; }, [](std::string) {}) &&
            persistedConfig.overlayEnabled;
        return final;
    }
}

int main()
{
    const auto directory = MakeTestDirectory();
    if (directory.empty()) {
        std::printf("setup=FAIL\n");
        return 1;
    }

    const auto path = directory / "config.ini";
    const bool secureStaging = TestStagingNameOwnership(directory);
    const bool boundedRead = TestOversizedConfigIsNotReadOrRewritten(directory);
    std::printf("staging_name_ownership_and_hardlinks=%s oversized_config_refused=%s\n",
        secureStaging ? "PASS" : "FAIL", boundedRead ? "PASS" : "FAIL");
    const auto initialPath = directory / "initial.ini";
    const auto readOnlyPath = directory / "read-only.ini";
    const auto noOpPath = directory / "no-op.ini";
    const auto isolatedUpdatePath = directory / "isolated-update.ini";
    const auto recoveryPath = directory / "damaged.ini";
    const auto bomPath = directory / "bom-legacy.ini";
    const std::string original = "[Gameplay]\nEnabled=true\n";
    const bool normal = TestNormalPersistence(path, original);
    const bool midReadFailure = TestMidReadFailureIsRejected();
    const bool duplicateOccurrences = TestDuplicateManagedOccurrencesConverge(path);
    const bool duplicateOverlayPlacement = TestDuplicateOverlayPlacementRoundTrips(path);
    const bool logicalSettings = TestCanonicalRebuildPreservesLogicalSettings(path);
    std::printf("canonical_logical_settings=%s\n", logicalSettings ? "PASS" : "FAIL");
    const bool multiValueAtomic = TestMultiValuePersistenceIsAtomic(path);
    const bool staging = TestStagingFailurePreservesOriginal(path, original);
    const bool replacement = TestReplacementFailurePreservesOriginal(path, original);
    const bool readOnly = TestReadOnlyFilePreservesOriginal(readOnlyPath,
        "[Gameplay]\nEnabled=true\n");
    const bool templateNormal = TestNormalTemplateSynchronization(path, original);
    const bool templateStaging = TestTemplateStagingFailurePreservesOriginal(path, original);
    const bool templateReplacement = TestTemplateReplacementFailurePreservesOriginal(path, original);
    const bool templateRepair = TestTemplateRepairPreservesValuesAndDropsNotes(path);
    const bool unknownRecovery = TestUnknownConfigRecoveryBacksUpAndPreservesReadableValues(
        recoveryPath);
    const bool bomMigration = TestUtf8BomLegacyConfigMigration(bomPath);
    const bool diagnostics = TestDiagnosticsConfig(path);
    const bool overlayLanguage = TestOverlayLocaleCodeConfig(path);
    const bool overlayEnabled = TestOverlayEnabledConfig(path);
    const bool overlayFontSize = TestOverlayFontSizeConfig(path);
    const bool cinematicFovMode = TestCinematicFovModeConfig(path);
    const bool cinematicMigrationRoundTrips = TestCinematicMigrationRoundTrips(path);
    const bool dialogueMigrationRoundTrips = TestDialogueMigrationRoundTrips(path);
    const bool hotkeyBindings = TestHotkeyValidationAndPersistence(path);
    const bool hotkeyRegistry = TestHotkeyBindingRegistryRoundTrip(path);
    const bool legacyOverlayToggle = TestLegacyOverlayToggleMigration(path);
    const bool canonicalInitial = TestCanonicalInitialConfigDocumentation(initialPath);
    const bool booleanDocumentationResync =
        TestBooleanDocumentationResyncPreservesValues(path);
    const bool noOpFileIdentity = TestNoOpStartupAndPersistenceDoNotTouchFile(noOpPath);
    const bool isolatedPersistence = TestPersistenceChangesOnlyRequestedAssignment(
        isolatedUpdatePath);
    const bool legacyDocumentation = TestLegacyDocumentationMigration(path);
    const bool userCommentsRemoved = TestUserCommentsAreNotPreserved(path);
    const bool modifiedLegacyReplaced = TestModifiedLegacyBlockIsReplaced(path);
    const bool styleNormalization = TestLegacyStyleNormalizationPreservesSettings(path);

    std::error_code error;
    std::filesystem::remove_all(directory, error);
    std::printf("normal=%s mid_read_failure_rejected=%s duplicate_overlay_placement=%s multi_value_atomic=%s "
        "staging_failure_preserves=%s replacement_failure_preserves=%s read_only_preserves=%s "
        "template_normal=%s template_staging_failure_preserves=%s "
        "template_replacement_failure_preserves=%s template_repair_idempotent=%s unknown_recovery_backup=%s bom_legacy_migration=%s "
        "no_op_file_identity=%s isolated_persistence=%s boolean_documentation_resync=%s "
        "diagnostics_config=%s overlay_enabled=%s overlay_language=%s overlay_font_size=%s cinematic_fov_mode=%s cinematic_migration_round_trips=%s dialogue_migration_round_trips=%s duplicate_occurrences=%s hotkey_bindings=%s hotkey_registry=%s legacy_overlay_toggle=%s canonical_initial_documentation=%s legacy_documentation=%s user_comments_removed=%s modified_legacy_replaced=%s style_normalization=%s\n",
        normal ? "PASS" : "FAIL", midReadFailure ? "PASS" : "FAIL",
        duplicateOverlayPlacement ? "PASS" : "FAIL", multiValueAtomic ? "PASS" : "FAIL",
        staging ? "PASS" : "FAIL",
        replacement ? "PASS" : "FAIL", readOnly ? "PASS" : "FAIL",
        templateNormal ? "PASS" : "FAIL",
        templateStaging ? "PASS" : "FAIL", templateReplacement ? "PASS" : "FAIL",
        templateRepair ? "PASS" : "FAIL",
        unknownRecovery ? "PASS" : "FAIL",
        bomMigration ? "PASS" : "FAIL",
        noOpFileIdentity ? "PASS" : "FAIL",
        isolatedPersistence ? "PASS" : "FAIL",
        booleanDocumentationResync ? "PASS" : "FAIL",
        diagnostics ? "PASS" : "FAIL",
        overlayEnabled ? "PASS" : "FAIL",
        overlayLanguage ? "PASS" : "FAIL",
        overlayFontSize ? "PASS" : "FAIL",
        cinematicFovMode ? "PASS" : "FAIL",
        cinematicMigrationRoundTrips ? "PASS" : "FAIL",
        dialogueMigrationRoundTrips ? "PASS" : "FAIL",
        duplicateOccurrences ? "PASS" : "FAIL",
        hotkeyBindings ? "PASS" : "FAIL", hotkeyRegistry ? "PASS" : "FAIL",
        legacyOverlayToggle ? "PASS" : "FAIL", canonicalInitial ? "PASS" : "FAIL",
        legacyDocumentation ? "PASS" : "FAIL",
        userCommentsRemoved ? "PASS" : "FAIL",
        modifiedLegacyReplaced ? "PASS" : "FAIL",
        styleNormalization ? "PASS" : "FAIL");
    return secureStaging && boundedRead && normal && midReadFailure && duplicateOverlayPlacement && logicalSettings && multiValueAtomic && staging && replacement && readOnly && templateNormal && templateStaging &&
        templateReplacement && templateRepair && unknownRecovery && bomMigration && diagnostics && overlayLanguage &&
        overlayEnabled && overlayFontSize && cinematicFovMode && cinematicMigrationRoundTrips &&
        dialogueMigrationRoundTrips &&
        duplicateOccurrences && hotkeyBindings && hotkeyRegistry &&
        legacyOverlayToggle && canonicalInitial && noOpFileIdentity &&
        isolatedPersistence && booleanDocumentationResync && legacyDocumentation && userCommentsRemoved &&
        modifiedLegacyReplaced && styleNormalization ? 0 : 1;
}
