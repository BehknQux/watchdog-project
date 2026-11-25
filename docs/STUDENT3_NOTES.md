# Student 3 — Integration and Test Notes

## Logging contract

The main program calls:

- `logger_init(LOG_FILE)` (0 on success, -1 on error)
- `log_event("message %d", value)` (timestamp + append + fflush)
- `log_status_report()` (called from the main loop following `SIGUSR1`)
- `logger_close()`

The status reporter calls the existing process-manager interface:

- `pm_worker_count()`
- `pm_get_worker(index, &pid, &restarts, &state)`


## Integration Status (Verified on 2026-11-25)

Student 2 successfully implemented pm_worker_count() and
pm_get_worker() in src/process_manager.c.

All three students' contributions have been merged into
the main branch of the GitHub repository.

Verification results:
- make clean: PASSED
- make: PASSED (no compiler warnings)
- make test-logger: PASSED
- make test: PASSED (8/8 integration checks)
- Manual Ctrl+C shutdown test: PASSED
- Process cleanup verification: PASSED

Both worker processes terminated successfully, and the
watchdog completed its shutdown procedure.

## Test procedure

1. `make test-logger` tests the logger using a separate fixture. It does not depend on Student 2's actual process manager.
2. `make` builds the full watchdog using all integrated student modules.
3. `make test` runs integration scenarios against isolated copies of the real executable, plus a tiny normal-exit fixture in one scenario.
4. Inspect `tests/results/last_run/*.log` and capture test outcomes for the final report.

The real `worker.c` runs indefinitely when launched without arguments, so the normal termination test builds a temporary replacement worker in a disposable directory. This tests the actual watchdog's handling of `exit(0)` without changing any of Student 2's files.

Manual terminal test is still recommended: press actual Ctrl+C while `./watchdog` is in the foreground. The automated test sends `SIGINT` to the watchdog PID, which is the same signal but not an interactive keyboard test.

## Example timeline

```
[2026-11-25 14:03:21] Watchdog started (pid 1230)
[2026-11-25 14:03:21] Worker 1 started (pid 1234, restarts 0)
[2026-11-25 14:03:21] STATUS REPORT: 2 worker(s)
[2026-11-25 14:03:21] Worker 1: pid=1234 state=RUNNING restarts=0
```

## Report checklist

- Screenshot of successful `make` with no compiler warnings
- Screenshot/output of `make test` including pass/fail counts
- Sample `watchdog.log` records with timestamps
- Architecture and group member responsibility table
- Public repository URL and complete `git log` output
- Explanation of restart bound, signal-safety and graceful shutdown
