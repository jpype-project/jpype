#!/usr/bin/env bash
# Install the jpypetest APK on a running emulator, launch it, and evaluate
# the results testapp/main.py prints to logcat. Used by
# .github/workflows/android.yml (emulator-test job); also runnable by hand
# against any emulator/device adb can see:
#
#     APK=bin/jpypetest-0.1-x86_64-debug.apk bash project/android/ci_emulator_check.sh
#
# Env:
#   APK          path to the APK (default: first apk/*.apk)
#   APP_PACKAGE  default org.jpype.test.jpypetest
#   LOG          where the logcat capture goes (default: ./logcat.txt)
#   RUN_TIMEOUT  seconds to wait for the end marker (default: 1800)
set -uo pipefail

APK="${APK:-$(ls apk/*.apk 2>/dev/null | head -n1)}"
APP_PACKAGE="${APP_PACKAGE:-org.jpype.test.jpypetest}"
ACTIVITY="$APP_PACKAGE/org.kivy.android.PythonActivity"
LOG="${LOG:-$PWD/logcat.txt}"
RUN_TIMEOUT="${RUN_TIMEOUT:-1800}"
SUMMARY="${GITHUB_STEP_SUMMARY:-/dev/null}"

START_MARKER='=== jpype android testapp starting ==='
END_MARKER='=== jpype android testapp done ==='

if [ -z "$APK" ] || [ ! -f "$APK" ]; then
  echo "ERROR: no APK found (APK='$APK')" >&2
  exit 1
fi

# True only if adb answers and the app has no process, twice in a row.
# pidof exits 1 when there is no such process; any other failure (adb
# offline or reconnecting) is not taken as "gone".
app_gone() {
  local n
  for n in 1 2; do
    adb shell pidof "$APP_PACKAGE" >/dev/null 2>&1
    [ $? -eq 1 ] || return 1
    sleep 2
  done
  return 0
}

# --- 1. wait until the device is really up -------------------------------
adb start-server >/dev/null 2>&1 || true
if ! timeout 300 adb wait-for-device; then
  echo "ERROR: emulator never reached device state" >&2
  adb devices || true
  exit 1
fi

booted=0
for i in $(seq 1 150); do
  if [ "$(adb shell getprop sys.boot_completed 2>/dev/null | tr -d '\r')" = "1" ] &&
     adb shell pm path android >/dev/null 2>&1; then
    booted=1
    break
  fi
  sleep 2
done
if [ "$booted" -ne 1 ]; then
  echo "ERROR: emulator did not finish booting" >&2
  exit 1
fi

# sys.boot_completed flips before the system is done with its post-boot
# work (the BOOT_COMPLETED broadcast and a burst of config changes follow
# it); an app launched into that window was killed during asset extraction
# on CI. Give it time to settle, then pin the device state: screen on and
# unlocked, rotation fixed to portrait.
sleep 30
adb shell settings put global window_animation_scale 0 >/dev/null 2>&1 || true
adb shell settings put global transition_animation_scale 0 >/dev/null 2>&1 || true
adb shell settings put global animator_duration_scale 0 >/dev/null 2>&1 || true
adb shell settings put system accelerometer_rotation 0 >/dev/null 2>&1 || true
adb shell settings put system user_rotation 0 >/dev/null 2>&1 || true
adb shell svc power stayon true >/dev/null 2>&1 || true
adb shell input keyevent KEYCODE_WAKEUP >/dev/null 2>&1 || true
adb shell wm dismiss-keyguard >/dev/null 2>&1 || true

# --- 2. install, then launch (retrying only start-up deaths) ----------------
if ! adb install -r "$APK"; then
  echo "ERROR: adb install failed" >&2
  exit 1
fi

adb logcat -G 16M >/dev/null 2>&1 || true
: > "$LOG"
rm -f "$LOG".attempt*
LOGCAT_PID=
cleanup() { [ -n "$LOGCAT_PID" ] && kill "$LOGCAT_PID" 2>/dev/null; }
trap cleanup EXIT

