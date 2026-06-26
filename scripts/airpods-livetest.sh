#!/usr/bin/env bash
# Live test harness for auto-connect-on-wear.
#
# Replaces the installed linuxpods-daemon with a freshly built one (--debug),
# then restores it on exit (Ctrl-C). Wear/unwear is manual — follow the printed
# checklist and watch the streamed log.
#
# Usage:  AIRPODS_MAC=AA:BB:CC:DD:EE:FF scripts/airpods-livetest.sh [path/to/linuxpods-daemon]
set -uo pipefail

MAC="${AIRPODS_MAC:?set AIRPODS_MAC to your AirPods Bluetooth address}"
INSTALLED=/usr/bin/linuxpods-daemon

# Locate the built daemon (anything but the installed one).
DAEMON="${1:-}"
if [[ -z "$DAEMON" ]]; then
    DAEMON=$(find "$HOME" /tmp -maxdepth 6 -name linuxpods-daemon -type f -executable \
                  -not -path '/usr/*' 2>/dev/null | head -1)
fi
if [[ -z "$DAEMON" || ! -x "$DAEMON" ]]; then
    echo "ERROR: pass the path to your built linuxpods-daemon as \$1" >&2
    exit 1
fi
echo ">>> Using built daemon: $DAEMON"

# Everything is tracked by explicit PID — no pkill/pgrep pattern matching, which
# previously matched this script's own argv and recursed through the trap.
BUILT_PID=""
restore() {
    trap - EXIT INT TERM                              # disarm so we can't re-enter
    echo; echo ">>> Restoring installed daemon..."
    [[ -n "$BUILT_PID" ]] && kill "$BUILT_PID" 2>/dev/null
    sleep 0.5
    setsid "$INSTALLED" --hide >/dev/null 2>&1 </dev/null &
    echo ">>> Done."
}
trap restore EXIT INT TERM

echo ">>> Stopping the installed daemon"
INSTALLED_PIDS=$(pidof "$INSTALLED" 2>/dev/null || true)
[[ -n "$INSTALLED_PIDS" ]] && kill $INSTALLED_PIDS 2>/dev/null
sleep 0.5

echo ">>> Disconnecting AirPods from this host (so they reconnect elsewhere)"
bluetoothctl disconnect "$MAC" >/dev/null 2>&1

cat <<EOF

============================ LIVE TEST CHECKLIST ============================
Watch the log below. Toggle modes in another terminal with:
  gdbus call --session -d io.github.Explor3Universe.LinuxPods \\
    -o /io/github/Explor3Universe/LinuxPods \\
    -m io.github.Explor3Universe.LinuxPods.Manager.SetAutoConnectBehavior <0|1|2>
  (0=Off, 1=When worn, 2=When worn & playing)

  A. Mode 1, idle elsewhere:        put on -> "AirPods put on -> connecting" + Connect() succeeded
  B. Mode 1, busy elsewhere (music/call): put on -> "busy on another device; not grabbing"
  C. Mode 2, nothing playing locally:     put on -> "nothing playing locally; waiting"
  D. Mode 2, start local audio, then put on -> connects
  E. Reinsert within 15s of a connect       -> nothing (cooldown)

Press Ctrl-C when done — the installed daemon is auto-restored.
============================================================================

EOF

echo ">>> Running built daemon (--debug). Logs:"
"$DAEMON" --debug &
BUILT_PID=$!
wait "$BUILT_PID"
