# demo1 — questions students ask, and the answers

Every answer points at the real code in `demo1/`: `include/robot.h` (the robot)
and `src/actions.cpp` (the actions). Nothing here is invented for teaching — it
is all in the program that made the K1 move.

---

# Part 1 — The idea

## What problem is OOP actually solving here?

Everything the K1 does arrives the same way: publish a message on the ROS topic
`LocoApiTopicReq` with an api id and a small JSON body. Turning the head, waving,
dancing — same act, different numbers.

Without classes you write that publish once per behaviour and copy it. Five
behaviours, five copies. Change the topic name and you edit five places and miss
one.

With classes you write it **once** in `Robot::callLocoApi()` and every action
inherits the ability to use it. `Action` owns what never changes; each subclass
supplies only what differs. That is the whole point, and everything below is
machinery serving it.

## What is a class? What is an object?

A **class** is a description. An **object** is a thing that exists in memory
built from that description.

```cpp
class Action { ... };            // the description
LookAt look(robot, 0.5f);        // an object. This one really exists.
```

`Action` weighs nothing; it is a plan. `look` occupies memory, holds values, and
can be told to do things. One class, as many objects as you want — `demo1`
builds five.

## What is an instance? Same as an object?

Yes. "An instance of `LookAt`" and "a `LookAt` object" mean the same thing.

## What is a member?

Anything declared inside the class: variables (`name_`, `robot_`) and functions
(`perform()`, `play()`). Member variables are also called **fields** or
**attributes**; member functions are also called **methods**.

## Why does every member variable end with an underscore?

`name_`, `robot_`, `pitch_`. It is a convention, not a language rule, and this
repo uses it throughout — the C++ agent (`agent_node.cpp`) does the same. It
lets you tell at a glance whether a name belongs to the object or is a local
variable, which matters most inside a constructor where both exist at once.

---

# Part 2 — Constructors

## What is a constructor?

The function that runs once, automatically, when an object is born. It has the
same name as the class and no return type.

```cpp
Action(Robot &robot, std::string name)
    : robot_(robot), name_(name) {
    std::cout << "[built] " << name_ << "\n";
}
```

Its job is to leave the object in a valid state. After it returns, the object is
usable — that is the promise.

## Why do we need one? Why not just set the values afterwards?

Because "afterwards" is optional and people forget. Before the constructor
existed in this demo, you could write:

```cpp
Action a;          // what is a.name_ right now? Garbage. Whatever was in memory.
a.name_ = "wave";  // and if you forget this line, nothing warns you
```

With a constructor that requires a `Robot`, `Action a;` **does not compile**.
The language now refuses to create an action with no robot to send it to. A
whole category of bug stops being possible instead of being something you have
to remember.

> **Try this in class:** type `Action a;` and let the compiler error appear. That
> error is the constructor doing its job.

## What is `: robot_(robot), name_(name)` — that colon before the braces?

The **member initialiser list**. It initialises members *as they are created*,
before the constructor body runs.

```cpp
Action(Robot &robot, std::string name)
    : robot_(robot), name_(name)      // <- initialise here
{
    std::cout << ...;                 // <- body runs after
}
```

You could write `name_ = name;` in the body instead and it would work — but that
first *default-constructs* an empty string and then *assigns* over it. Two steps
where one would do.

For `robot_` there is no choice at all: it is a **reference**, and a reference
must be bound the moment it is created. It cannot be assigned later. So it has
to be in the initialiser list.

## Members are initialised in declaration order, not list order

A trap worth knowing. If you write `: name_(name), robot_(robot)` the compiler
still initialises `robot_` first, because `robot_` is declared first in the
class. Keep the list in the same order as the declarations and the question
never arises.

## What is a destructor? Why `virtual ~Action() {}`?

The mirror of the constructor: it runs when the object is destroyed. `~Action`.

The `virtual` matters and is not decoration. In `main` we hold actions as base
pointers:

```cpp
std::vector<Action *> lesson;
lesson.push_back(new Wave(*robot, 3.0));
...
delete action;              // action is an Action*, but it points at a Wave
```

