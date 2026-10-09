#include "../../src/config/config_repository.hpp"
#include "../../src/overlay/placement_config.hpp"
#include "../../src/overlay/placement_save_state.hpp"

#include <Windows.h>

#include <barrier>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <thread>

namespace
{
    bool TestPlacementSaveDisposition()
    {
        overlay::PlacementSaveState state;
        if (state.disposition() != overlay::PlacementSaveDisposition::Clean)
            return false;
        state.PositionChanged();
        if (state.ShouldAttemptSave(true) || !state.ShouldAttemptSave(false))
            return false;
        state.CompleteAttempt(false);
        if (state.disposition() !=
            overlay::PlacementSaveDisposition::FailedUntilNextMovement)
            return false;
        for (int frame = 0; frame < 120; ++frame)
            if (state.ShouldAttemptSave(false)) return false;
        state.PositionChanged();
        if (state.ShouldAttemptSave(true) || !state.ShouldAttemptSave(false))
            return false;
        state.CompleteAttempt(true);
        return state.disposition() == overlay::PlacementSaveDisposition::Clean &&
            !state.ShouldAttemptSave(false);
    }

    bool WriteText(const std::filesystem::path& path, const std::string& text)
    {
        std::ofstream output(path, std::ios::binary | std::ios::trunc);
        output << text;
        return static_cast<bool>(output);
    }

    std::string ReadText(const std::filesystem::path& path)
    {
        std::ifstream input(path, std::ios::binary);
        return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
    }

    bool HasAssignment(const std::string& text, const char* key, const char* value)
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

    bool FlushProfileCache(const std::filesystem::path& path)
    {
        return WritePrivateProfileStringW(nullptr, nullptr, nullptr,
            path.c_str()) != FALSE;
    }

    bool TestLoadAndSave(const std::filesystem::path& path)
    {
        FlushProfileCache(path);
        if (!WriteText(path, "[Gameplay]\nEnabled=true\n[Overlay]\n"
            "Language=Auto\nPositionX=35\nPositionY=48\n"
            "PositionMigrationComplete=1\nCustomValue=keep\n")) {
            std::printf("placement_test=setup_write_failed\n");
            return false;
        }

        float x{};
        float y{};
        bool migrated = true;
        if (!overlay::placement_config::Load(path.wstring(), {}, x, y, migrated) ||
            migrated || x != 35.0f || y != 48.0f) {
            std::printf("placement_test=load_failed migrated=%d x=%.0f y=%.0f\n",
                migrated ? 1 : 0, x, y);
            return false;
        }

        if (!overlay::placement_config::Save(path.wstring(), 640, 360)) {
            std::printf("placement_test=save_failed\n");
            return false;
        }
        FlushProfileCache(path);
        const int storedX = static_cast<int>(GetPrivateProfileIntW(L"Overlay",
            L"PositionX", -1, path.c_str()));
        const int storedY = static_cast<int>(GetPrivateProfileIntW(L"Overlay",
            L"PositionY", -1, path.c_str()));
        const bool keptCustom = HasAssignment(ReadText(path), "CustomValue", "keep");
        if (storedX != 640 || storedY != 360 || !keptCustom)
            std::printf("placement_test=saved_values_mismatch x=%d y=%d custom=%d\n",
                storedX, storedY, keptCustom ? 1 : 0);
        return storedX == 640 && storedY == 360 && keptCustom;
    }

    bool TestLegacyMigrationAndFollowingSettingWrite(
        const std::filesystem::path& configPath,
        const std::filesystem::path& legacyPath)
    {
        FlushProfileCache(configPath);
        if (!WriteText(configPath, "[Gameplay]\nEnabled=true\n[Overlay]\n"
                "Language=uk\nPositionMigrationComplete=0\n") ||
            !WriteText(legacyPath, "[Window]\nX=777\nY=333\n")) {
            std::printf("placement_test=migration_setup_failed\n");
            return false;
        }

        float x{};
        float y{};
        bool migrated = false;
        if (!overlay::placement_config::Load(configPath.wstring(),
                legacyPath.wstring(), x, y, migrated) ||
            !migrated || x != 777.0f || y != 333.0f) {
            std::printf("placement_test=migration_load_failed migrated=%d x=%.0f y=%.0f\n",
                migrated ? 1 : 0, x, y);
            return false;
        }
        FlushProfileCache(configPath);

        const auto log = [](std::string) {};
        if (!config::PersistConfigValue(configPath, "Gameplay", "Enabled",
                "false", log)) {
            std::printf("placement_test=following_setting_persist_failed\n");
            return false;
        }

        const auto contents = ReadText(configPath);
        if (!HasAssignment(contents, "Language", "uk") ||
            !HasAssignment(contents, "Enabled", "false") ||
            !HasAssignment(contents, "PositionX", "777") ||
            !HasAssignment(contents, "PositionY", "333") ||
            !HasAssignment(contents, "PositionMigrationComplete", "1")) {
            std::printf("placement_test=migration_values_not_preserved\n");
            return false;
        }

        migrated = true;
        const bool reload = overlay::placement_config::Load(configPath.wstring(),
                legacyPath.wstring(), x, y, migrated) && !migrated &&
            x == 777.0f && y == 333.0f;
        if (!reload)
            std::printf("placement_test=migration_not_idempotent x=%.0f y=%.0f migrated=%d\n",
                x, y, migrated ? 1 : 0);
        return reload;
    }

