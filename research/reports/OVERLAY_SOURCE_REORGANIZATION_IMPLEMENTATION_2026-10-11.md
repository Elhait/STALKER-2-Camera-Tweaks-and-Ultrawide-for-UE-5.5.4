# Overlay Source Reorganization — Implementation Report

**Date:** 2026-10-11  
**Scope:** Approved directory-only reorganization of `src/overlay/`, Batches 1–6.  
**Behavior/API changes:** None intended or made.

## Result

The 51-file Overlay inventory now matches the approved responsibility layout. Fifty files moved; `optional_overlay_boundary.hpp` remains at the common root. Translation units and filenames were preserved. Changes outside moves are limited to include paths and build, test, resource, and source-audit references.

```text
src/overlay/
  composition/    composition_runtime.cpp/.hpp
  input/          input_state.cpp/.hpp
  localization/   16 catalog, font, resource, validation, key, manager,
                  formatter and game-language files
  presentation/   20 projection, layout, selector, tooltip and placement files
  presenter/      10 renderer, backend, presenter-state and policy files
  optional_overlay_boundary.hpp
```

## Batch ledger

| Batch | Files moved | Result |
| --- | ---: | --- |
| 1 — semantic presentation | 7 | Complete |
| 2 — layout/selector/tooltip support | 13 | Complete |
| 3 — localization/resources | 16 | Complete |
| 4 — input | 2 | Complete |
| 5 — presenter/rendering owner | 10 | Complete |
| 6 — composition root | 2 | Complete; moved last |

Total moved: 50. Root-level shared boundary retained: 1. Total inventory: 51.

## Updated references

- Production source/resource inputs: `build.cmd`.
- Test resource input and moved harness source inputs: `test.cmd`.
- Overlay harness includes: camera integration, feature presentation, layout metrics, placement, selector tooltip, localization, input, composition presenter state, and camera writer transaction.
- Localization/source contract audit: `tests/runner/localization_catalog_audit.ps1`.
- Native runtime evidence resource compiler input: `tests/runner/native_runtime_evidence.ps1`.
- N06 validation resource inputs: `research/evidence/native-runtime/N06/tools/n06_validate.ps1` and `n06_m01_validate.ps1`.
- Locale resource-ID include: `src/localization/locale_registry.hpp`.
- Plugin composition facade include: `src/plugin/runtime.cpp`.
- Internal includes within moved source/header files were adjusted only for their new relative locations.

The active build, test, runner, and tool scripts were scanned for the old root-level source/resource paths; no stale references remain there. Historical research records were deliberately left as contemporaneous snapshots.

## Validation

| Check | Result |
| --- | --- |
| Localization/resource source audit | PASS — 18 locales, 212 keys, resource embedding and localization contracts pass |
| Production `build.cmd` | PASS — exit code 0 on final tree |
| Diagnostic `tools/build/build-diagnostic.cmd` | PASS — exit code 0 on final tree |
| Camera Integration harness | PASS |
| Camera writer transaction harness | PASS |
| Feature presentation harness | PASS |
| Layout metrics harness | PASS |
| Input state harness | PASS |
| Selector tooltip state harness | PASS |
| Composition presenter lifecycle harness | PASS |
| Localization harness | PASS — process exit code 0 |
| Placement config harness | FAIL — build succeeded, assertions reported save/load/migration failures and 24/24 lost concurrent setting/X/Y updates. No persistence behavior was changed in this task; cause remains undiagnosed. |
| Full `test.cmd` | FAIL — exit code 2. The runner audit reports 59 registered harnesses and one pre-existing unregistered `n06_m01_zoom_game_derived_harness`; the run also reports compiler diagnostics including C++17 language-feature errors. No unrelated harness registration or test behavior was changed here. |
| `git diff --check` / `git diff --find-renames` | Not available in this environment: Git reports “this operation must be run in a work tree” for diff operations, although `git status` and `git rev-parse --is-inside-work-tree` work. No staging was performed to force rename detection. |

The failed test gates are reported as-is; this report does not attribute them to the directory moves. The completed production and diagnostic builds verify the final source paths compile and link.

## Working tree preservation

The initial status and SHA-256 hashes for all 51 source files were captured before moving. Existing staged, unstaged, and untracked work was preserved; no reset, clean, checkout, or staging operation was performed. The initial staged state for already-modified files remains separate from the new unstaged path/include edits. New destination files appear as untracked and their old paths as deleted until the user stages them; Git rename detection was not forced because that would alter the index.

