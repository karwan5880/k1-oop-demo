#!/usr/bin/env bash
#
# The robot is the source of truth. This machine only publishes.
#
#   ~/oop_ws on booster-robot   ->   ~/py/booster   ->   github
#        edit, build, run              commit, push
#
# RULE A: never edit ~/py/booster by hand. Edit on the robot, then run this.
#   Anything changed here and rsynced the other way silently overwrites whatever
#   the robot had, and the two directions look identical while they are agreeing.
#   They only differ on the day it matters.
#
# RULE B (what this script enforces): before syncing, prove that nothing here
#   would be destroyed by it. `check` compares both sides and names every file
#   that differs; `pull` refuses to run unless check passes, or unless --force
#   says the robot wins on purpose.
#
# Usage:
#   ./sync.sh check      what differs, and in which direction. Changes nothing.
#   ./sync.sh pull       robot -> here, only if nothing here is at risk
#   ./sync.sh pull --force   robot wins, discard local-only edits
#   ./sync.sh            same as check
#
set -euo pipefail

ROBOT=${ROBOT:-booster-robot}
HERE=$(cd "$(dirname "$0")" && pwd)
MODE=${1:-check}
FORCE=${2:-}

# --delete keeps this tree honest about files removed on the robot, so anything
# that lives ONLY here has to be named. sync.sh deleted itself the first time.
RSYNC_ARGS=(-a --delete --exclude '.git/' --exclude 'sync.sh'
            --filter=':- .gitignore')

# Everything rsync WOULD do, without doing any of it.
plan() { rsync "${RSYNC_ARGS[@]}" --dry-run --itemize-changes \
                "$ROBOT:oop_ws/" "$HERE/"; }

# Files this machine has modified since the last commit. Losing these to a pull
# is the exact accident RULE B exists to prevent.
dirty() { git -C "$HERE" status --porcelain | awk '{print $NF}'; }

case "$MODE" in
  check|pull) ;;
  *) echo "usage: $0 [check|pull] [--force]" >&2; exit 64 ;;
esac

echo "comparing $ROBOT:oop_ws/  ->  $HERE/"
CHANGES=$(plan | grep -v '^\.d\.\.t' || true)
LOCAL=$(dirty || true)

if [ -z "$CHANGES" ]; then
  echo "  robot and here are identical."
else
  echo
  echo "the robot would change these:"
  echo "$CHANGES" | sed 's/^/  /'
fi

# The dangerous overlap: a file uncommitted HERE that the pull would also write.
AT_RISK=""
if [ -n "$LOCAL" ] && [ -n "$CHANGES" ]; then
  for f in $LOCAL; do
    if echo "$CHANGES" | awk '{print $2}' | grep -qxF "$f"; then
      AT_RISK="$AT_RISK $f"
    fi
  done
fi

if [ -n "$LOCAL" ]; then
  echo
  echo "uncommitted here (RULE A says this should be empty):"
  # shellcheck disable=SC2086
  for f in $LOCAL; do echo "  $f"; done
fi

if [ -n "$AT_RISK" ]; then
  echo
  echo "STOP: these are modified here AND would be overwritten by the pull:"
  # shellcheck disable=SC2086
  for f in $AT_RISK; do echo "  $f"; done
  echo
  echo "Your edit and the robot's disagree. Decide which is right:"
  echo "  - the robot is right  ->  ./sync.sh pull --force"
  echo "  - you are right       ->  copy it TO the robot, then pull"
  [ "$MODE" = "pull" ] && [ "$FORCE" != "--force" ] && exit 1
fi

if [ "$MODE" = "check" ]; then
  echo
  echo "nothing changed. run './sync.sh pull' to bring the robot's copy across."
  exit 0
fi

rsync "${RSYNC_ARGS[@]}" "$ROBOT:oop_ws/" "$HERE/"
echo
echo "pulled. now:"
git -C "$HERE" status --short
echo
echo "  git add -A && git commit && git push"