Without `virtual`, `delete` on an `Action*` calls only `~Action` — the `Wave`
part is never destroyed. That leaks whatever the subclass owned, and it is
formally undefined behaviour.

**The rule: if a class has any virtual function, give it a virtual destructor.**

Ours are empty (`{}`) because our subclasses hold only `int` and `double`, which
need no cleanup. It is still virtual, because the rule is about the shape of the
class, not about today's members.

---

# Part 3 — public, protected, private

## What do they mean?

Who is allowed to touch a member.

| | the class itself | subclasses | outside code |
|---|---|---|---|
| `public` | yes | yes | yes |
| `protected` | yes | yes | **no** |
| `private` | yes | **no** | **no** |

## Why is `perform()` public but `play()` protected?

Look at what each one is for.

```cpp
public:
    void perform();          // what the OUTSIDE asks for
protected:
    virtual void play() = 0; // what SUBCLASSES supply
```

`main` says `action->perform()`. It never calls `play()` and must not — calling
`play()` directly would skip the `-> nod` line that `perform()` prints, so the
program's output would silently stop matching what the robot did.

`play()` is protected rather than private because subclasses must be able to
*define* it. `LookAt`, `Nod`, `Wave` and `Dance` each write their own.

That split is the design: **`perform()` is the contract with the outside world,
`play()` is the contract with subclasses.** They are different audiences and
they get different doors.

## Why is `robot_` protected instead of private?

Because subclasses use it — `Wave::play()` calls `robot_.callLocoApi(...)`. If
it were private, `Wave` could not see it.

## Why is `statusOf()` private in `Robot`?

Nobody outside `Robot` has any business parsing a status header. Keeping it
private means it can be rewritten or deleted without checking whether anything
else depends on it. Private is not about secrecy; it is about being free to
change your mind later.

## What is the default if I write neither?

`class` defaults to `private`. (`struct` defaults to `public` — that is the only
real difference between them in C++.) Always write the keyword; do not rely on
the default.

---

# Part 4 — Inheritance

## What does `class Wave : public Action` mean?

`Wave` **is an** `Action`. It gets every member `Action` has, and can add its
own. In other languages the keyword is `extends`; in C++ it is the colon.

The "is a" test is how you check whether inheritance is the right tool. A wave
is an action — true, so this is sound. A wave is not *a kind of robot*, which is
why `Wave` does not inherit from `Robot`.

## Why `public` in `: public Action`?

It preserves the access levels: public members of `Action` stay public in
`Wave`. `private` inheritance exists and means "implemented in terms of" rather
than "is a" — you will almost never want it. Use `public`.

## What does `Wave` actually get?

Everything: `perform()`, `robot_`, `name_`, the constructor's behaviour. It
writes only `play()` and one member of its own (`seconds_`). Roughly eight lines
buys a complete new robot behaviour.

## Why does `Wave`'s constructor say `: Action(robot, "wave")`?

Because a `Wave` **contains** an `Action`, and that inner `Action` has to be
built before the `Wave` around it can exist. That is the base constructor call.

```cpp
Wave(Robot &robot, double seconds)
    : Action(robot, "wave"), seconds_(seconds) {}
```

Order of events when you write `new Wave(robot, 3.0)`:

1. `Action(robot, "wave")` runs — prints `[built] wave`
2. `seconds_` is initialised to 3.0
3. `Wave`'s own body runs (empty here)

**Constructors run base-first, outward.** A subclass can never skip its parent.
If you omit the call, the compiler tries the base's *default* constructor — and
`Action` has none, so you get an error. That error is the language enforcing
that no `Wave` can exist without a valid `Action` inside it.

## In what order do destructors run?

Exactly backwards: `~Wave` first, then `~Action`. Built inside-out, destroyed
outside-in.

## Can a class inherit from two classes?

C++ allows it; most languages do not. Do not reach for it while learning. Every
problem in this demo is solved by single inheritance.

---

# Part 5 — virtual, `= 0`, override, polymorphism

## What does `virtual` do?

