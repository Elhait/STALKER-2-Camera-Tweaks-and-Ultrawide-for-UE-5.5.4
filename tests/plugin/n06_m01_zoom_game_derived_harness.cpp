#define main N06UnusedAdapterHarnessMain
#include "runtime_native_adapter_harness.cpp"
#undef main
#include "n06_m01_zoom_game_derived_fixtures.hpp"

LONG WINAPI N06M01ExceptionFilter(EXCEPTION_POINTERS* exception) {
    std::cerr << "fixture exception=0x" << std::hex
        << exception->ExceptionRecord->ExceptionCode << " rip=0x"
        << exception->ContextRecord->Rip << " rsp=0x"
        << exception->ContextRecord->Rsp << " rdx=0x"
        << exception->ContextRecord->Rdx << " access="
        << exception->ExceptionRecord->ExceptionInformation[0] << " address=0x"
        << exception->ExceptionRecord->ExceptionInformation[1] << std::dec << std::endl;
    return EXCEPTION_EXECUTE_HANDLER;
}

int main(int argc, char** argv) {
    try {
        SetUnhandledExceptionFilter(N06M01ExceptionFilter);
        Check(argc == 3, "Expected fixture name and diagnostic mode");
        const std::string fixture = argv[1];
        diagnosticMode = std::string(argv[2]) == "on";
        Check(argv[2] == std::string("on") || argv[2] == std::string("off"),
            "Diagnostic mode argument must be on or off");
        if (fixture == "zoom-in") RunZoomGameDerivedFixture(true);
        else if (fixture == "zoom-out") RunZoomGameDerivedFixture(false);
        else throw std::runtime_error("Unknown game-derived Zoom fixture");
        Check(!logHeldGate, "Production Log emitted while correction gate held");
        std::cout << fixture << " game-derived diagnostic="
            << (diagnosticMode ? "on" : "off") << ": PASS\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "N06-M01 fixture: FAIL: " << error.what() << '\n';
        return 1;
    }
}
