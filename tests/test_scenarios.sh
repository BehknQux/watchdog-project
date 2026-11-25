#!/usr/bin/env bash
# Isolated POSIX-watchdog integration checks. Only processes launched by this
# script are signaled; the developer's build and files are never overwritten.
set -Eeuo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TEMP="$(mktemp -d)"
OUT="$ROOT/tests/results/last_run"
mkdir -p "$OUT"
WD_PID=""
CURRENT=""
PASS=0

fail() { echo "FAIL: $*" >&2; exit 1; }
pass() { echo "PASS: $*"; PASS=$((PASS + 1)); }

shutdown_watchdog() {
    if [[ -n "$WD_PID" ]] && kill -0 "$WD_PID" 2>/dev/null; then
        kill -TERM "$WD_PID" 2>/dev/null || true
        for _ in {1..50}; do
            kill -0 "$WD_PID" 2>/dev/null || break
            sleep 0.1
        done
        if kill -0 "$WD_PID" 2>/dev/null; then
            # This is only the watchdog process launched by the test.
            kill -KILL "$WD_PID" 2>/dev/null || true
        fi
    fi
    if [[ -n "$WD_PID" ]]; then
        wait "$WD_PID" 2>/dev/null || true
    fi
    WD_PID=""
}
cleanup() {
    shutdown_watchdog
    rm -rf "$TEMP"
}
trap cleanup EXIT

wait_for() {
    local path="$1" pattern="$2" tries="${3:-80}"
    for ((i=0; i<tries; ++i)); do
        if [[ -f "$path" ]] && grep -Eq "$pattern" "$path"; then return 0; fi
        sleep 0.1
    done
    return 1
}

start_case() {
    local name="$1"
    shutdown_watchdog
    CURRENT="$TEMP/$name"
    mkdir -p "$CURRENT/workers"
    cp "$ROOT/watchdog" "$CURRENT/watchdog"
    cp "$ROOT/workers/worker" "$CURRENT/workers/worker"
}

run_watchdog() {
    (cd "$CURRENT" && exec ./watchdog > stdout.txt 2>&1) &
    WD_PID=$!
    wait_for "$CURRENT/watchdog.log" 'Worker 1 started' || fail "watchdog worker 1 never started"
    wait_for "$CURRENT/watchdog.log" 'Worker 2 started' || fail "watchdog worker 2 never started"
}

get_worker_pid() {
    local n="$1"
    sed -nE "s/.*Worker ${n} started \\(pid ([0-9]+),.*/\\1/p" "$CURRENT/watchdog.log" | tail -1
}

save_case() {
    local name="$1"
    cp "$CURRENT/watchdog.log" "$OUT/${name}.log" || true
    cp "$CURRENT/stdout.txt" "$OUT/${name}.stdout.txt" || true
}

# 1. Normal operation and 5. SIGUSR1 status report.
start_case normal_and_status
run_watchdog
kill -0 "$(get_worker_pid 1)" || fail 'worker 1 not alive'
kill -0 "$(get_worker_pid 2)" || fail 'worker 2 not alive'
pass 'normal operation: at least two running workers'
kill -USR1 "$WD_PID"
wait_for "$CURRENT/watchdog.log" 'STATUS REPORT' || fail 'SIGUSR1 status report missing'
wait_for "$CURRENT/watchdog.log" 'Worker 1: pid=[0-9]+' || fail 'worker 1 status missing'
wait_for "$CURRENT/watchdog.log" 'Worker 2: pid=[0-9]+' || fail 'worker 2 status missing'
pass 'SIGUSR1 status report'
save_case normal_and_status
shutdown_watchdog