It tells C++: when this function is called through a base pointer or reference,
look up what the *actual object* is and call that version.

Without `virtual`, the compiler decides at compile time from the pointer's
declared type. With `virtual`, the decision happens at run time from the object
itself. That run-time decision is called **dynamic dispatch**, and it is what
makes the loop in `main` work.

## What does `= 0` mean? That looks like assigning zero.

It is unrelated to the number zero — it is just the syntax C++ chose.

```cpp
virtual void play() = 0;
```

reads: *I declare this function exists, and I refuse to write it. My subclasses
must.* It is called a **pure virtual function**.

## What is an abstract class?

A class with at least one pure virtual function. `Action` is abstract, and that
has a concrete consequence:

```cpp
Action a(robot, "thing");   // COMPILER ERROR. Action is abstract.
LookAt look(robot, 0.5f);   // fine
```

You cannot create a bare `Action`, and that is correct — "an action" with no
particular thing to do is not a real object. `Action` exists to be inherited
from.

Other languages call this an **interface** or an **abstract base class (ABC)**.

## What does `override` do?

```cpp
void play() override { ... }
```

It says *I intend to replace a virtual function from my base class.* If no such
function exists — you misspelled it, or got the signature wrong — the compiler
errors instead of silently creating an unrelated new function that never gets
called.

It is optional. Always write it. Without it, changing `play()` to
`play(int)` in the base leaves four subclasses quietly defining dead functions,
and the program compiles and does nothing.

## What is polymorphism, in this program?

```cpp
std::vector<Action *> lesson;
lesson.push_back(new LookAt(*robot, 0.5f));
lesson.push_back(new Nod(*robot, 2));
lesson.push_back(new Wave(*robot, 3.0));
lesson.push_back(new Dance(*robot, 6, 8.0));

for (Action *action : lesson)
    action->perform();
```

One list. One loop. Four different behaviours, on two different limbs, through
two different vendor APIs, one of which is a start/stop pair.

**The loop does not know what it is holding.** It calls `perform()`; the object
decides what happens. Adding a fifth behaviour next week means writing one class
and touching nothing here.

Greek: *poly* = many, *morph* = form. Many forms behind one interface.

## How does the computer know which `play()` to call?

Each object with virtual functions carries a hidden pointer to a **vtable** — a
small table of function addresses for its actual class. `action->perform()`
calls `play()` through that table, so a `Wave` object reaches `Wave::play` even
when the pointer says `Action *`.

You never write this; it is what `virtual` costs and buys. The cost is one
pointer per object and one indirection per call — irrelevant next to an ssh
round trip to a robot.

---

# Part 6 — References, pointers, memory

## What is `&` in `Robot &robot`?

A **reference** — another name for an existing object. Not a copy.

```cpp
Action(Robot &robot, ...) : robot_(robot) { }
```

Without `&`, C++ would copy the whole `Robot` into the constructor. Every action
would hold its own copy of the robot, publishers and subscriptions and all, and
commands would go nowhere. (In fact `Robot` inherits from `rclcpp::Node`, which
cannot be copied — so it would not even compile. The language caught the mistake
for us.)

**There is one robot. Everything refers to that one.** That is what `&` says.

## Reference vs pointer — what is the difference?

| | reference `Robot &r` | pointer `Robot *p` |
|---|---|---|
| can be null | no | yes |
| can be reseated | no | yes |
| syntax | `r.headPitch()` | `p->headPitch()` |
| must be initialised | yes | no |

Rule of thumb: **use a reference when the thing always exists; use a pointer
when it might not, or when you need a container of different types.** Both
appear in this demo for exactly those reasons.

## Then why is `lesson` a vector of pointers?

```cpp
std::vector<Action *> lesson;
```

Because it holds *different types* — `LookAt`, `Nod`, `Wave`, `Dance` — and they
are different sizes. A `std::vector<Action>` stores actual `Action` objects of
one fixed size; putting a `Wave` in one would slice off the `Wave` part and keep
only the `Action` (a real bug with a real name: **object slicing**). And you
cannot have a vector of `Action` anyway, because `Action` is abstract.

