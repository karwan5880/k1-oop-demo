# demo1 — object-oriented programming on a Booster K1 humanoid

A fifteen-minute live-coding demo. Class, object, constructor, inheritance and
polymorphism, taught by making a real humanoid robot move its head and arms.

Not `Animal` and `Dog`. Every command in here is one the robot actually accepts.

```
[built] look
[built] nod
[built] look
[built] wave
[built] dance
-> look
-> nod
-> look
-> wave
-> dance
```

The head looks left, nods twice, looks right, the arm waves, the robot cheers.

## What it teaches, and where

| idea | where |
|---|---|
| class, object | `Action` in `src/demo1/src/actions.cpp` |
| constructor, member initialiser list | `Action::Action`, and `: Action(robot, "wave")` in every subclass |
| encapsulation | `Wave` and `Dance` own their own stop command |
| inheritance | `LookAt`, `Nod`, `Wave`, `Dance` — all `: public Action` |
| abstract base class | `virtual void play() = 0;` |
| polymorphism | one `std::vector<Action *>`, one loop, five behaviours |

The whole argument is the last row: the loop does not know what it is holding,
so a sixth behaviour costs one new class and no change to the loop.

## Why this is not a toy example

Everything the K1 does arrives the same way — publish a `RpcReqMsg` on the ROS 2
topic `LocoApiTopicReq` with an api id and a small JSON body:

| api id | vendor name | body |
|---|---|---|
| 2004 | `kRotateHead` | `{"pitch":..,"yaw":..}` |
| 2005 | `kWaveHand` | `{"hand_index":1,"hand_action":0 or 1}` |
| 2016 | `kDance` | `{"dance_id":0..7, 1000 to stop}` |

One act, many kinds of command. That repetition is what the base class removes,
and it is why the code is shaped this way rather than to satisfy a lesson plan.

Two things in here were learned the hard way and are worth the read even if you
never touch a K1:

- **`kRotateHead` is absolute.** Command it before you know where the head is
  and the head snaps at full servo speed. `Robot::waitForHeadPose()` waits for
  one `/head_pose_stamped` message first.
- **`kWaveHand` and `kDance` are start/stop and neither ends by itself.** Forget
  the stop and that action stays live forever, after which the robot refuses
  *every* later command — including from a different program. `Wave` and `Dance`
  each send their own stop, so no caller can forget. That is the clearest
  argument for encapsulation in the whole program, and it came from a real bug.

## Requirements

**A physical Booster K1 humanoid.** This will not build without one:
`booster_msgs` ships with the robot, at `/opt/booster/BoosterRos2/install`.
There is no simulator path here.

- ROS 2 Humble
- g++ 11 or newer, cmake 3.8 or newer
- Tested on the K1 Jetson Orin, aarch64, Ubuntu 22.04

## Build and run

On the robot:

```bash
source /opt/ros/humble/setup.bash
source /opt/booster/BoosterRos2/install/setup.bash
colcon build --packages-select demo1
source install/setup.bash
ros2 run demo1 actions
```

`ros2 run` takes the package name then the executable name — `demo1` then
`actions`.

## Safety

**This program never makes the robot walk.** `kMove` (2001) does not appear
anywhere in it, deliberately: a room full of students is no place for a walking
humanoid. Head and arms only, and the head is slewed in 20 ms steps rather than
jumped.

If you extend it, keep it that way.

## When a command does nothing

It has not failed silently. The robot answers every request on
`/LocoApiTopicResp`, and `Robot` prints anything non-zero:

```
[robot] api 2016 REFUSED status=501 (server refused - busy, wrong mode, or low battery)
     why: ssh booster-robot "grep -A3 'id: 2016' ...(newest)/motion.log | tail -20"
```

The status code names a category. The **reason** is in the robot's own
`motion.log`, in words, which is what that second line fetches — for example
`action 5 conflict with current action`.

## Files

```
src/demo1/include/robot.h     the robot: one ROS node, the loco API, head pose
src/demo1/src/actions.cpp     the lesson: Action and its four subclasses
QNA.md                        every question a student asks, answered
TYPING_SCRIPT.md              the fifteen minutes, beat by beat
```

`QNA.md` is the longer half of this repository and the part worth reading:
constructors and initialiser lists, why the destructor is virtual, references
versus pointers, the three meanings of `static`, what `rclcpp` is, what spinning
is for, and why the arm APIs are start/stop.

## License

MIT — see `LICENSE`.
