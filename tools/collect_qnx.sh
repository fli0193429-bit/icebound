#!/bin/sh
# Run on the real QNX target after deploying the actual qcc-built binary.
set -eu
if [ "$(uname -s)" != "QNX" ]; then
    echo 'Refusing: this collector requires an actual QNX target.' >&2
    exit 2
fi
binary=${1:-./icebound-qnx}
out=${2:-qnx-evidence}
# Never overwrite a prior evidence run.
mkdir "$out"
uname -a > "$out/uname.txt"
date -u > "$out/collected-at.txt"
failed=0
for scenario in nominal link-loss nav-stale low-energy obstacle deadline current; do
    if "$binary" "$scenario" > "$out/$scenario.csv" 2> "$out/$scenario.log"; then
        echo "$scenario simulated PASS" >> "$out/status.txt"
    else
        code=$?; echo "$scenario simulated FAIL exit=$code" >> "$out/status.txt"; failed=1
    fi
done
for scenario in nominal link-loss nav-stale; do
    if "$binary" "$scenario" --realtime > "$out/$scenario-realtime.csv" 2> "$out/$scenario-realtime.log"; then
        echo "$scenario realtime PASS" >> "$out/status.txt"
    else
        code=$?; echo "$scenario realtime FAIL exit=$code" >> "$out/status.txt"; failed=1
    fi
done
echo "Evidence recorded in $out; inspect every log before claiming validation."
exit "$failed"
