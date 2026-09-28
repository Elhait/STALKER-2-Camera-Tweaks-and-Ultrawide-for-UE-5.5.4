#define OVERLAY_STARTUP_TIMELINE
#include "../../src/diagnostics/startup_timeline.hpp"
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <thread>
#include <vector>

namespace
{
    void Check(bool condition, const char* message)
    {
        if (!condition) throw std::runtime_error(message);
    }
    UNICODE_STRING Text(wchar_t* value)
    {
        UNICODE_STRING text{};
        text.Buffer = value;
        text.Length = static_cast<USHORT>(std::wcslen(value) * sizeof(wchar_t));
        text.MaximumLength = text.Length;
        return text;
    }
}

int main()
{
    try {
        using namespace diagnostics::startup;
        ProcessAttach(GetModuleHandleW(nullptr));
        Check(events[0].qpc != 0 && events[0].utc != 0 && events[0].thread != 0,
            "DLL entry has independently usable QPC/wall/thread anchors");
        wchar_t basename[] = L"PLUGIN.ASI";
        wchar_t path[] = L"C:\\fixture\\Тест\\PLUGIN.ASI";
        auto base = Text(basename);
        auto full = Text(path);
        NotificationData loaded{0, &full, &base, reinterpret_cast<void*>(0x1234), 4096};
        const auto position = next.load();
        ModuleNotice(1, &loaded, nullptr);
        path[0] = L'X'; // Loader-owned UNICODE_STRING must not survive by reference.
        Check(events[position].published.load() && events[position].path[0] == L'C',
            "Notification copies its payload before publication");
        Check(events[position].qpc == 0 && events[position].thread == 0,
            "Loader notification does not call foreign clock/thread APIs");
        Check(events[position].address == loaded.base && events[position].value == loaded.size,
            "Notification layout matches SDK payload");
        ModuleNotice(2, &loaded, nullptr);
        const auto notices = next.load();
        ModuleNotice(0, &loaded, nullptr);
        ModuleNotice(1, nullptr, nullptr);
        wchar_t unrelated[] = L"unrelated.dll";
        base = Text(unrelated);
        ModuleNotice(1, &loaded, nullptr);
        Check(next.load() == notices, "Invalid/unrelated notifications do not consume capacity");
        BeginNotifications();
        Check(notificationCookie != nullptr, "Real Windows loader accepts notification ABI");
        wchar_t systemPath[32768]{};
        const auto systemLength = GetSystemDirectoryW(systemPath, 32768);
        Check(systemLength != 0 && systemLength < 32750, "System module path available");
        wcscat_s(systemPath, L"\\winmm.dll");
        Check(!GetModuleHandleW(systemPath), "Live loader fixture must begin with an unloaded system module");
        const auto beforeLoad = next.load();
        const auto loadedModule = LoadLibraryExW(systemPath, nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
        Check(loadedModule != nullptr, "Live loader fixture loads the exact system DLL");
        bool observedLoad = false;
        for (unsigned i = beforeLoad; i < next.load() && i < Capacity; ++i)
            if (events[i].published.load() && events[i].address == loadedModule &&
                std::string(events[i].kind) == "MODULE_LOAD") observedLoad = true;
        Check(observedLoad, "Real loader notification reaches the recorder before LoadLibrary returns");
        const auto beforeUnload = next.load();
        FreeLibrary(loadedModule);
        bool observedUnload = false;
        for (unsigned i = beforeUnload; i < next.load() && i < Capacity; ++i)
            if (events[i].published.load() && events[i].address == loadedModule &&
                std::string(events[i].kind) == "MODULE_UNLOAD") observedUnload = true;
        Check(observedUnload, "Real unload notification copies the payload before DLL memory goes away");
        wchar_t longPath[PathCapacity + 20]{};
        for (unsigned i = 0; i < PathCapacity + 19; ++i) longPath[i] = L'a';
        base = Text(basename);
        full = Text(longPath);
        const auto truncatedPosition = next.load();
        ModuleNotice(1, &loaded, nullptr);
        Check(events[truncatedPosition].truncated &&
            events[truncatedPosition].path[PathCapacity - 1] == 0,
            "Oversized loader names truncate explicitly and terminate");
        Check(FactoryEnter(1, reinterpret_cast<void*>(0x3456)), "First factory event recorded");
        FactoryReturn(1, E_ACCESSDENIED, nullptr);
        const auto firstFactoryCount = next.load();
        Check(!FactoryEnter(1, reinterpret_cast<void*>(0x3456)) && next.load() == firstFactoryCount,
            "Repeated factory calls cannot create recurring log traffic");
        SnapshotExports("fixture", nullptr);
        Check(events[next.load() - 1].address == nullptr,
            "Missing exports remain observational and do not load DXGI");
        Flush();
        ToggleProbe();
        const auto probeCount = next.load();
        ToggleProbe();
        Check(next.load() == probeCount, "Unavailable Overlay can drain once without toggling input");
        std::vector<std::thread> writers;
        for (unsigned i = 0; i < 4; ++i) writers.emplace_back([] {
            for (unsigned j = 0; j < Capacity; ++j) Mark("fixture_overflow");
        });
        for (auto& thread : writers) thread.join();
        Check(next.load() > Capacity, "Overflow is bounded rather than reallocating");
        for (const auto& event : events)
            Check(event.published.load(std::memory_order_acquire), "Distinct concurrent slots publish completely");
        Flush();
        wchar_t tracePath[32768]{};
        GetModuleFileNameW(nullptr, tracePath, 32768);
        auto* traceBase = std::wcsrchr(tracePath, L'\\') + 1;
        wcscpy_s(traceBase, static_cast<SIZE_T>(32768 - (traceBase - tracePath)),
            L"STALKER2CameraTweaksStartup.log");
        std::ifstream input(tracePath, std::ios::binary);
        const std::string text{std::istreambuf_iterator<char>(input), {}};
        Check(text.find("STARTUP_TIMELINE v=2") != std::string::npos &&
            text.find("ATTACH_STACK") != std::string::npos &&
            text.find("MODULE_LOAD") != std::string::npos &&
            text.find("FACTORY_RETURN") != std::string::npos &&
            text.find("first_toggle_probe") != std::string::npos &&
            text.find("TIMELINE_OVERFLOW") != std::string::npos,
            "Trace drains startup, native-result, missing-activation and overflow facts");
        std::cout << "startup_timeline_harness: PASS\n";
        return 0;
    } catch (const std::exception& exception) {
        std::cerr << exception.what() << '\n';
        return 1;
    }
}
