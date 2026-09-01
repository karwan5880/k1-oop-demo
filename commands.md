# commands

Everything here runs **on the robot** (`ssh booster-robot`, or the VS Code
terminal when Remote-SSH has `~/oop_ws` open).

## Build and run

Two `source` lines are needed in every new shell — ROS 2 is not on `PATH` by
default, and `booster_msgs` comes from the vendor overlay:

```bash
source /opt/ros/humble/setup.bash
source /opt/booster/BoosterRos2/install/setup.bash
colcon build --packages-select demo1
source install/setup.bash
ros2 run demo1 actions
```

Once both overlays are sourced in a shell, rebuilding and rerunning is one line:

```bash
cd ~/oop_ws && colcon build --packages-select demo1 && source install/setup.bash && ros2 run demo1 actions
```

Expected: five `[built]` lines, then `-> look`, `-> nod`, `-> look`, `-> wave`,
`-> dance`, and no refusal summary.

## When a command does nothing

`demo1` prints refusals itself. To see every response including the successful
ones, capture the topic while it runs:

```bash
cd ~/oop_ws && source install/setup.bash
(ros2 topic echo /LocoApiTopicResp > /tmp/resp.log 2>&1 &) ; sleep 1
ros2 run demo1 actions ; sleep 2 ; pkill -f "topic echo"
grep -B1 -A2 demo1- /tmp/resp.log
```

`{"status":0}` is accepted. Anything else is not, and the number is only a
category — the **reason** is in the vendor's own log, in words:

```bash
grep -A3 "id: 2016" $(ls -td /var/log/booster/*/motion | head -1)/motion.log | tail -20
```

That is what answered it on 2026-09-01: `action 5 conflict with current action`
/ `conflict action and refused`.

## Unstick a robot that refuses everything

An action left running refuses every later command, including head commands from
a different program. Stop the dance by hand:

```bash
source /opt/ros/humble/setup.bash
source /opt/booster/BoosterRos2/install/setup.bash
ros2 topic pub --once /LocoApiTopicReq booster_msgs/msg/RpcReqMsg \
  '{uuid: stop, header: "{\"api_id\":2016}", body: "{\"dance_id\":1000}"}'
```

`dance_id 1000` is `DanceId::kStop` — the same thing the Stop button in the
Booster iOS app sends.

## Ask the robot what mode it is in

```bash
(ros2 topic echo /LocoApiTopicResp > /tmp/mode.log 2>&1 &) ; sleep 2
ros2 topic pub --once /LocoApiTopicReq booster_msgs/msg/RpcReqMsg \
  '{uuid: mode, header: "{\"api_id\":2017}", body: ""}'
sleep 3 ; pkill -f "topic echo" ; grep -A2 "uuid: mode" /tmp/mode.log
```

`{"mode":N}` — 0 damping, 1 prepare, 2 walking, 3 custom, 4 soccer.

## Publishing to GitHub

The robot is where the code is edited; the **workstation** is the only machine
that pushes, so the credentials do not travel to venues. On the workstation:

```bash
cd ~/py/booster
./sync.sh check          # what differs, and in which direction
./sync.sh pull           # robot -> workstation, refuses if it would destroy work
git add -A && git commit && git push
```

Never edit `~/py/booster` by hand — it is a staging area, and an edit there is
overwritten by the next pull.

## Before the lecture

Blank the file you will type into, keeping the finished copy:

```bash
cd ~/oop_ws/src/demo1
cp src/actions.cpp /tmp/actions_final.cpp
: > src/actions.cpp
```

## VS Code says a member does not exist, but it compiles

Stale IntelliSense. **In this order**, or it does nothing:

1. close the file
2. Ctrl+Shift+P → C/C++: Reset IntelliSense Database
3. reopen the file

`colcon build` is the only verdict that counts.
