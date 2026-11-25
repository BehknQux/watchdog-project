# POSIX Watchdog — Process Monitoring and Management

System Programming Term Project 1, Sakarya University (2026–2027).

A C/POSIX supervisor starts two worker processes, reaps terminated children, restarts unexpected failures at most three times per worker, logs status on `SIGUSR1`, and shuts down cleanly on `SIGINT` or `SIGTERM`.

## Requirements

- Linux (Ubuntu or WSL Ubuntu)
- GCC, GNU Make, Bash, Git

On Ubuntu:

```bash
sudo apt update
sudo apt install build-essential git
```

## Build

```bash
make
```

This creates `./watchdog` and `./workers/worker`. The `watchdog` executable expects to be launched from the repository root because the worker path is relative.

## Run

Terminal A:

```bash
./watchdog
```

It prints `watchdog pid: <PID>` and appends events to `watchdog.log`.

Terminal B (replace `<PID>` with the printed watchdog PID):

```bash
kill -USR1 <PID>    # Append a worker status report to watchdog.log
cat watchdog.log    # Read event and status records
kill -TERM <PID>    # Graceful shutdown
```

For a worker restart test, use a **verified worker PID** from `watchdog.log` (never the watchdog PID):

```bash
kill -KILL <WORKER_PID>
```

You can also press Ctrl+C in Terminal A for graceful shutdown.

## Tests

```bash
make test-logger  # Independent logger smoke test with mocked worker status
make test         # Full integration test: runs multiple watchdog instances
make clean        # Remove compiled executables
```

The integration script creates isolated temporary working directories, so it does not replace the project's real worker. Logs from its most recent run are saved in `tests/results/last_run/`.

Scenarios checked: normal startup, normal worker exit, unexpected kill with restart, three-restart limit, `SIGUSR1` status, `SIGTERM` shutdown, `SIGINT` shutdown, and timestamped log entries. A temporary test-only worker exits normally for the normal-exit scenario.

## Architecture

- `src/main.c`: main watchdog loop and shutdown (Student 1)
- `src/signal_manager.c`: async-signal-safe signal flags (Student 1)
- `src/process_manager.c`: worker creation, monitoring and restarts (Student 2)
- `src/logger.c`: append-only timestamped logs and `SIGUSR1` status reporting (Student 3)
- `workers/worker.c`: example child executable (Student 2)
- `tests/test_scenarios.sh`, `Makefile`, `README.md`: tests, build and documentation (Student 3)

**Signal-safety:** The signal handler only sets `sig_atomic_t` flags. Logging and worker state reporting are performed by the main flow, not by the handler.

## Collaboration

All contributors use their own GitHub accounts and descriptive commit messages. Include the public repository URL and full Git commit history in the report submitted to SABIS.

For project-specific testing limitations and integration notes, see `docs/STUDENT3_NOTES.md`.