Pointers are all the same size regardless of what they point at. That is what
makes one container of mixed types possible, and polymorphism with it.

## What do `new` and `delete` do?

`new` allocates an object on the **heap** and returns its address. It lives until
you `delete` it.

```cpp
lesson.push_back(new Wave(*robot, 3.0));   // born here
...
for (Action *action : lesson) delete action;   // dies here
```

Forget the `delete` and the memory is never returned — a **memory leak**.

## Is raw `new`/`delete` good practice?

Not in production C++ — modern code uses `std::unique_ptr` or `std::shared_ptr`,
which delete automatically. `std::vector<std::unique_ptr<Action>>` would be the
grown-up version of `lesson`.

It is written with raw `new`/`delete` here on purpose: in a fifteen-minute
lesson, smart pointers add template syntax that has nothing to do with
inheritance, and they hide the moment of destruction that `virtual ~Action()`
exists to explain. This is a deliberate simplification, and now you know it is
one.

## What is `*robot` in `new Wave(*robot, 3.0)`?

`robot` is a `std::shared_ptr<Robot>` — a pointer. `Wave`'s constructor wants a
`Robot &` — a reference. The `*` **dereferences**: "the object this points at".

## What is `std::make_shared<Robot>()`?

It creates a `Robot` on the heap and hands back a `shared_ptr` — a pointer that
counts how many people hold it and deletes the object when the last one lets go.
ROS 2 wants nodes held this way, because internals keep their own references.

## What is `->` versus `.`?

`.` on an object or reference; `->` on a pointer. `action->perform()` is exactly
`(*action).perform()`, written the short way.

---

# Part 7 — static and const

## What does `static` mean on a member?

**It belongs to the class, not to any object.** One copy exists, no matter how
many objects there are.

```cpp
static constexpr int kRotateHead = 2004;
```

`kRotateHead` is not stored in each `Robot` — it is a property of the concept.
You use it without an object: `Robot::kRotateHead`.

## Why not just write `2004` where it is used?

Because `callLocoApi(2004, ...)` is a magic number. `Robot::kRotateHead` names
it, and the name is the vendor's own — you can find `kRotateHead = 2004` in
`b1_loco_api.hpp`. A reader in a year can look it up. `2004` sends them
guessing.

## What does `constexpr` add?

"Known at compile time." The value is baked into the program; there is no
run-time lookup and no memory to read.

## And `static` on a *function*?

```cpp
static int statusOf(const std::string &header);
```

Same idea: it belongs to the class, not to an object. It uses no member
variables — hand it a header string, get a number back. Marking it `static` says
so, and the compiler enforces it: a static function that touches `pitch_` will
not compile.

## Careful — `static` inside a function body means something else

```cpp
void callLocoApi(int apiId, const std::string &body) {
    static uint64_t sequence = 0;
    ...
}
```

Here `static` means: **this variable is created once and survives between
calls.** A plain local would reset to 0 every call and every request would have
the same uuid. This one counts 0, 1, 2, 3... for the life of the program.

Same keyword, three meanings, depending on where it appears. C++ does this.

## What does `const` on the end of a function mean?

```cpp
float headPitch() const { return pitch_; }
```

*This function does not modify the object.* The compiler enforces it — add
`pitch_ = 0;` inside and it will not compile. It also means the function can be
called on a `const Robot &`, which a non-const function cannot.

Mark every member function `const` that does not change anything. It is free
documentation the compiler checks.

## What about `const std::string &header` in a parameter?

Two things at once. `&` means do not copy the string; `const` means this
function will not change the caller's copy. Together: **fast and safe** — the
standard way to pass a string you only intend to read.

---

# Part 8 — Headers, files, and the build

## Why is there a `.h` and a `.cpp`?

The header declares what exists; the source defines what runs. Anything that
needs `Robot` includes `robot.h`.

Our own file is `robot.h`. The SDK's are `.hpp`. That is this project's
convention: **`.h` is ours, `.hpp` came from someone else.** The include block
at the top of `robot.h` is a boundary you can read at a glance.

