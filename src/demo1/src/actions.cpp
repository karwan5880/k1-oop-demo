#include "robot.h"
#include <iostream>
#include <vector>

class Action {
public:
    Action(Robot &robot, std::string name)
        : robot_(robot), name_(name) {
        std::cout << "[built] " << name_ << "\n";
    }

    virtual ~Action() {}

    void perform() {
        std::cout << "-> " << name_ << "\n";
        play();
    }

protected:
    virtual void play() = 0;

    Robot &robot_;
    std::string name_;
};

class LookAt : public Action {
public:
    LookAt(Robot &robot, float yaw)
        : Action(robot, "look"), yaw_(yaw) {}

protected:
    void play() override { robot_.moveHead(robot_.headPitch(), yaw_, 1.0); }

private:
    float yaw_;
};

class Nod : public Action {
public:
    Nod(Robot &robot, int times)
        : Action(robot, "nod"), times_(times) {}

protected:
    void play() override {
        for (int i = 0; i < times_; ++i) {
            robot_.moveHead(robot_.headPitch() + 0.25f, robot_.headYaw(), 0.4);
            robot_.moveHead(robot_.headPitch() - 0.25f, robot_.headYaw(), 0.4);
        }
    }

private:
    int times_;
};

class Wave : public Action {
public:
    Wave(Robot &robot, double seconds)
        : Action(robot, "wave"), seconds_(seconds) {}

protected:
    void play() override {
        robot_.callLocoApi(Robot::kWaveHand, "{\"hand_index\":1,\"hand_action\":0}");
        robot_.hold(seconds_);
        robot_.callLocoApi(Robot::kWaveHand, "{\"hand_index\":1,\"hand_action\":1}");
        robot_.hold(3.0);
    }

private:
    double seconds_;
};

class Dance : public Action {
public:
    Dance(Robot &robot, int danceId, double seconds)
        : Action(robot, "dance"), danceId_(danceId), seconds_(seconds) {}

protected:
    void play() override {
        robot_.callLocoApi(Robot::kDance,
                           "{\"dance_id\":" + std::to_string(danceId_) + "}");
        robot_.hold(seconds_);
        robot_.callLocoApi(Robot::kDance,
                           "{\"dance_id\":" + std::to_string(Robot::kDanceStop) + "}");
        robot_.hold(3.0);
    }

private:
    int danceId_;
    double seconds_;
};

int main(int argc, char **argv) {
    rclcpp::init(argc, argv);
    auto robot = std::make_shared<Robot>();

    if (!robot->waitForHeadPose()) {
        std::cerr << "no /head_pose_stamped -- is the robot up?\n";
        return 1;
    }

    std::vector<Action *> lesson;
    lesson.push_back(new LookAt(*robot, 0.5f));
    lesson.push_back(new Nod(*robot, 2));
    lesson.push_back(new LookAt(*robot, -0.5f));
    lesson.push_back(new Wave(*robot, 3.0));
    lesson.push_back(new Dance(*robot, 6, 8.0));

    for (Action *action : lesson)
        action->perform();

    robot->hold(1.0);
    robot->printRefusalSummary();

    for (Action *action : lesson) delete action;
    rclcpp::shutdown();
    return 0;
}
