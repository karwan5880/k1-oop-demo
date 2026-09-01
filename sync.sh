#!/usr/bin/env bash
# Pull the workspace off the robot, then push from here.
#
# The robot is where the code is edited (VS Code Remote-SSH into ~/oop_ws) and
# where it is built and run. This machine is where git lives, because the
# GitHub credentials are here and a robot that travels to venues should not
# carry a deploy key.
#
# Build artifacts and the private _notes.txt never come across: .gitignore is
# passed to rsync as a filter, so the exclude list has exactly one home.
#
# --delete keeps this tree honest about files removed on the robot, which means
# anything that lives ONLY here has to be protected from it by name. sync.sh
# deleted itself the first time this ran.
set -euo pipefail

ROBOT=${ROBOT:-booster-robot}
HERE=$(cd "$(dirname "$0")" && pwd)

rsync -a --delete \
  --exclude '.git/' \
  --exclude 'sync.sh' \
  --filter=':- .gitignore' \
  "$ROBOT:oop_ws/" "$HERE/"

cd "$HERE"
git status --short
echo
echo "review the above, then:  git add -A && git commit && git push"