## Why is the whole `Robot` class written inside the header?

Convenience for a small demo — one file to read. Larger programs put
declarations in the header and definitions in a matching `.cpp`, so that
changing one function does not force everything that includes the header to
recompile. `agent_node.cpp` in this repo is built that way.

## What is `#pragma once`?

"Include this file at most once per translation unit." Without it, a header
included twice defines its classes twice and the compile fails. The older
equivalent is the `#ifndef`/`#define` include guard; `#pragma once` does the same
in one line and every compiler you will meet supports it.

## What does `#include` actually do?

Pastes the contents of that file in, textually, before compilation. `<angle
brackets>` search the system and configured include paths; `"quotes"` search the
current directory first. That is the whole difference.

## What are CMakeLists.txt and package.xml?

The build instructions. `CMakeLists.txt` says which source files make which
program and what to link against; `package.xml` declares the package's name and
its dependencies to the ROS tooling. `colcon` reads both.

## What does each build command do?

```
source /opt/ros/humble/setup.bash              # put ROS 2 on PATH and in cmake's search
source /opt/booster/BoosterRos2/install/setup.bash   # add the vendor's booster_msgs
colcon build --packages-select demo1           # configure + compile
source install/setup.bash                      # make the built package findable
ros2 run demo1 actions                         # run executable `actions` from package `demo1`
```

`ros2 run` takes **two** names: the package, then the executable. They are
different things that often share a name; here they deliberately do not.

---

# Part 9 — What is ROS 2, and what is rclcpp?

## What is ROS 2?

**Robot Operating System 2** — not an operating system, despite the name. It is a
set of libraries and conventions for letting many small programs on a robot talk
to each other.

The one idea you need today: programs publish **messages** on named **topics**,
and other programs subscribe to those topics. Nobody connects to anybody
directly. A publisher does not know or care who is listening.

The K1 runs ROS 2 **Humble** on its Jetson.

## So what is `rclcpp`?

**R**OS **C**lient **L**ibrary for **C++**. The C++ API for ROS 2 — it gives you
`rclcpp::Node`, publishers, subscriptions, and the spinning machinery. (Python
gets `rclpy`; same concepts.)

`#include <rclcpp/rclcpp.hpp>` is what makes all of that available.

## What is a node?

One participating program. `class Robot : public rclcpp::Node` means **our robot
class IS a ROS node** — that is inheritance again, from a library class this
time, and it is why `Robot` can call `create_publisher` and
`create_subscription` as if they were its own. They are: it inherited them.

```cpp
Robot() : rclcpp::Node("demo1") { ... }
```

`"demo1"` is this node's name on the ROS graph. `ros2 node list` shows it while
the program runs.

## What is a topic? What is a message?

A **topic** is a named channel: `LocoApiTopicReq`, `/head_pose_stamped`. A
**message** is a typed struct sent on it.

`booster_msgs/msg/RpcReqMsg` has exactly three string fields:

```
string uuid
string header
string body
```

`ros2 interface show booster_msgs/msg/RpcReqMsg` prints that. Our three topics:

| topic | direction | message | carries |
|---|---|---|---|
| `LocoApiTopicReq` | we publish | `RpcReqMsg` | a command |
| `/LocoApiTopicResp` | we subscribe | `RpcRespMsg` | the robot's answer |
| `/head_pose_stamped` | we subscribe | `PoseStamped` | where the head is, 100×/s |

## What is the `10` in `create_publisher<...>("LocoApiTopicReq", 10)`?

The **queue depth**. If messages arrive faster than they can be handled, ROS
keeps this many and drops the oldest. 10 is the ordinary default.

## What is a callback? What is `[this](...)`?

A **callback** is a function you hand to the library, to be called later when
something arrives. You do not call it; ROS does.

`[this](const ...::SharedPtr m) { ... }` is a **lambda** — a function written
inline where it is needed instead of being declared separately. The `[this]` is
the **capture list**: it says the lambda may use the surrounding object's
members. That is how the callback can write to `pitch_` and `yaw_`.

