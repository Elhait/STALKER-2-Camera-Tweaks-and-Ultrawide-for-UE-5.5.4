# v4 Cross-Build Resolver Audit — 2026-11-19

## Question

Do the v4 production resolver patterns and structural contracts match the retained Steam 2.0.x executable images, and the user's currently installed game image, without changing source signatures?

## Result

The v4-required startup resolver set passed the static image checks on Steam 2.0.2, 2.0.3, 2.0.4, 2.0.5 and 2.0.6. It also passed against the currently installed executable, which the user identifies as Steam 2.1.

| Image | Gameplay writer | Cinematic aspect | Cinematic ENTER/EXIT | Dialogue boundary | Provider activation | Zoom pair |
| --- | --- | --- | --- | --- | --- | --- |
| Steam 2.0.2 | PASS | 1 unique | PASS, legacy EXIT | 1 unique | 1 unique | Absent |
| Steam 2.0.3 | PASS | 1 unique | PASS, legacy EXIT | 1 unique | 1 unique | Absent |
| Steam 2.0.4 | PASS | 1 unique | PASS, legacy EXIT | 1 unique | 1 unique | Absent |
| Steam 2.0.5 | PASS | 1 unique | PASS, indexed EXIT fallback | 1 unique | 1 unique | Present |
| Steam 2.0.6 | PASS | 1 unique | PASS, indexed EXIT fallback | 1 unique | 1 unique | Present |
| Installed current image (reported as 2.1) | PASS | 1 unique | PASS, indexed EXIT fallback | 1 unique | 1 unique | Present |

For every image, the Gameplay writer matched uniquely and the expected store instruction bytes were present. Cinematic ENTER/EXIT resolved to the same consumer target with their required surrounding virtual-call pairs. The aspect signature includes the expected field/store pattern. The exact Dialogue boundary pattern matched uniquely. Provider activation matched uniquely. The optional post-process and post-setter diagnostic patterns and language reader/anchor were also present on all listed images.

The historical image scan checked identities before interpreting each image. The installed current executable was imported into an isolated temporary Ghidra project and scanned without full analysis. The scanner and raw outputs are retained alongside this report.

## Impact of the absent Zoom pair

The Zoom IN/OUT signatures are absent on 2.0.2–2.0.4. Source inspection confirms `InstallZoomTransitionObservation()` is optional: failure records the observer as unavailable and does not abort Gameplay, Cinematic or Dialogue hook installation. It removes zoom/neutral-zoom observations for Camera State on those builds; it does not disable the FOV correction paths. The pair is present on 2.0.5, 2.0.6 and the installed current image.

No production signature change is justified by this result. Adding a fallback for the older Zoom observer would require its own validated instruction and operand contract; none was established in this audit.

## Production hook address and dependency audit

The active production hook sites are resolved from executable-section signatures or from validated relative-call/control-flow relationships. The checked set includes Gameplay writer and its optional post-store/helper observations, Cinematic aspect store and ENTER/EXIT boundaries, Dialogue boundary, post-EXIT provider activation/setter-return observations, paired Zoom observers, and the language reader/free-function anchor. Hook installation requires the resolver-specific uniqueness and byte/instruction checks described in [`SUPPORTED_BUILD_MANIFEST.md`](../../../docs/SUPPORTED_BUILD_MANIFEST.md); linked cinematic call targets and the shared consumer are resolved from the matched instructions rather than fixed RVAs.

The production build defines `POST_EXIT_BLEND_OBSERVATION`, which is the signature-resolved provider activation/setter observation path. Three legacy trace implementations still contain fixed RVAs and executable-byte checks tied to one historical image, but their enabling macros (`POST_EXIT_CAMERA_FOV_WRITE_OWNER_TRACE`, `POST_EXIT_FOV_PRODUCER_STORE_TRACE`, `POST_EXIT_FOV_STATE_CONSUMER_TRACE`) are not defined by the release CMake target. If enabled in a research build, they additionally require the matching executable hash and expected bytes before creating a hook. They are not active production hook dependencies and cannot silently bind to a changed image.

The ASI is not literally free of fixed native layout assumptions: camera/provider fields, instruction operands, and virtual-call slots use fixed offsets. The audit found these represented in the matched signature or checked decoded/byte contract for the active paths on the six examined images. Some offsets are observation-only reads protected by `SafeRead`; they do not authorize writes or determine a correction when unreadable. This supports portability across the examined layouts, not a guarantee that a future patch will retain the same ABI. A future change that invalidates a required signature or contract is rejected at initialization/component install; the affected correction remains unavailable and native behavior is retained rather than installing at a guessed address.

Existing resolver harnesses include zero/one/multiple-match and malformed-contract rejection for the Gameplay writer and cinematic aspect resolver, and missing/duplicate/wrong-operand/truncated-contract rejection for Zoom and provider-observation resolvers. These are source-level test coverage findings from inspection, not a fresh test execution in this audit.

## Evidence limits

- This is static resolver and instruction-contract evidence. It establishes that the checked current signatures would resolve on these images; it is not a live ASI launch test.
- Existing runtime records on 2.0.5, 2.0.6 and the current installed image are from earlier mod artifacts. The installed game-directory ASI inspected during this audit identifies as 3.2.2.0. They must not be attributed to v4.
- The exact v4 artifact has not been launched in-game on any of these builds. This audit does not prove all engine behavior, callback scheduling or visual results.
- Executable images for 2.0.0 and 2.0.1 were not available in the retained Ghidra project, so no compatibility claim is made for them.

## Artifacts

- [`ValidateV4ResolverSet20261119.java`](../../tools/ValidateV4ResolverSet20261119.java) — exact v4 source signature set and identity-gated static scanner.
- [`ghidra-2.0.2-to-2.0.6.log`](ghidra-2.0.2-to-2.0.6.log) — results for retained historical images.
- [`ghidra-installed-current-image.log`](ghidra-installed-current-image.log) — result for the currently installed executable.

## Conclusion

No blocking core signature mismatch or active fixed-RVA production hook was found for 2.0.2–2.0.6 or the installed image reported as 2.1. Active hook locations are dynamically resolved and structurally checked; native layout offsets remain ABI assumptions validated on the examined images, so no future-patch guarantee is possible. The release compatibility notes can state static resolver coverage for those exact images, while keeping live v4 runtime support explicitly unverified. The only observed resolver gap is the optional Zoom observer on 2.0.2–2.0.4.
