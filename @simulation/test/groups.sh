#!/usr/bin/env bash
# Runs the simulation checks as one process per group of test/Main.cpp, side by side, and fails if
# any group fails. Each group still stops at its first failing check; the others run on.
#   test/groups.sh [build directory, relative to @simulation] [-word...]
#   test/groups.sh ../.out/simulation/build -glaciated     # leave out testReliefGlaciatedSite
# Every argument goes to every group; a filter word a group holds no test for leaves that group
# empty and passing. To run some tests by name, run simulation-tests itself.
set -o pipefail
cd "$(dirname "$0")/.."
build=../.out/simulation/build
if [[ $# -gt 0 && -d $1 ]]; then build=$1; shift; fi
binary=$(find "$build" -path '*/release/simulation-tests' -type f | head -1)
[[ -x $binary ]] || { echo "no simulation-tests under $build; build it first" >&2; exit 2; }

groups=($("$binary" --groups))
[[ ${#groups[@]} -gt 0 ]] || { echo "$binary lists no test groups" >&2; exit 2; }
pids=()
# Background jobs ignore an interrupt, so stop the groups' processes by hand.
trap 'for pid in "${pids[@]}"; do pkill -TERM -P "$pid"; kill "$pid"; done 2>/dev/null; exit 130' INT TERM
start=$(date +%s)
for group in "${groups[@]}"; do
    (
        begin=$(date +%s)
        "$binary" --group "$group" "$@" 2>&1 | sed -u "s/^/[$group] /"
        status=$?
        echo "[$group] exit $status after $(( $(date +%s) - begin )) s"
        exit $status
    ) &
    pids+=($!)
done
failed=0
for i in "${!groups[@]}"; do
    wait "${pids[$i]}" || failed=1
done
echo "simulation-tests: ${#groups[@]} groups in $(( $(date +%s) - start )) s, $([[ $failed == 0 ]] && echo passed || echo FAILED)"
exit $failed
