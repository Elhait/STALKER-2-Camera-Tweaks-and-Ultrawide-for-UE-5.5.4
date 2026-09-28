# DXGI factory-table bootstrap implementation

Date: 2026-09-28. Scope: bounded implementation of the independent factory-table observation path proposed in the causal report. No game launch, Git operation, release or package action was performed.

## Result

Factory export hooks remain installed as supplemental discovery. They no longer establish observation readiness by themselves. Readiness requires a successfully installed modern factory creation table whose table and relevant code owners are image-backed and pinned for process lifetime. The table is selected by querying supported factory interfaces widest-first.

The bootstrap creates one seed factory through the existing `CreateDXGIFactory1` original route and queries `IDXGIFactory2`. That object exists only to expose COM factory interfaces and tables. Its reference is released after installation; no seed device, window, swapchain, queue, renderer target or GPU work is created. A bootstrap-depth guard prevents the seed creation call from recursively entering export-result discovery.

Only successful real `CreateSwapChain*` callbacks can produce a pending renderer target, with the original call's actual device/queue association. Hook callbacks preserve native arguments and exact HRESULTs. Nested proxy/native callback chains deduplicate the same canonical swapchain COM identity; unrelated identities remain independently observable. The accepted pending-target validation, two-successful-Present threshold and delayed renderer/input activation are unchanged.

## Coverage and safety gates

- Shared-table coverage is accepted only when a supported modern factory interface exposes an in-image vtable with the required extent and all creation-method addresses are executable image code. The table and method-owning modules are pinned. If this validation fails, bootstrap reports unavailable; it does not patch a private, transient table.
- The SDK interface list is queried widest-first, and distinct returned interface tables are installed independently. The existing registry retains full SDK-bounded originals and does not truncate private tail entries.
- The inspected ReShade DXGI proxy delegates swapchain creation through its retained original factory's COM methods; the inspected OptiScaler path detours creation methods obtained from a real factory table. This source evidence supports the shared native-table route for those implementations. It is not a guarantee for arbitrary per-instance proxy tables or dispatch that bypasses COM virtual methods.
- Export observation can still add tables when it receives callbacks, but readiness does not rely on that event. A topology using a different private table while bypassing export callbacks remains unsupported by this bounded bootstrap and must fail closed/unavailable. An already-created swapchain cannot be recovered from a seed factory; this batch does not add enumeration, memory scanning, polling, retries or synthetic targets.
- If no safe shared modern table can be pinned, `InitializeImpl` does not mark discovery ready. Overlay remains unavailable through the existing optional-subsystem failure boundary; camera core initialization remains independent.

## Offline regression contracts

`dxgi_callback_harness` covers both capture orderings from the causal report:

1. A foreign wrapper captured its original before our export callback; its later call bypasses that callback. Bootstrap hooks the shared table, and a factory object that existed before bootstrap is still observed on its later real creation call.
2. A foreign wrapper captured our export callback; export discovery succeeds first, and bootstrap safely accepts the already-observed shared table.

Both paths verify exact native HRESULT and argument forwarding. Separate checks ensure successful export-hook state alone is not readiness, validated shared-table state is readiness, and nested proxy/native identities deduplicate without merging different objects. Existing factory extent, private-tail, queue/device ownership, two-Present activation, renderer lifetime and FS-01–FS-05 harnesses remain required regression coverage.

These deterministic fixtures establish the intended mechanics for modeled shared-table topologies. They do not emulate arbitrary injector binaries or prove that every runtime wrapper uses the seed's table; final runtime confirmation is required.

## Runtime evidence and remaining check

The user subsequently reported five consecutive successful launches of the bootstrap-enabled production build on the full graphics stack, with the Overlay working. The contemporaneous successful logs showed camera core modules `AVAILABLE`, pinned shared factory-table coverage, real application swapchain creation with the associated device/direct queue, the two-successful-Present activation gate, and renderer/input/Overlay activation. This is user-run evidence for that bootstrap build; it is not a claim that every graphics-wrapper topology is supported.

The later production startup-journal addition was rebuilt into the canonical root ASI after those five runs. It has not yet been runtime-tested against its exact current hash. One controlled launch of that journal-enabled build is the only remaining requested check: keep the normal graphics stack and canonical filename, preserve existing logs, and verify a fresh `STALKER2CameraTweaksStartup.log` records the expected bootstrap/discovery/target/activation milestones. Do not repeat the five-run matrix or require ten launches. If the Overlay fails to activate or bootstrap reports unavailable, retain all logs and identify the failed gate. No game launch was performed during the source/build validation turn.

## Offline validation completed

- `test.cmd`: PASS; the repository runner compiled/executed its 52/52 inventoried sources and the complete native harness set exited successfully. This includes the two capture-order paths, export-callback bypass, pinned-table rejection, native forwarding, renderer lifetime and FS-01–FS-05 contracts.
- Localization/resource/glyph checks: PASS for all 18 catalogs and 182 keys; embedded resources, font profiles and glyph coverage passed. Runtime shaping/bidi/RTL and visual rendering remain outside offline proof.
- Production `build.cmd`: PASS. It overwrote the canonical root `STALKER2CameraTweaks.asi` at 2026-09-28 01:32:12; size 2,760,704 bytes; SHA-256 is listed above. The root output was intentionally rebuilt after the separate validation-path build.
- `git diff --check`: PASS with no whitespace errors. Git printed line-ending normalization advisories for pre-existing mixed-LF working files.
- No game launch or Git/release/package action was performed. Runtime checklist above remains for the user's confirmation.

### Superseding validation snapshot — 2026-09-28

After adding the lightweight production startup journal, the complete suite was rerun: `test.cmd` passed with 53/53 inventoried runner sources compiled and executed. Localization/resource/font/glyph validation passed for 18 catalogs and 182 keys. Production `build.cmd` passed; the current canonical root ASI hash and the fact that it still needs the single journal-output runtime check are recorded in `TESTING_AND_RESEARCH.md`. The earlier 52/52 count and SHA above describe this report's original bootstrap-build snapshot, not the newest build.