## What does `spin()` do, and why does the program need it?

**Callbacks do not fire on their own.** Messages arrive and sit in a queue until
the program asks ROS to deliver them. `rclcpp::spin_some()` is that ask: *deliver
anything waiting, then return.*

This bit us for real. `hold()` originally slept without spinning, so refusals
arrived and sat unread, and the program looked like everything had succeeded.
Now every waiting loop spins:

```cpp
void hold(double seconds) {
    while (rclcpp::ok() && now < deadline) {
        spin();
        std::this_thread::sleep_for(20ms);
    }
}
```

`rclcpp::spin(node)` is the usual form — spin forever. We use `spin_some()`
because this program has its own sequence to run and only wants to collect
messages between steps.

## What are `rclcpp::init` and `rclcpp::shutdown`?

Start and stop the ROS layer. `init` before any node exists; `shutdown` at the
end. `main` also holds one second before shutting down, because shutting down
immediately after publishing throws away the last command's response — a real
bug this program had.

## What is `rclcpp::ok()`?

False once ROS is shutting down — Ctrl+C, for example. Every loop checks it so
the program stops promptly instead of finishing an eight-second dance first.

---

# Part 10 — The robot's own API

## How does one command actually reach the K1?

```cpp
booster_msgs::msg::RpcReqMsg request;
request.uuid   = "demo1-7";
request.header = "{\"api_id\":2004}";
request.body   = "{\"pitch\":0.100,\"yaw\":0.250}";
request_->publish(request);
```

The api id says which command; the body carries its parameters as JSON. That is
the entire interface. Every id is in the vendor header `b1_loco_api.hpp`:

| id | name | body |
|---|---|---|
| 2000 | `kChangeMode` | `{"mode":0..4}` |
| 2001 | `kMove` | `{"vx":..,"vy":..,"vyaw":..}` — **walks. Not in this demo.** |
| 2004 | `kRotateHead` | `{"pitch":..,"yaw":..}` |
| 2005 | `kWaveHand` | `{"hand_index":1,"hand_action":0 or 1}` |
| 2016 | `kDance` | `{"dance_id":0..7, or 1000 to stop}` |
| 2017 | `kGetMode` | empty; answers `{"mode":N}` |

## Why does the demo never make the robot walk?

`kMove` (2001) is deliberately absent. A room with students in it is no place
for a walking humanoid. Head and arms only.

## Why does `waitForHeadPose()` exist? Why not just command the head?

Because `kRotateHead` is **absolute**, not relative. It means "go to this angle",
not "move by this much". Command it before you know where the head currently is
and the head snaps from wherever it really is to your number, at full servo
speed.

So `Robot` subscribes to `/head_pose_stamped`, waits for one message, and starts
from the true pose. The v6 agent guards the same thing with a flag called
`head_cmd_seeded_` — this is not a demo-only precaution, it is how the real
system behaves.

## What is that quaternion maths in the head callback?

The robot reports orientation as a **quaternion** (`x, y, z, w`) — four numbers,
because three angles have a failure mode called gimbal lock. Those two lines
convert it to the pitch and yaw we actually want:

```cpp
pitch_ = std::asin(2*(w*y - z*x));
yaw_   = std::atan2(2*(w*z + x*y), 1 - 2*(y*y + z*z));
```

It is standard quaternion-to-Euler, copied from `agent_node.cpp`. You are not
expected to derive it. Know what it converts *from* and *to*, and that the
`fmax/fmin` clamp before `asin` exists because floating-point error can push the
value a hair past 1.0, where `asin` returns NaN.

## Why does `moveHead()` send fifty small commands instead of one?

One command to the target angle would be a jump at servo speed. Fifty steps at
20 ms is 1 second of smooth travel. That is all the "slew" is — interpolation:

```cpp
const float t = float(step) / steps;    // 0.0 -> 1.0
pitch = fromPitch + (pitch - fromPitch) * t;
```

## Why are `kWaveHand` and `kDance` start/stop? What happens if I forget the stop?

