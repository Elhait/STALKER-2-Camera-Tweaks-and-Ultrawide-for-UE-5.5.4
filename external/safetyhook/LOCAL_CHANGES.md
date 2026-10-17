# Camera Tweaks local integration changes

The bundled amalgamation remains vendor code. Preserve the existing retained
executable-allocation/fault-handling integration when refreshing it.

## Process-resident state (2026-10-17)

Camera Tweaks stays loaded until process termination and pins its module from
its existing initialization worker. The allocator's shared weak-reference state
and the TrapManager/VEH/map now remain process-resident. They must not register
destructors that traverse interrupted installation state under loader lock.
Per-hook reset remains serialized with patch transactions. Trap records are
retained as compact process-lifetime tombstones after page restoration so a
VEH dispatch delayed past RX restoration/transaction return still has its
protection epoch and allocation identity; completed records are replaced when
the same source begins a later transaction. No handler is removed under loader
lock.

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

## Executable-code publication and trap registration (2026-10-17)

New stubs and complete trampolines are instruction-cache synchronized before
publication. Target patch/restoration ranges are synchronized while execution
is still trapped, before restoring page permissions. Synchronization failure
returns an error and preserves uncertain executable ownership for recovery;
successful byte/cache restoration remains distinct from a later page-protection
restore failure. The Windows implementation uses FlushInstructionCache only
for the current process and exact changed ranges. The existing Linux branch
uses the compiler cache-clear primitive; that branch is not part of Windows
qualification.

Trap registration is checked before changing execute protection. A failed
AddVectoredExceptionHandler refuses the patch without changing page protection;
later attempts may retry registration. Test-only wrappers inject cache and VEH
failures in the actual native-hook harness. No API hiding or alternate patching
algorithm is introduced.

## Remove unreachable destructor state (2026-10-17)

TrapManager is allocated once with `new` and held by a process-lifetime raw
pointer; no code deletes it. Its destructor therefore cannot run on the current
execution paths. The destructor's `is_destructed` flag was initialized false,
never set by reachable code, and guarded the trap-registration block with an
always-true condition. Removed that dead state and branch. Handler registration,
trap-map locking, patch transactions and process-lifetime ownership are
unchanged. This clarifies the existing lifetime contract; it is not claimed to
affect antivirus classification.

## Protection, VEH and executable allocation hardening (2026-10-17)

Patch transactions snapshot each affected page's original protection and
allocation identity, mark a transition before calling VirtualProtect, inspect
the effective state afterward, and restore every affected page. A reported
failure after a successful OS change is still a failed transaction. A true
failed restore gets two bounded attempts; if a page remains unsafe, the process
fails fast with its trap record retained. For PE image pages Windows may report
copy-on-write variants of requested read/write protection. The transaction
accepts those equivalent temporary capabilities, while restoration compares
against each page's original state.

The VEH accepts only a coherent execute access violation on a page whose
allocation identity and protection history match a SafetyHook transaction.
During the transaction it admits the owned NX fault; after RX restoration it
can admit one delayed execute fault from the committed/completed transaction
and retries the unchanged instruction pointer once. A repeated same-address
fault on that thread continues search instead of looping. Completed trap records
remain as bounded-per-hook process-lifetime tombstones so delayed dispatch does
not depend on a current RX/NX query alone. Read/write faults, malformed records,
foreign allocation/page faults and unrelated execution faults continue to
other handlers. The cached Windows shutdown bypass remains before trap-map
locking.

Allocator blocks no longer share pages. Each stub/trampoline uses a private
page-sized RW reservation during construction. Inline trampolines are flushed
and sealed RX before publication; MidHook stubs are flushed again and sealed
RX after their final continuation pointer is written. VMT tables remain RW
data. Once a hook has executed, its retained continuation keeps its RX page
for process lifetime. Patch transactions may still grant temporary RWX to
code in the ASI's own image or the page containing VirtualProtect; they restore
the original protection before returning. This is not a persistent allocation
or an antivirus-specific fallback.

Allocator metadata and vector capacity are prepared before VirtualAlloc; the
returned region is attached to a unique owner immediately, before any later
throwing operation. A fault-injection checkpoint verifies that bad_alloc
releases the actual reservation (`VirtualQuery` reports MEM_FREE).

The native harness exercises genuine pre-change VirtualProtect failures,
partial rollback, strict VEH admission, two concurrent execute faults held
until after RX restoration and transaction completion, repeated-fault escape,
two simultaneous RX hooks, enable/disable, reset and recreation in both
Diagnostics modes. It also verifies allocator exception cleanup against the
OS memory state. This remains Windows x64 production integration; the Linux
branch of this amalgamation is unchanged.

## Retry generations and route-exposed ownership (2026-10-17)

The delayed execute-retry ticket now includes trap generation and page
allocation identity, so one unresolved retry cannot suppress a later
transaction at the same source VA. Trap selection prefers the active
transaction over completed same-page records, and an active route requires the
fault address to lie in that transaction's exact source range.

The protection transaction reports whether VEH exposed a continuation before
patch bytes were written. Inline trampoline and MidHook stub allocations are
retained before the NX transition, then returned to ordinary ownership only
when neither a write nor a route was published. Route-exposed continuations
remain process-lifetime through rollback and owner destruction. This avoids
freeing a trampoline or stub while a worker is already executing it.

Focused native evidence covers a persistent worker across two transaction
generations, completed-route versus active-route isolation, same-VA allocation
reuse, and a real worker held in a relocated trampoline while a pre-write
VirtualProtect failure rolls back and the owning hook is destroyed. The
post-destruction trampoline and stub are queried as committed RX before the
worker is released. Results and candidate identity are recorded in
`research/reports/SAFETYHOOK_WB01_WB02_CORRECTION_2026-10-17.md`.
