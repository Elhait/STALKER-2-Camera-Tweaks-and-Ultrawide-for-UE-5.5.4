# Task Log Rollover — Task Plan

## Objective

Preserve the current `backlog/TASKLOG.md` byte-for-byte as a clearly named previous log, create a fresh active `backlog/TASKLOG.md` for future entries, and update backlog navigation.

## Established evidence and current state

- User explicitly requested a new active task log and a rename of the current file to identify it as the previous version.
- `backlog/TASKLOG.md` is approximately 369 KB and already contains working-tree changes. Previous patch attempts to append failed.
- `backlog/README.md` points to `TASKLOG.md`; historical plans also reference `TASKLOG.md`, which will remain the active path after rollover.

## Approved scope

- Move the exact existing `backlog/TASKLOG.md` to `backlog/TASKLOG_PREVIOUS.md` without changing contents.
- Create a new concise `backlog/TASKLOG.md` with a rollover entry and a link to the previous log.
- Update `backlog/README.md` to distinguish the current and previous logs.

## Explicit non-goals

- Do not edit the archived log contents or rewrite historical plans/reports.
- Do not modify Git index/history.
- No source/build/runtime work under this plan.

## Expected files

- `backlog/TASKLOG.md` (new active file)
- `backlog/TASKLOG_PREVIOUS.md` (preserved existing contents)
- `backlog/README.md`
- `TASKLOG_ROLLOVER_TASK_PLAN.md` (archive after completion)

## Batch and validation

1. Verify destination does not exist; move the existing log to the exact archive name and create a new active log.
2. Update backlog navigation; compare the archived file length and SHA-256 against the pre-move values; verify both links/paths and inspect Git status.

## Risks and safe failure

- The old log is user history and already dirty. Preserve it by exact move; never overwrite an existing archive target.
- If move, new file creation, or hash comparison fails, stop and report without deleting/replacing any file.

## Stop conditions and phase gates

- Stop if `TASKLOG_PREVIOUS.md` already exists or the moved archive hash differs from the original hash.
- No unrelated backlog files or historical references should be mass-edited.

## Final Git review

- Read-only `git status`, exact-path review, archive hash/size, README diff, and recent commits. No Git add/commit/rename command.
