# v1.0.0 to Future v2.0 — Change Report

Date: 2026-09-26  
Comparison: tag v1.0.0 to the current staged index  
Status: descriptive diff report; no source changes made by this report

## Comparison scope

HEAD is commit 3120a05 (v1.0.0). The future v2.0 candidate has not been
committed or tagged; the candidate is the staged index. There are no unstaged
tracked changes. The index differs from v1.0.0 by **271 files, 95,788
insertions and 7,903 deletions**. The production-source, harness and build
entry-point subset covers 96 files with 13,171 insertions and 711 deletions.

Most of the change is the new production overlay, vendored UI/font resources,
localization, and the expanded research and task archive. This is a major
product integration change around the existing camera correction features.

## Product behavior and runtime integration

### In-game settings overlay

The candidate adds a D3D12/Win32 Dear ImGui overlay to the production ASI. The
overlay includes Gameplay, Cinematics, Dialogue, Overlay, Hotkeys and
Camera State views, runtime status descriptions, selector help with examples,
notifications, text-size selection, language selection and draggable window
placement. The interactive toggle defaults to Delete; Esc can dismiss the
open overlay, and popup/rebinding interactions receive their own Escape
handling. The existing mode-cycle hotkeys remain separately configurable and
disabled by default.

Overlay integration is split between swapchain/window discovery and renderer
resource lifetime. Renderer state tracks device/swapchain readiness, frame
contexts, resize/recreation, font atlas rebuilding and failure/disable states.
Input capture, selector interactions, notifications, localization and camera
presentation are implemented in dedicated overlay modules. The overlay reads
typed runtime settings and observational camera snapshots; its controls send
mutations through the runtime settings API.

Principal additions are in src/overlay/, with the production callback and
settings integration in src/plugin/runtime.cpp. Dear ImGui v1.91.9b sources
and Win32/DX12 backends are added under external/imgui/.

### Camera and runtime behavior

The existing Gameplay HorPlus/AspectRecalculation, cinematic aspect/FOV and
Dialogue zoom feature families remain. The candidate adds or formalizes:

- a pure Gameplay enabled-transition decision model for defer, restore, apply
  and already-applied cases;
- a shared Forced21x9Aspect value based on 3440×1440 for production policy
  and overlay assessment;
- additional camera snapshot evidence for active zoom and retained Gameplay
  FOV, including validity and provenance;
- a public typed settings contract for applying, reading, and persisting
  runtime mutations;
- stable hotkey binding identities and one registry for defaults and INI
  locations; and
- gated runtime lock/callback telemetry.

Most of the integration changes are in src/plugin/runtime.cpp; supporting
contracts live in src/plugin/runtime_settings.hpp, src/gameplay/,
src/camera/, src/cinematics/, and src/diagnostics/. The source changes
preserve Runtime as the owner of camera effects and feature lifecycle. The
overlay's camera panel is a read-only presentation projection.

### Configuration and persistence

The main STALKER2CameraTweaks.ini now also owns overlay settings: toggle key,
language, font size and position. The config layer adds locale/font parsing,
the binding registry, managed comment generation, serialized writes, and
one-time import of legacy overlay toggle/placement data. Placement saves and
feature setting writes share the config repository update path. Managed INI
documentation is generated from the canonical English localization entries.

Relevant files are src/config/feature_config.*, src/config/config_repository.*,
src/config/config_template.cpp, and tools/generate_ini_documentation.ps1.
research/ue4ss/STALKER2CameraTweaks/STALKER2CameraTweaks.ini is updated as the
repository's example config.

## Localization and visual assets

The overlay adds 18 catalogs: Arabic, Czech, German, Latin American Spanish,
European Spanish, French, Italian, Japanese, Korean, Polish, Brazilian
Portuguese, Russian, Serbian Cyrillic, Turkish, Ukrainian, Simplified Chinese,
Traditional Chinese and English. A locale registry maps codes to display
names, script/font profiles and recommended initial text size. Auto reads
the game's Interface Language at overlay opening; a manual locale selection
remains in effect until Auto is selected.

Font assets include Proggy Vector and small catalog-specific Noto Sans subsets
for common glyphs, Arabic and regional Japanese/Korean/Simplified Chinese/
Traditional Chinese profiles. Font license notices, source/version
provenance, hashes and a glyph contract are added alongside the assets.
Resources are embedded in the ASI; production builds do not rely on fonts
installed on the user's machine. The catalog/resource audit checks key parity,
placeholders, encoding and glyph availability. It does not establish Arabic
shaping, bidi/RTL layout or runtime visual correctness.