# 2. Normal termination: isolated worker fixture exits with code 0. This
# verifies watchdog's waitpid behavior without changing the real worker.c.
start_case normal_exit
# The isolated fixture exits successfully after 0.2 seconds.
cat > "$CURRENT/worker_exit_zero.c" <<'C_EOF'
#define _POSIX_C_SOURCE 200809L
#include <time.h>
int main(void) { struct timespec ts = {0, 200000000L}; nanosleep(&ts, 0); return 0; }
C_EOF
gcc -std=c11 -Wall -Wextra -pedantic "$CURRENT/worker_exit_zero.c" -o "$CURRENT/workers/worker"
run_watchdog
wait_for "$CURRENT/watchdog.log" 'No running children left' || fail 'normal termination not observed'
[[ "$(grep -c 'exited with code 0' "$CURRENT/watchdog.log")" -ge 2 ]] || fail 'both normal exits missing'
! grep -q 'Restarting worker' "$CURRENT/watchdog.log" || fail 'normal exit restarted unexpectedly'
pass 'normal worker termination without restart'
save_case normal_exit
shutdown_watchdog

# 3. SIGKILL unexpected termination and restart.
start_case sigkill_restart
run_watchdog
old_pid="$(get_worker_pid 1)"
[[ "$old_pid" =~ ^[0-9]+$ ]] || fail 'invalid worker pid'
kill -KILL "$old_pid"
wait_for "$CURRENT/watchdog.log" 'Restarting worker 1 \(attempt 1/3\)' || fail 'restart event missing'
[[ "$(grep -c 'Worker 1 started' "$CURRENT/watchdog.log")" -ge 2 ]] || fail 'replacement worker did not start'
new_pid="$(get_worker_pid 1)"
[[ "$new_pid" != "$old_pid" ]] || fail 'worker PID was not replaced'
pass 'SIGKILL triggers unexpected termination restart'
save_case sigkill_restart
shutdown_watchdog

# 4. Kill the same worker four times: only 3 restarts allowed.
start_case restart_limit
run_watchdog
for attempt in 1 2 3 4; do
    victim="$(get_worker_pid 1)"
    [[ "$victim" =~ ^[0-9]+$ ]] || fail 'invalid restart-limit victim PID'
    kill -KILL "$victim"
    if ((attempt <= 3)); then
        wait_for "$CURRENT/watchdog.log" "Restarting worker 1 \\(attempt $attempt/3\\)" || fail "missing restart $attempt"
        for _ in {1..50}; do
            [[ "$(grep -c 'Worker 1 started' "$CURRENT/watchdog.log")" -ge "$((attempt+1))" ]] && break
            sleep 0.1
        done
    else
        wait_for "$CURRENT/watchdog.log" 'Worker 1 reached restart limit' || fail 'restart limit not recorded'
    fi
done
[[ "$(grep -c 'Worker 1 started' "$CURRENT/watchdog.log")" -eq 4 ]] || fail 'restart count exceeded three'
pass 'three restarts maximum'
save_case restart_limit
shutdown_watchdog

# 6. SIGTERM graceful shutdown.
start_case sigterm_shutdown
run_watchdog
kill -TERM "$WD_PID"
wait_for "$CURRENT/watchdog.log" 'All children reaped, watchdog exiting' || fail 'SIGTERM shutdown did not complete'
pass 'SIGTERM graceful shutdown'
save_case sigterm_shutdown
shutdown_watchdog

# 7. Ctrl+C behavior: deliver SIGINT to watchdog, equivalent signal.
start_case sigint_shutdown
run_watchdog
kill -INT "$WD_PID"
wait_for "$CURRENT/watchdog.log" 'All children reaped, watchdog exiting' || fail 'SIGINT shutdown did not complete'
pass 'SIGINT graceful shutdown (Ctrl+C equivalent)'
save_case sigint_shutdown
shutdown_watchdog

# 8. Log validation with timestamp and key event names.
[[ -f "$OUT/normal_and_status.log" ]] || fail 'log files missing'
grep -Eq '^\[[0-9]{4}-[0-9]{2}-[0-9]{2} [0-9]{2}:[0-9]{2}:[0-9]{2}\]' "$OUT/normal_and_status.log" || fail 'timestamp missing'
grep -q 'Watchdog started' "$OUT/normal_and_status.log" || fail 'start event missing'
grep -q 'Restarting worker' "$OUT/sigkill_restart.log" || fail 'restart log missing'
grep -q 'All children reaped' "$OUT/sigterm_shutdown.log" || fail 'shutdown event missing'
pass 'timestamped event logging'

echo "$PASS checks passed. Logs are saved in tests/results/last_run/"
