# 15-min live OOP demo — C++ on the K1

Robot moves its head. Head only, no walking. Does not touch the v6 agent.

**VERIFIED ON THE ROBOT 2026-09-01** — built with colcon on the K1 (ROS humble,
aarch64) over VS Code Remote-SSH on the iPad hotspot, and the head moved.

## The venue setup (verified 2026-09-01)

Robot on the iPad hotspot — set from the **Booster iOS app over Bluetooth**, not
over ssh (see `k1pro/docs/NETWORK.md`). Laptop on the same hotspot. Both land on
one `/28`, Tailscale goes direct, ssh connect measured 0.05-0.20 s. No internet
in the critical path.

    ssh booster-robot          # -> 100.70.61.7

## On the robot

Verified present: ROS **humble**, `colcon` at /usr/bin/colcon, g++ 11.4.0,
cmake 3.22.1, `booster_msgs` at /opt/booster/BoosterRos2/install.

**There is no `nano`.** `vim` and `vi` only. If you want nano in front of a
class, install it before you travel.

Source both, every new shell:

    source /opt/ros/humble/setup.bash
    source /opt/booster/BoosterRos2/install/setup.bash

## VS Code says a member does not exist and the build is fine

IntelliSense caches the header and does not re-read it when the file changes
underneath VS Code (an rsync from the workstation, say). It then reports
`class "Robot" has no member "..."` for a member that is right there.

**The order matters, and only this order works:**

1. CLOSE the file
2. Ctrl+Shift+P -> C/C++: Reset IntelliSense Database
3. REOPEN the file

Resetting with the file still open does nothing -- the open editor immediately
repopulates the database from the stale parse. `colcon build` is the only
verdict that counts; if it compiles, the squiggle is wrong.

## Before class (Karwan runs these)

The package is already at `~/oop_ws/src/demo1/` on the robot. Build it once,
to prove it builds:

    ssh booster-robot
    cd ~/oop_ws
    source /opt/ros/humble/setup.bash
    source /opt/booster/BoosterRos2/install/setup.bash
    colcon build --packages-select demo1
    source install/setup.bash
    ros2 run demo1 actions

Then blank `src/actions.cpp`, keeping a copy:

    cp src/demo1/src/actions.cpp /tmp/actions_final.cpp
    : > src/demo1/src/actions.cpp

In class you ssh in, `vim src/demo1/src/actions.cpp`, and type.
`/tmp/actions_final.cpp` is the safety net.

## What you do NOT type

`include/robot.h` — pre-made, ~60 lines. Explain it in one sentence:

> This waits until the robot tells me where its head is, then moves it in small
> steps. `kRotateHead` is absolute — command it blind and the head snaps.

## Student questions

`QNA.md` in this folder answers what they will ask: class, object, constructor,
member initialiser list, public/protected/private, inheritance, virtual, `= 0`,
override, polymorphism and vtables, references vs pointers, new/delete, static
(all three meanings), const, headers, ROS 2, rclcpp, nodes, topics, callbacks,
spin, the loco API, and why both arm APIs are start/stop. Read it before class.

## The five beats

**1. class + object (0:00–2:00)**

```cpp
class Action {
public:
    std::string name_;
    void run() { std::cout << name_ << "\n"; }
};
// main: Action m; m.name_ = "nod"; m.run();
```
Class = blueprint. `m` = the object. Ask: what is `name_` before I set it?

**2. constructor (2:00–5:00)**

```cpp
    Action(Robot &robot, std::string name) : robot_(robot), name_(name) {}
```
Then try `Action m;` again on purpose. Let the compiler error hit the screen —
that is the constructor refusing to build a move with no robot.

**3. pure virtual (5:00–7:00)**

```cpp
    virtual void play() = 0;
```
`run()` never changes: print, then `play()`. `play()` changes every time.
`= 0` means I refuse to answer; my subclasses must.

**4. inheritance (7:00–12:00)** — the core

```cpp
class Nod : public Action {
public:
    Nod(Robot &robot, int times) : Action(robot, "nod"), times_(times) {}
    ...
};
```
Say while typing `: Action(robot, "nod")` — before a `Nod` can exist, the
`Action` inside it must be built first. Constructors run base-first. A
subclass cannot skip its parent.

Then `LookAt` the same way. It should feel boring. That is the point.

**5b. the arm — one new class, loop untouched**

```cpp
class Wave : public Action {
public:
    Wave(Robot &robot, double seconds)
        : Action(robot, "wave"), seconds_(seconds) {}
protected:
    void play() override {
        robot_.callLocoApi(Robot::kWaveHand, "{\"hand_index\":1,\"hand_action\":0}");
        robot_.hold(seconds_);
        robot_.callLocoApi(Robot::kWaveHand, "{\"hand_index\":1,\"hand_action\":1}");
    }
private:
    double seconds_;
};
```

This is the closing argument, so land it deliberately: the base class is called
`Action`, not `Action`, and adding an ARM move needed no change to it and
no change to the loop. `Wave` is a different limb, a different vendor API, and
a two-part start/stop instead of a single command — and the loop still just
calls `run()`.

`kWaveHand` (2005) is START/STOP, not one-shot: `hand_action:0` starts a
LOOPING wave, `1` stops it and returns the arm to rest. The class owns that
pairing, which is the encapsulation point — nobody using `Wave` can forget the
stop.

`Dance` (2016) takes `dance_id` 5 bow, 6 cheer, 7 lucky-cat. Upper body,
self-completing. Never WholeBodyDance — that moves the legs.

**5. polymorphism (12:00–15:00)** — the payoff

```cpp
    std::vector<Action*> lesson;
    lesson.push_back(new LookAt(*robot, 0.5f));
    lesson.push_back(new Nod(*robot, 2));
    for (Action *move : lesson) move->run();
```

Build and run in front of them:

    colcon build --packages-select demo1 && source install/setup.bash
    ros2 run demo1 actions

The head looks left, nods twice, looks right.

Closing line:

> That loop does not know which kind of move it is holding. When I add a new
> behaviour next week I write one class and I do not touch the loop.

## If the robot is down

`ros2 run` prints `no /head_pose_stamped -- is the robot up?` and exits 1.
Nothing moves, nothing hangs. The code still compiled in front of them, which
is most of the lesson.
