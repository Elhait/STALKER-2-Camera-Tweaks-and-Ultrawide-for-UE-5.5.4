#include "runtime.hpp"
#include "../diagnostics/startup_journal.hpp"
#include "../diagnostics/startup_timeline.hpp"

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID reserved)
{
    if (reason == DLL_PROCESS_DETACH) {
        plugin::NotifyProcessDetach(reserved != nullptr);
        return TRUE;
    }
    if (reason != DLL_PROCESS_ATTACH) return TRUE;

#if defined(OVERLAY_STARTUP_TIMELINE)
    diagnostics::startup::ProcessAttach(module);
#endif
#if defined(OVERLAY_STARTUP_JOURNAL)
    diagnostics::startup_journal::ProcessAttach(module);
#endif
    plugin::SetModuleHandle(module);
    DisableThreadLibraryCalls(module);
#if defined(OVERLAY_STARTUP_TIMELINE)
    diagnostics::startup::Mark("dllmain_thread_request");
#endif
    const auto thread = CreateThread(nullptr, 0, plugin::InitializeThread, nullptr, 0, nullptr);
#if defined(OVERLAY_STARTUP_TIMELINE)
    diagnostics::startup::Mark("dllmain_thread_created", thread, thread ? 0 : GetLastError());
#endif
    if (thread) CloseHandle(thread);
#if defined(OVERLAY_STARTUP_TIMELINE)
    diagnostics::startup::Mark("dllmain_attach_return");
#endif
    return TRUE;
}
