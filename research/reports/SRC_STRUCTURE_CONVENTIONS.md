# SRC_STRUCTURE_CONVENTIONS

Date: 2026-10-11  
Status: proposed standard; not an instruction to implement or reopen B1–B7.

## Purpose

Use one responsibility-based standard across src without imposing identical directory depths. Structure should communicate owners, profiles and dependencies; filename symmetry and line-count reduction are not architecture goals.

## Directory rules

1. Keep a cohesive module flat while its files are easily navigable and have one domain boundary. Current config (6), dialogue (4), gameplay (8), cinematics (10) and camera (14) do not need new subdirectories.
2. Create a thematic directory only for a genuine distinct responsibility or build/lifetime profile. Overlay's composition, input, localization, presentation and presenter groups qualify; the three standalone language-state research files qualify as one diagnostics/language_state group.
3. Keep an OS boundary explicit: platform/win32. Keep hooks/signatures for version-independent resolver data. A one-file group is allowed when the boundary is real.
4. Shared product identity belongs in localization, not optional UI implementation. The shared locale registry and resource-ID macros may be colocated. Catalog parsing, font loading, resource payload embedding and UI localization stay overlay/localization.
5. Do not move an owner-bound callback to a domain directory just because its feature name matches. plugin/runtime.cpp remains the composition root; its integration role is documented.
6. No generic utils/common/managers directories; no extra namespace/API, files split/merged, wrappers or second owners for organizing paths.
7. A standalone research build must remain distinguishable from the production registry. diagnostics/language_state is a research-profile group, not a new production subsystem.

## Filenames and pairs

- Use lowercase snake_case names, with established compound domain terms allowed (matchgameplay).
- Keep implementation and interface basename pairs adjacent. Do not create empty cpp counterparts for header-only helpers or artificial headers for DLL entries.
- _runtime: owner/integration execution; _state: state model or lifecycle primitive; _policy: decisions; _view: observational projection; _transaction: coordinated decisions and side-effect contract; _trace/_provenance: diagnostics; _repository: persistence; _template: generated/config document shaping.
- A suffix is descriptive, not mandatory. Do not rename composition_presenter_state, imgui_d3d11_renderer, cinematic_initialization or camera_integration merely to align suffixes.
- Use _adapter only when a file actually adapts an external ABI/context. Native adapters currently remain in runtime.cpp; no artificial adapter files proposed.
- Retain .hpp for project C++, .h for the RC-compatible macro resource interface, .rc for resource payloads. Do not replace resource macros with C++ constructs.

## Public/private boundaries

Public here means intermodule project API, not a supported external SDK or exported ABI.

| Boundary | Rule / exception |
| --- | --- |
| plugin/runtime.hpp and runtime_settings.hpp | Supported project lifecycle/settings/readback surface; preserve typed mutations and semantic facts |
| plugin/runtime_internal.hpp | Plugin-private forward declaration/access; concrete RuntimeState definition remains in runtime.cpp; test owner probe is an explicit consumer exception |
| plugin transaction headers | Internal integration policy, directly exercised by harnesses; not a license for other modules to acquire mutable runtime state |
| domain headers | Export calculation/data/store contracts actually used; caller-held store instances do not become global owners |
| Overlay facade | External C linkage startup/readiness/wake contracts remain unchanged; renderer/private resources are not promoted to plugin API |
| Cinematic initialization | Existing FeatureStatus return/import is an accepted integration exception; do not add duplicate capability types |
| diagnostics inline state | Explicit inline shared linkage allowed; no TU-local mutable singleton definitions copied into headers |
| localization resource IDs | Shared numeric build identity, no resource loader or mutable UI state |

Do not introduce public/private subfolders as a mass move. Current named internal header and owner-private definitions communicate the meaningful distinction. When a future interface genuinely needs isolation, approve it as architecture work with consumers and lifecycle evidence, not as a directory cleanup.

## Include conventions

- Project-local C++ includes: quoted paths relative to including file; own header first where applicable.
- Cross-module includes: explicit relative paths to owned interfaces. No new include-root/alias system for this task.
- SDK/vendor includes: angle brackets and build-provided include directories.
- RC includes: retain repository-root source path, since build/test/evidence runners invoke rc from repository root. Payload paths stay locales and assets; only ID-header reference changes.
- Generated INI documentation: retain canonical_ini_documentation.hpp through build-artifacts/generated and the existing generator; not source-controlled generated output.
- Include declared dependencies directly for future code. Existing transitive cleanup requires separate compiler-isolation validation, not broad edits during moves.
- Windows include casing varies historically; target is Windows. No case-only mass edits or unsupported portability claim.
- Never forward-declare a concrete value-member dependency to hide an actual semantic boundary. Header-only helpers stay inline/templates/constexpr where required.
- Uniform relevant compile defines across TUs are an ODR/lifecycle requirement. Moving paths cannot justify changing macros or duplicating storage.

## Build/test/reference maintenance

build.cmd remains the canonical production source registry. Supported diagnostic and settings wrappers derive from it. runtime_native_adapter_inputs.ps1 extracts that registry and excludes runtime.cpp/dll_entry to avoid double inclusion.

For each approved move update source includes, direct build inputs, harness includes, resource includes, source-inspecting audit lookups and independently built research tools together. Preserve artifact filenames, link libraries, compiler options and resource integers. Keep generated objects, response files and research ASIs out of release assets.

A historical research record is a dated snapshot, not an operational script. Do not rewrite every old path; fix only active instructions and annotate superseding status where necessary.

## Documented exceptions / approval boundaries

- runtime.cpp stays large because it owns native ordering, correction gate and cross-domain lifecycle; B1–B7 are complete.
- camera_writer_transaction.hpp stays header-only and large; no decomposition approval.
- locale_registry.hpp was a single shared identity file; proposed resource header colocation strengthens this responsibility without moving the UI API.
- hooks/signatures and platform/win32 retain their real boundaries.
- startup_journal/startup_timeline/overlay_activation_trace inline storage is intentional, not per-TU duplication.
- hook_set.cpp is currently an include-only build input: legacy candidate, not deletion authority.
- Presenter experiment gates require separate Review; missing inactive experiment headers do not justify altering ordinary renderer code.
- Existing Native/legacy configuration compatibility and runtime evidence gaps are outside organizational work.

Record any future exception with owner, consumers, rationale, validation boundary and revisit condition. Leave unresolved N03–N06 statuses untouched. A fully approved mechanical plan can continue automatically after passing batch gates; architecture changes still require separate approval.

