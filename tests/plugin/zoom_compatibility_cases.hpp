// Included inside the actual-adapter harness namespace, after its helpers.
bool RuntimeAdapterZoomEnable(safetyhook::MidHook& hook, unsigned ordinal)
{
    // Execute callbacks on another thread while the actual install is partial.
    // No callback may publish even when the first real hook is already enabled.
    const auto sequence = g_runtime.snapshotZoomSequence.load();
    const auto hits = g_zoomInHits.load();
    std::thread early([] {
        SafetyHookContext context{};
        SafeMidHookEntry<&TraceZoomIn>(context);
        SafeMidHookEntry<&TraceZoomOut>(context);
    });
    early.join();
    Check(g_runtime.snapshotZoomSequence == sequence && g_zoomInHits == hits &&
        !g_runtime.zoomObserverAvailable, "Partial pair admitted a callback");
    if (zoomEnableFailureOrdinal == ordinal) return false;
    return hook.enable().has_value();
}

void ZoomResolverAndInstallation()
{
    struct Image {
        std::uint8_t* bytes = static_cast<std::uint8_t*>(VirtualAlloc(nullptr, 8192,
            MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE));
        ~Image() { if (bytes) VirtualFree(bytes, 0, MEM_RELEASE); }
    } image;
    Check(image.bytes != nullptr, "Zoom image allocation");
    auto* dos = reinterpret_cast<IMAGE_DOS_HEADER*>(image.bytes);
    dos->e_magic = IMAGE_DOS_SIGNATURE; dos->e_lfanew = 0x80;
    auto* nt = reinterpret_cast<IMAGE_NT_HEADERS64*>(image.bytes + 0x80);
    nt->Signature = IMAGE_NT_SIGNATURE;
    nt->FileHeader.NumberOfSections = 1;
    nt->FileHeader.SizeOfOptionalHeader = sizeof(IMAGE_OPTIONAL_HEADER64);
    auto* section = IMAGE_FIRST_SECTION(nt);
    std::memcpy(section->Name, ".text", 5);
    section->VirtualAddress = 4096;
    section->Misc.VirtualSize = 4096;
    section->Characteristics = IMAGE_SCN_MEM_EXECUTE;
    auto* in = image.bytes + 4096;
    auto* out = in + 128;
    constexpr std::uint8_t inBytes[] = {
        0xF3,0x0F,0x10,0x40,0x4C,0xF3,0x0F,0x10,0x8E,0x38,0x01,0,0,0x0F,0x2E,0xC8};
    constexpr std::uint8_t outBytes[] = {
        0xF3,0x0F,0x10,0x40,0x50,0xF3,0x0F,0x10,0x8E,0x3C,0x01,0,0,0x0F,0x2E,0xC8};
    std::memset(in, 0x90, 4096);
    std::memcpy(in, inBytes, sizeof(inBytes));
    std::memcpy(out, outBytes, sizeof(outBytes));
    const auto resolve = [&] { return hooks::ResolveZoomTransitions(image.bytes); };
    auto result = resolve();
    Check(result && result.in == in + 13 && result.out == out + 13,
        "Diskless image native contract not resolved");
    nt->FileHeader.TimeDateStamp = 0xFFFFFFFF;
    image.bytes[1024] = 0xAB; // Changes image bytes/identity outside the contract.
    const auto changed = resolve();
    Check(changed && changed.in == result.in && changed.out == result.out,
        "Image identity changed native admission");
    std::memcpy(in + 256, inBytes, sizeof(inBytes));
    Check(!resolve() && resolve().inMatches == 2, "Duplicate IN accepted");
    std::memset(in + 256, 0x90, sizeof(inBytes));
    out[4] = 0x54;
    Check(!resolve(), "Wrong OUT displacement accepted");
    out[4] = 0x50;
    hooks::ExecutableSpan span{in, 4096};
    in[3] = 0x48;
    Check(!hooks::ValidateZoomTransition(in, true, span), "Wrong destination register accepted");
    in[3] = 0x40;
    Check(!hooks::ValidateZoomTransition(in, true, {in, 15}), "Truncated contract accepted");
    in[8] = 0x8F;
    Check(!hooks::ValidateZoomTransition(in, true, span), "Wrong source register accepted");
    in[8] = 0x8E;
    out[0] = 0x90;
    Check(!resolve(), "Missing OUT accepted");
    out[0] = 0xF3;

    const auto saved = g_executable;
    g_executable = reinterpret_cast<HMODULE>(image.bytes);
    for (unsigned failure : {1u, 2u}) {
        zoomEnableFailureOrdinal = failure;
        Check(!InstallZoomTransitionObservation(), "Injected enable failure accepted");
        Check(!g_runtime.zoomObserverAvailable && !g_zoomInHook && !g_zoomOutHook,
            "Failed pair retained active capability/hooks");
        Check(std::memcmp(in, inBytes, sizeof(inBytes)) == 0 &&
            std::memcmp(out, outBytes, sizeof(outBytes)) == 0, "Rollback changed native code");
        SafetyHookContext context{};
        const auto seq = g_runtime.snapshotZoomSequence.load();
        SafeMidHookEntry<&TraceZoomIn>(context);
        Check(g_runtime.snapshotZoomSequence == seq, "Post-rollback callback published");
    }
    zoomEnableFailureOrdinal = 0;
    Check(InstallZoomTransitionObservation() && g_runtime.zoomObserverAvailable,
        "Valid pair failed commit");
    Check(InstallZoomTransitionObservation(), "Repeated committed initialization failed");
    g_runtime.zoomObserverAvailable.store(false);
    g_zoomOutHook.reset(); g_zoomInHook.reset();
    Check(std::memcmp(in, inBytes, sizeof(inBytes)) == 0 &&
        std::memcmp(out, outBytes, sizeof(outBytes)) == 0, "Successful pair reset failed");
    g_executable = saved;
    // The remaining synthetic callback scenario explicitly models a committed pair.
    g_runtime.zoomObserverAvailable.store(true);
}