# A launch is only retried if the process dies *before* main.py printed its
# start marker - i.e. in the Android/p4a start-up phase. Anything after that
# is a real result and must not be papered over by a retry.
started=0
for attempt in 1 2 3; do
  echo "--- launch attempt $attempt ---"
  cleanup
  adb shell am force-stop "$APP_PACKAGE" >/dev/null 2>&1 || true
  adb logcat -c || true
  adb logcat -v threadtime > "$LOG.attempt$attempt" &
  LOGCAT_PID=$!
  sleep 1
  adb shell am start -W -n "$ACTIVITY" || true

  for i in $(seq 1 60); do       # up to ~2 min for the start marker
    if grep -qF "$START_MARKER" "$LOG.attempt$attempt"; then started=1; break; fi
    if [ "$i" -gt 5 ] && app_gone; then
      # logcat reaches the host file with a delay; a crash right after
      # the marker must still count as started, not be retried away.
      if grep -qF "$START_MARKER" "$LOG.attempt$attempt"; then started=1; break; fi
      echo "app process died before '$START_MARKER' appeared"
      break
    fi
    sleep 2
  done
  [ "$started" -eq 1 ] && break
done
CUR_LOG="$LOG.attempt$attempt"

if [ "$started" -eq 1 ]; then
  deadline=$(( $(date +%s) + RUN_TIMEOUT ))
  while [ "$(date +%s)" -lt "$deadline" ]; do
    grep -qF "$END_MARKER" "$CUR_LOG" && break
    if app_gone; then
      echo "app process is gone before '$END_MARKER' appeared"
      break
    fi
    sleep 5
  done
  sleep 3
fi
cleanup
LOGCAT_PID=
cat "$LOG".attempt* > "$LOG"

# --- 3. evaluate ------------------------------------------------------------
fail=0
check() {
  if grep -qF "$1" "$CUR_LOG"; then echo "ok:   $1"; else echo "FAIL: missing '$1'"; fail=1; fi
}
check 'GOLDEN PATH: PASS'
check 'REGRESSION CHECK #1257: PASS'
check "$END_MARKER"

result=$(grep -o 'PORTED SUITE: ran=[0-9]* failures=[0-9]* errors=[0-9]* skipped=[0-9]*' "$CUR_LOG" | tail -n1)
if [ -z "$result" ]; then
  echo "FAIL: no 'PORTED SUITE: ran=...' summary line"; fail=1
else
  echo "$result"
  echo "$result" | grep -q 'failures=0 errors=0' || fail=1
  # Guard against a suite that silently collected nothing.
  [ "$(echo "$result" | sed 's/.*ran=\([0-9]*\).*/\1/')" -gt 0 ] || { echo "FAIL: ran=0"; fail=1; }
fi

if grep -qE 'PORTED SUITE: (FAILED TO LOAD|FAIL to run)' "$CUR_LOG"; then
  grep -E 'PORTED SUITE: (FAILED TO LOAD|FAIL to run)' "$CUR_LOG"; fail=1
fi

if grep -q 'terminating due to uncaught exception' "$CUR_LOG"; then
  echo "FAIL: uncaught C++ exception in logcat"; fail=1
fi

{
  echo "### Android on-device result"
  echo
  echo "launch attempts: $attempt"
  echo '```'
  grep -hE 'GOLDEN PATH|PORTED SUITE|REGRESSION CHECK' "$CUR_LOG" | sed 's/^.*python *: *//'
  echo '```'
} >> "$SUMMARY"

if [ "$fail" -ne 0 ]; then
  echo "---- failing/erroring tests ----"
  grep -E 'python *: (FAIL|ERROR)(:| )' "$CUR_LOG" | tail -n 60
  echo "---- last 400 lines of logcat ----"
  tail -n 400 "$CUR_LOG" || true
fi

exit "$fail"
