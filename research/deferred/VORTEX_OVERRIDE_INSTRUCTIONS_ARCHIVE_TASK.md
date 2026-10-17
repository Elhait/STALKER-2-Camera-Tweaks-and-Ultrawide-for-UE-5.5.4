# Vortex override instructions for the mod archive

**Status:** Deferred until `RUNTIME_CPP_DECOMPOSITION` is complete and its final validation passes.
**Trigger:** The runtime decomposition plan reaches its verified final offline gate.

## Objective

Create `vortex_override_instructions.json` with the appropriate Vortex
instructions for this mod, then include that file in the distributable mod
archive.

## Scope at trigger

- Check the current Vortex-supported instruction format and the repository's
  final package layout; do not guess the JSON schema or paths in advance.
- Create and validate the JSON against that format.
- Add it to the release/archive assembly inputs and verify it is present in the
  resulting archive at the intended path.
- Keep the task limited to Vortex metadata and package contents. Do not alter
  camera/runtime behavior or broaden the runtime decomposition.

Do not start this task before the runtime decomposition is complete. Archive
assembly and publication remain separate actions requiring their own scope.