## Final source SHA-256 manifest

These hashes identify the final files after approved path-only include adjustments. They are useful for the next review/build; they are not a substitute for the pre-move snapshot captured at task start.

```text
75071D3C1A08DDD831C7C788EF206A95EE18A4436F3459372B81387D10E037B5  src/overlay/composition/composition_runtime.cpp
AC81562022EDC5BE825B3163F90C0A1DB432BEB868913501466986866082D79D  src/overlay/composition/composition_runtime.hpp
FAECEBCC5D0E89344FC893B20A13B350C0B695B88C1D2392C54E9FBE690E3AA4  src/overlay/input/input_state.cpp
08E51D66880B161511BD6288EDFC6E554A6BFCB7F9FE85167C6971942C298476  src/overlay/input/input_state.hpp
B6A6EE515C774E54E42CF457B8FA323816BFFD81695F1881E8981A0777FE7476  src/overlay/localization/game_language_reader.cpp
8F85E8E1A488D245271A9AC4241C7FB9321A3A666A437750432CD35F0ED4B395  src/overlay/localization/game_language_reader.hpp
6CAA582242A1EB3E0B727D2BE2D767380058E1D13653314AAB164CA2C08066E5  src/overlay/localization/localization_catalog.cpp
9AB66AB63C5F95D204FDB72B3268E86A807317578089304C4B8035A9CA909E56  src/overlay/localization/localization_catalog.hpp
C8B2777088FA949B5C624EC334F61E6B1F0327A04C79793EB7F89F3A4406AA01  src/overlay/localization/localization_font.cpp
68B884BAA9AAA077707559D9C576F09B7F7380647BC09FDEB9C7D21BA06CEC1F  src/overlay/localization/localization_font.hpp
E3795B747D62298AF04E760C78AA40FF4E61EEB0A309A009EB014346A6C4B484  src/overlay/localization/localization_formatter.cpp
7C65653198AE15965C0D2739B7E353DB75158D1ECA0EBE029F713C20CE23BB6F  src/overlay/localization/localization_formatter.hpp
554D807589CFC3ED5AE5A03178C353419F10964B79469414CA53AD41AB450FCE  src/overlay/localization/localization_keys.cpp
6270E8B4E224DD364E0A1B774C151155D9FCAFD850B9EC4EEF3280371BAF6E13  src/overlay/localization/localization_keys.hpp
DCBB3F1886C9BADC6A331128F5F6C7FF126285DA41F5B90FB61BC221DC058C89  src/overlay/localization/localization_manager.cpp
84537C77D7D140823008FE04F7BE18C2F0737FB8CD791A8D92710B7A7794F070  src/overlay/localization/localization_manager.hpp
EC860DF4C3EF0009F17F7493AA9B2A9368CFF929F872F25D49AE74BA84728213  src/overlay/localization/localization_resource_ids.h
AF6DD3A42ADAC62556F950FEBA921A4B006EBE0ADD8C4328573B8E47F2284449  src/overlay/localization/localization_resources.rc
56BA677D237E5799CD2170539748E78CA60B025BB45B44F2605F600CFCD18AEA  src/overlay/localization/localization_validator.cpp
DCD6FEBFC32E7878806A9AB78781E4B5B40862B4CDF0A5AB2BB308CCD6EABAE0  src/overlay/localization/localization_validator.hpp
D5D53703D71F8BCF5E8B6B1730A511B7A66782B05BFA825EC46BE57D1A61AAD2  src/overlay/optional_overlay_boundary.hpp
EC1A0CB4D57C1AD8CDF6C60ACA6C0AE97029811DA83957EC6A714AB9D0FCECF5  src/overlay/presentation/aspect_ratio_format.hpp
701A11EEC03CE16C7C0DB2D7EB005F41731241400467775A3E925D5E8FF1B39C  src/overlay/presentation/camera_integration.cpp
D9D27D7D35F0291498578CC8E866E9B08F36003AD933AF1617575521C30C1CA9  src/overlay/presentation/camera_integration.hpp
9355D7C50DF151F1CA3C05A2BCDC622B40CB40AC033B7447B987062E19449401  src/overlay/presentation/camera_state_view.cpp
73C35EC43F51A0430E20881BBD0D27D402E87B7C5EC86BB2DA2C7C55E67042B9  src/overlay/presentation/camera_state_view.hpp
8D861EED97BDF239249BF255EF7AAAFBC8472001D9DE2856786DE6075D6F8CD6  src/overlay/presentation/feature_presentation.cpp
A4B3834A0EA210A3151998C157D7ACC3B7244323C082E98D527864B019B6E4E4  src/overlay/presentation/feature_presentation.hpp
02C6B8B62EF9F318F0B093084B77C46962385321DB7989BD0AC3382F6257CFD0  src/overlay/presentation/hotkey_binding_presentation.hpp
7C610D51A40BCE9AFE934633CD4E59EC770718F7A80A327BD26A4911048DAB9B  src/overlay/presentation/inline_hotkey_layout.hpp
3655B1661EE0D47CC80FC2E7B5E46B6BDCC2828BE2C8685871B3C1C8B5FAADB3  src/overlay/presentation/overlay_layout_metrics.cpp
C5A05545C66FC3420AF601E02D531D7850470C11B463318C30E8AECB5978E9AB  src/overlay/presentation/overlay_layout_metrics.hpp
D9D9092DFE6B9F0C27AC16D7F040AAC645A269F1646C3A0BEA71BDA8A92FDB4B  src/overlay/presentation/placement_config.cpp
15011CA8567943B26CB1F6BFA0A8A27BF476D9D9AFDDF4F4263A29B5B85FC6BC  src/overlay/presentation/placement_config.hpp
33300F1AB3F15774C45D33B9FF2854AF354817F713F87C70701429405E33E1E8  src/overlay/presentation/presentation_contracts.hpp
C81F51E40CBB24C35553477BD689FB2C158F60B0946FD983D17D1372A8F433F3  src/overlay/presentation/selector_documentation_view.cpp
1E6B886C39E228F8F3A59D8FE0DD21EC752C48CEE197BF8597955909A0644C0C  src/overlay/presentation/selector_documentation_view.hpp
9F739F28C3B04F17854283CD623513FAF9E1716265CBBB541EF7FA383F43E485  src/overlay/presentation/selector_examples_geometry.hpp
5556CAAC557A45DC7A40757D9CE981BA0C422AC1D34B66EDC3007B7FF6F2A234  src/overlay/presentation/selector_tooltip_state.hpp
BD214A2803C60C0761786ACBF9FE43E672765F37309DA891670020D87FDD58E2  src/overlay/presentation/setting_tooltip_content.hpp
ABC3C31BE077C1DD8E17A1E7D48BC4158F3B60A0D78852910018A530BD32D382  src/overlay/presentation/tooltip_row_layout.hpp
BED5229B3FEA47A95036A34DD107E8A8C8A6324378D241E092CD8B41A6691501  src/overlay/presenter/composition_presenter_state.cpp
7144EE4D65FAC5394973B96E5F994DF53AE2354D9E6AA43279ABA82163359CC0  src/overlay/presenter/composition_presenter_state.hpp
3A42512DD7BE8FAE4BE1FAF4251DBA57FA46ECA7B6CDFD89928EEC59BC534945  src/overlay/presenter/imgui_d3d11_renderer.cpp
F86A85A8A49F44AD11CABC54DAC79FDAB69B0D438D4926DBEAC8CE15615B7E51  src/overlay/presenter/imgui_d3d11_renderer.hpp
BF29D94C699DD7896B644F1664422E1DD9050DECA894121D9DA00AE1E2D60F06  src/overlay/presenter/placement_save_state.hpp
9A02E58AFB27A805C1E844E814DB6D3C0BE60DA2FE5C7BC99885001EA4018E2C  src/overlay/presenter/presenter_runtime_policy.hpp
5DC287300D1AF95846D439B36A3E77A7D06084307868593260DB9D95B37C9ED1  src/overlay/presenter/renderer_runtime.cpp
E4AB254C4D7A0F6AA40FB46FD6DE8803C54C59CD4263BAC2C07F274EF6D34E30  src/overlay/presenter/renderer_runtime.hpp
E9E30A8299594E57AF7B15C9A53AD0BC53384827439898DE1DB9BDA7A708AB9E  src/overlay/presenter/renderer_state.cpp
3D0EF8002D44CC2EEE2B9AD859172C88ECFA7D264640EC1254C612B4D0F1D5CC  src/overlay/presenter/renderer_state.hpp
```

## Deviations and scope notes

- No deviation from the approved move matrix.
- The working tree had extensive staged, unstaged, and untracked changes before this task. They were retained; only Overlay paths and their required mechanical references were changed.
- No game was launched and no ASI was installed.
- The test failures above remain unresolved; fixing their registration, compiler setup, or persistence assertions would exceed this source-organization task.
