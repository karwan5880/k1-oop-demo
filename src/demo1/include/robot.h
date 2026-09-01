#pragma once

#include <chrono>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <thread>
#include <utility>

#include <rclcpp/rclcpp.hpp>
#include <geometry_msgs/msg/pose_stamped.hpp>
#include <booster_msgs/msg/rpc_req_msg.hpp>
#include <booster_msgs/msg/rpc_resp_msg.hpp>

class Robot : public rclcpp::Node {
public:
    Robot() : rclcpp::Node("demo1") {
        request_ = create_publisher<booster_msgs::msg::RpcReqMsg>("LocoApiTopicReq", 10);
        headPose_ = create_subscription<geometry_msgs::msg::PoseStamped>(
            "/head_pose_stamped", 10,
            [this](const geometry_msgs::msg::PoseStamped::SharedPtr m) {
                const auto &q = m->pose.orientation;
                float sp = 2.0f * float(q.w * q.y - q.z * q.x);
                sp = std::fmax(-1.0f, std::fmin(1.0f, sp));
                pitch_ = std::asin(sp);
                yaw_ = std::atan2(2.0f * float(q.w * q.z + q.x * q.y),
                                  1.0f - 2.0f * float(q.y * q.y + q.z * q.z));
                headPoseKnown_ = true;
            });
        response_ = create_subscription<booster_msgs::msg::RpcRespMsg>(
            "/LocoApiTopicResp", 20,
            [this](const booster_msgs::msg::RpcRespMsg::SharedPtr m) {
                auto it = inFlight_.find(m->uuid);
                if (it == inFlight_.end()) return;
                const int apiId = it->second;
                inFlight_.erase(it);
                const int status = statusOf(m->header);
                if (status == 0) return;
                refusalCount_++;
                if (++refusalsByApi_[{apiId, status}] > 1) return;
                std::cerr << "[robot] api " << apiId << " REFUSED status=" << status
                          << " (" << statusMeaning(status) << ")\n";
                std::cerr << "     why: ssh booster-robot "
                             "\"grep -A3 'id: " << apiId
                          << "' \\$(ls -td /var/log/booster/*/motion | head -1)"
                             "/motion.log | tail -20\"\n";
            });
    }

    bool waitForHeadPose(double timeoutSeconds = 5.0) {
        const auto deadline = std::chrono::steady_clock::now()
                            + std::chrono::milliseconds(int(timeoutSeconds * 1000));
        while (rclcpp::ok() && !headPoseKnown_
               && std::chrono::steady_clock::now() < deadline) {
            spin();
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }
        return headPoseKnown_;
    }

    void callLocoApi(int apiId, const std::string &body) {
        static uint64_t sequence = 0;
        const std::string uuid = "demo1-" + std::to_string(sequence++);
        inFlight_[uuid] = apiId;
        booster_msgs::msg::RpcReqMsg request;
        request.uuid = uuid;
        request.header = "{\"api_id\":" + std::to_string(apiId) + "}";
        request.body = body;
        request_->publish(request);
    }

    void moveHead(float pitch, float yaw, double seconds = 1.2) {
        const int steps = std::max(1, int(seconds * 50));
        const int refusalsBefore = refusalCount_;
        const float fromPitch = pitch_, fromYaw = yaw_;
        for (int step = 1; step <= steps && rclcpp::ok(); ++step) {
            if (refusalCount_ > refusalsBefore) return;
            const float t = float(step) / steps;
            char body[96];
            std::snprintf(body, sizeof(body), "{\"pitch\":%.3f,\"yaw\":%.3f}",
                          fromPitch + (pitch - fromPitch) * t,
                          fromYaw + (yaw - fromYaw) * t);
            callLocoApi(kRotateHead, body);
            spin();
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }
        pitch_ = pitch;
        yaw_ = yaw;
    }

    void hold(double seconds) {
        const auto deadline = std::chrono::steady_clock::now()
                            + std::chrono::milliseconds(int(seconds * 1000));
        while (rclcpp::ok() && std::chrono::steady_clock::now() < deadline) {
            spin();
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }
    }

    void spin() { rclcpp::spin_some(get_node_base_interface()); }

    float headPitch() const { return pitch_; }
    float headYaw() const { return yaw_; }

    void printRefusalSummary() {
        spin();
        if (refusalsByApi_.empty()) return;
        std::cerr << "\n[robot] refusal summary\n";
        for (const auto &entry : refusalsByApi_)
            std::cerr << "     api " << entry.first.first << "  status "
                      << entry.first.second << "  x" << entry.second << "\n";
        std::cerr << "     the robot refuses everything while another action is "
                     "still playing.\n"
                     "     wait for it to finish, then run again.\n";
    }

    static constexpr int kRotateHead = 2004;
    static constexpr int kWaveHand = 2005;
    static constexpr int kDance = 2016;
    static constexpr int kDanceStop = 1000;

private:
    static int statusOf(const std::string &header) {
        const auto key = header.find("\"status\"");
        if (key == std::string::npos) return 0;
        const auto colon = header.find(':', key);
        if (colon == std::string::npos) return 0;
        return std::atoi(header.c_str() + colon + 1);
    }

    static const char *statusMeaning(int status) {
        switch (status) {
            case 400: return "bad request";
            case 500: return "internal server error";
            case 501: return "server refused - busy, wrong mode, or low battery";
            case 502: return "state transition failed";
            default:  return "see booster/robot/rpc/error.hpp";
        }
    }

    rclcpp::Publisher<booster_msgs::msg::RpcReqMsg>::SharedPtr request_;
    rclcpp::Subscription<geometry_msgs::msg::PoseStamped>::SharedPtr headPose_;
    rclcpp::Subscription<booster_msgs::msg::RpcRespMsg>::SharedPtr response_;
    std::map<std::string, int> inFlight_;
    std::map<std::pair<int, int>, int> refusalsByApi_;
    int refusalCount_ = 0;
    float pitch_ = 0.0f, yaw_ = 0.0f;
    bool headPoseKnown_ = false;
};