    bool TestFailureLeavesExistingConfigUntouched(
        const std::filesystem::path& configPath,
        const std::filesystem::path& missingDirectory)
    {
        const std::string original = "[Overlay]\nPositionX=11\nPositionY=22\n";
        if (!WriteText(configPath, original)) return false;
        const auto invalidPath = missingDirectory / L"config.ini";
        const bool rejected = !overlay::placement_config::Save(
            invalidPath.wstring(), 900, 901);
        return rejected && ReadText(configPath) == original;
    }

    bool TestConcurrentSettingAndPlacementWrites(const std::filesystem::path& path)
    {
        constexpr int iterations = 24;
        int reproduced = 0;
        int settingLost = 0;
        int xLost = 0;
        int yLost = 0;
        for (int iteration = 0; iteration < iterations; ++iteration) {
            const std::string initial = "[Gameplay]\nEnabled=true\n[Overlay]\n"
                "PositionX=30\nPositionY=30\nPositionMigrationComplete=1\n";
            if (!WriteText(path, initial)) return false;
            FlushProfileCache(path);

            std::barrier start(3);
            bool settingResult = false;
            bool placementResult = false;
            std::thread settingWriter([&] {
                start.arrive_and_wait();
                settingResult = config::PersistConfigValue(path, "Gameplay",
                    "Enabled", "false", [](std::string) {});
            });
            std::thread placementWriter([&] {
                start.arrive_and_wait();
                placementResult = overlay::placement_config::Save(path.wstring(),
                    500 + iteration, 250 + iteration);
            });
            start.arrive_and_wait();
            settingWriter.join();
            placementWriter.join();
            FlushProfileCache(path);

            const auto contents = ReadText(path);
            const bool settingKept = settingResult &&
                HasAssignment(contents, "Enabled", "false");
            const int storedX = static_cast<int>(GetPrivateProfileIntW(
                L"Overlay", L"PositionX", -1, path.c_str()));
            const int storedY = static_cast<int>(GetPrivateProfileIntW(
                L"Overlay", L"PositionY", -1, path.c_str()));
            const bool xKept = placementResult && storedX == 500 + iteration;
            const bool yKept = placementResult && storedY == 250 + iteration;
            if (!settingKept || !xKept || !yKept) ++reproduced;
            if (!settingKept) ++settingLost;
            if (!xKept) ++xLost;
            if (!yKept) ++yLost;
        }

        std::printf("concurrent_ini_writers=%s iterations=%d/%d setting_lost=%d "
            "x_lost=%d y_lost=%d\n", reproduced == 0 ? "PASS" : "FAIL",
            iterations, iterations, settingLost, xLost, yLost);
        return reproduced == 0;
    }

    std::filesystem::path MakeTestDirectory()
    {
        wchar_t temporaryPath[MAX_PATH]{};
        if (GetTempPathW(MAX_PATH, temporaryPath) == 0) return {};
        const auto directory = std::filesystem::path(temporaryPath) /
            (L"STALKER2PlacementConfigHarness-" +
                std::to_wstring(GetCurrentProcessId()));
        std::error_code error;
        std::filesystem::remove_all(directory, error);
        std::filesystem::create_directories(directory, error);
        return error ? std::filesystem::path{} : directory;
    }
}

int main()
{
    const auto directory = MakeTestDirectory();
    if (directory.empty()) {
        std::puts("placement_config_setup=FAIL");
        return 1;
    }

    const auto loadSavePath = directory / L"load-save.ini";
    const auto migrationPath = directory / L"migration.ini";
    const auto failurePath = directory / L"failure.ini";
    const auto concurrencyPath = directory / L"concurrency.ini";
    const auto legacyPath = directory / L"STALKER2CameraTweaksOverlay.ini";
    const bool loadSave = TestLoadAndSave(loadSavePath);
    const bool migration = TestLegacyMigrationAndFollowingSettingWrite(
        migrationPath, legacyPath);
    const bool failurePreserves = TestFailureLeavesExistingConfigUntouched(
        failurePath, directory / L"missing-parent");
    const bool saveDisposition = TestPlacementSaveDisposition();
    const bool concurrentWrites = TestConcurrentSettingAndPlacementWrites(concurrencyPath);

    std::error_code error;
    std::filesystem::remove_all(directory, error);
    std::printf("placement_load_save=%s migration_and_setting_write=%s "
        "failure_path=%s concurrency_exercised=%s\n",
        loadSave ? "PASS" : "FAIL", migration ? "PASS" : "FAIL",
        failurePreserves ? "PASS" : "FAIL", concurrentWrites ? "YES" : "NO");
    std::printf("placement_save_disposition=%s\n",
        saveDisposition ? "PASS" : "FAIL");
    return loadSave && migration && failurePreserves && saveDisposition &&
        concurrentWrites ? 0 : 1;
}
