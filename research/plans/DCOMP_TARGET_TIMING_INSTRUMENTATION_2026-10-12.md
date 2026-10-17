# Target lifecycle timing instrumentation

Approved scope: isolated research artifact only. No game launch or installation.
Use the target-release source snapshot preparation with an optional timing patch,
never canonical production inputs. Preserve all renderer/mutex/COM ordering.

Measure QPC intervals for SetRoot, Commit, target COM Release and window-procedure
mutex acquisition. Store only >=2 ms samples in a fixed 64-slot atomic buffer.
No allocating, formatting, file I/O, additional mutex or compositor wait on capture.
Bounded overflow drops samples and reports the count. Drain on the Overlay owner
outside renderer critical sections, with operation, start/end QPC and thread ID.
Slow window-thread samples post one coalesced diagnostic-only owner message so
idle does not indefinitely retain them; this message does not request rendering.
Existing lifecycle logger behavior is unchanged, not asserted nonblocking.

Research files: timing header, checked snapshot patch script, preparation/fixture
runner extensions and fixture checks. Outputs under ignored build-artifacts/trt.
Tests: threshold boundary, bounded overflow, returned result/exception preservation,
real contended mutex interval, actual target-release Renderer regression. Build
isolated ASI and verify canonical source parity. No full-suite or game experiment
required; do not treat local timings as a measurement of the user's freeze.

Review whether lock scope can be reduced from actual ownership dependencies. Do
not implement unlocking, new resource ownership, extra waits or delayed teardown.
Deliver measurements capability, offline validation and a bounded design finding;
actual game blocking remains unmeasured without a future authorized capture.

Isolated timing build and local fixtures passed with exit 0. Production inputs
are unchanged; no source synchronization was modified. The experiment report
records threshold/buffer tests, a controlled mutex wait and real HWND regression.
Instrumentation is prepared, not proof of the previously observed game stall.

Latest history review: timing artifact 011E...D8FB is retained separately from
original target-release 5D2A...B55B. The prior target-release variant already had
reported instability before these wrappers were added. No new target lifecycle
change exists in the timing step. Preserve both; no rollback, deletion, promotion
or new gameplay experiment is requested by this review.