## Build, tests and diagnostics

build.cmd now generates canonical INI documentation, embeds localization
resources and builds the combined production overlay. It uses the
OVERLAY_PRODUCTION, OVERLAY_SETTINGS_FRONTEND and OVERLAY_COMBINED
defines. build-overlay-settings.cmd is a production-build compatibility
alias. POC/discovery and language-watcher scripts are research paths; the
project docs identify the retired standalone overlay profiles as unsupported.

The staged test infrastructure adds overlay lifecycle, renderer state,
input/rebinding, selector tooltip, layout, localization, camera integration,
feature presentation, semantic snapshot and placement/config harnesses, plus
source audits for test registration and localization/resource contracts. The
current harness inventory is 45 files. The historical audit reports in this
candidate have earlier inventory figures (43 and 44) and key counts of 162;
the current localization audit output recorded during the preceding build
reports 18 catalogs and 182 keys.

New diagnostics include bounded language-state observation tools and optional
runtime performance telemetry. Diagnostic outputs and research watchers are
separate from the supported production ASI profile.

## Documentation, research and repository content

The README, architecture/safety documents, supported-build manifest,
testing/research overview, third-party notices, backlog index and license are
updated. Six visual media files are added for product documentation. The
current task log is rolled over into backlog/TASKLOG_PREVIOUS.md; task plans
and reports are reorganized into active, completed and deferred research
records. New reports cover the v2 architecture/maintainability audit,
architecture repair closure, performance audit, overlay feasibility and
implementation evidence. Research scripts and UE4SS language probes are
archived under research/.

The license adds a source-attribution condition when covered code,
documentation or research materially contributes to a public software product,
including where used through AI-assisted development. It states that
independent discovery or similarity alone does not trigger the condition and
that reference use alone does not make an implementation a derivative release
or require release permission. The scope still names official v1.0.0-or-later
material; releases before v1.0.0 retain their MIT terms.

## Evidence recorded with the candidate

The staged reports distinguish source/harness evidence from runtime evidence.
They record a 2.0.6 combined runtime session, but its mod hash (19F2…) differs
from both the documented release-candidate hash (D04A…) and the locally built
production hash (1AFF…). After those report snapshots, the project owner
confirmed that the current mod works in-game and that nearly all behavior has
been checked. The hash and complete scenario matrix for that later review were
not supplied here, so the report records the confirmation without assigning
it to one of the older hashes.

The README's opening description says the current implementation is
runtime-tested on Steam 2.0.6. The supported-build manifest and closure report
give the narrower binary identity and scenario boundaries described above.
These statements are both present in the candidate documentation.

The audit report snapshots initially recorded 43 harnesses / 162 keys at
architecture closure, then 44 harnesses / 162 keys at performance-repair
follow-up. The latest audit-only refresh reports a 45-entry runner set and
18 catalogs / 182 keys. The performance report now explicitly marks its
original PERF-01/02 findings as pre-repair and the later repair section as the
current disposition: both are repaired and offline-validated; PERF-C01
measurement remains pending. Historical counts are labeled by snapshot rather
than presented as competing current totals.

The repository intentionally carries a broad v1-to-v2 change set; its size is
part of the requested comparison, not a proposed scope reduction. The prior
diff check also reported trailing whitespace in some added files. This report
does not alter that content or treat the intentional breadth of the change set
as a defect. Production and harness build results are historical evidence
recorded elsewhere; this report did not rerun builds or launch the game.

## Changed path inventory

The 271 paths are grouped broadly as follows:

- src/: 71 paths across plugin/runtime, overlay, config, diagnostics,
  camera, gameplay, cinematics and Win32 viewport support.
- tests/: 23 paths across configuration, runtime API, camera/feature
  transitions, overlay and source audits.
- research/: 99 paths containing plans, reports, diagnostic tools, probes
  and completed/deferred task records.
- locales/: 19 paths for 18 catalogs and their catalog contract/readme.
- assets/: 15 paths for font binaries, subsets, licenses and glyph contract.
- external/imgui/: 15 vendored Dear ImGui core/backend/license paths.
- screens/: 6 added images/GIFs.
- Remaining paths: build/test entry points, architecture/safety/support docs,
  README, license, third-party notices and backlog records.

This report is an untracked analysis artifact and is not part of the staged
v2.0 candidate it describes.
