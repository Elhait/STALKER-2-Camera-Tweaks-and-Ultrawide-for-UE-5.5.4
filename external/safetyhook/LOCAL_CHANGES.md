# Camera Tweaks local integration changes

The bundled amalgamation remains vendor code. Preserve the existing retained
executable-allocation/fault-handling integration when refreshing it.

## Process-resident state (2026-10-17)

Camera Tweaks stays loaded until process termination and pins its module from
its existing initialization worker. The allocator's shared weak-reference state
and the TrapManager/VEH/map now remain process-resident. They must not register
destructors that traverse interrupted installation state under loader lock.
Normal per-hook reset and trap removal still use their existing synchronization;
no executable-patching algorithm is replaced by this change.

A retained VEH must also avoid inspecting interrupted state during process
termination. The Windows 10+ RtlDllShutdownInProgress export is resolved before
taking the trap mutex and cached in a lock-free atomic function pointer. Its
true result makes the existing VEH return CONTINUE_SEARCH before locking or
reading the trap map. If the required export is unavailable, trap installation
fails before writing the patch. There is no new VEH, shutdown hook or thread.

The SAFETYHOOK_TEST_VIRTUAL_PROTECT build exposes a focused test that invokes
the actual handler in shutdown state while its mutex is already held. This
test-only entry is absent from the production ASI. The hook/reset/native-
continuation matrix continues to exercise ordinary behavior.

See `research/reports/FINAL_ASI_SHUTDOWN_LIFECYCLE_CLOSURE_2026-10-17.md` in the
repository root for the bounded evidence and process-termination limitations.