Neither ends by itself. `kWaveHand` with `hand_action:0` starts a **looping**
wave; `hand_action:1` stops it. `kDance` with `dance_id:6` starts a cheer;
`dance_id:1000` (`DanceId::kStop`) stops it — it is exactly what the **Stop**
button in the Booster iOS app sends.

If you forget the stop, that action stays live **forever**, and the robot then
refuses *every* later command — including head commands from a completely
different program. This is not hypothetical: it happened while building this
demo, and a run failed on the head because a *previous* run's dance had never
been stopped.

**That is why `Wave` and `Dance` each send their own stop.** The caller cannot
forget what the class already handles. It is the clearest argument for
encapsulation in this whole program, and it comes from a real bug rather than a
textbook.

## Why print the robot's refusals? What is a 501?

Every request is answered on `/LocoApiTopicResp` with the same uuid we sent.
`Robot` keeps a `uuid -> api id` map, matches the answer, and prints anything
non-zero.

`501` is `kRpcStatusCodeServerRefused` — "request rejected". It does **not** say
why. The reason is in the robot's own log:

```
grep -A3 "id: 2016" $(ls -td /var/log/booster/*/motion | head -1)/motion.log
```

which is what the second printed line hands you. On the day this demo was built
that command answered in two lines:

```
[mode_manager.cpp:807] action 5  conflict with current action
[mode_manager.cpp:814] conflict action and refused
```

**The status code names a category; the robot's log names the cause.** Read both.

## Why print each refusal only once?

`moveHead()` sends a command every 20 ms. When they are being refused, printing
each one produces sixty identical lines a second and buries everything else — it
did exactly that. Now each `(api, status)` pair prints once in full, repeats are
counted, and a summary appears at the end. `moveHead` also gives up as soon as
its own commands come back refused, instead of sending the remaining hundred it
already knows will fail.

**A diagnostic nobody can read is not better than silence.**

---

# Part 11 — Questions with sharp edges

## Could this have been done without any classes?

Yes, and the first version was. Five functions, each with its own copy of the
publish. It worked. It also meant a change to the topic name was a change in
five places, adding a behaviour meant touching the dispatch code, and forgetting
a wave's stop was a mistake each caller had to remember not to make.

Classes did not make the robot move. They made the fifth behaviour cost eight
lines instead of forty, and made one category of bug impossible.

## Is `Action` the base class or the parent class?

Both names are used, plus **superclass**. The subclasses are **derived classes**
or **child classes**. Same relationship, different vocabulary.

## Why is `Robot` not the base class of `Action`?

Because an action is not a kind of robot. Inheritance is for "is a"; here the
relationship is "an action **uses** a robot", which is what a member reference
expresses. Getting this wrong is the single most common mistake with
inheritance: reaching for it when you wanted "has a".

## What is composition, then?

Holding an object instead of inheriting from it. `Action` holds `Robot &robot_`
— that is composition. Rule of thumb: **prefer composition; use inheritance only
when "is a" is literally true and you need polymorphism.** This demo uses both,
each where it belongs.

## Why is `Robot` a class at all, rather than plain functions?

Because it holds **state** that the functions all need: the publisher, two
subscriptions, the last known head pose, the in-flight request map, the refusal
counts. Passing that around as arguments to free functions is how you end up
with nine-parameter functions. A class is a set of functions that share state.

## Does `#include "robot.h"` copy the whole file into `actions.cpp`?

Yes, textually, before compilation. That is exactly what `#include` does.

## Where is the executable after `colcon build`?

`install/demo1/lib/demo1/actions`. `ros2 run demo1 actions` finds it via
`AMENT_PREFIX_PATH`, which is what `source install/setup.bash` sets. You can also
run that path directly.

## VS Code says a member does not exist but it compiles fine

IntelliSense cached a stale copy of the header. **Close the file, then reset the
IntelliSense database, then reopen it** — in that order; resetting with the file
open does nothing, because the open editor immediately repopulates the cache.

`colcon build` is the only verdict that counts. If it compiles, the squiggle is
wrong.
