#include <chrono>
#include <cmath>
#include <memory>
#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/float64.hpp"
#include "sensor_msgs/msg/imu.hpp"

using namespace std::chrono_literals;

class FakeSensors : public rclcpp::Node
{
public:
  FakeSensors() : Node("fake_sensors"), t_(0.0)
  {
    left_pub_ = this->create_publisher<std_msgs::msg::Float64>("/wheel/left_speed", 10);
    right_pub_ = this->create_publisher<std_msgs::msg::Float64>("/wheel/right_speed", 10);
    imu_pub_ = this->create_publisher<sensor_msgs::msg::Imu>("/imu/data", 10);
    timer_ = this->create_wall_timer(33ms, std::bind(&FakeSensors::tick, this));
    RCLCPP_INFO(this->get_logger(), "Skid-steer sahte sensor yayinlayici basladi.");
  }

private:
  void tick()
  {
    t_ += 0.033;
    auto now = this->get_clock()->now();
    double v_left = 0.28;
    double v_right = 0.32;

    std_msgs::msg::Float64 lmsg; lmsg.data = v_left; left_pub_->publish(lmsg);
    std_msgs::msg::Float64 rmsg; rmsg.data = v_right; right_pub_->publish(rmsg);

    sensor_msgs::msg::Imu im;
    im.header.stamp = now;
    im.header.frame_id = "imu_link";
    im.angular_velocity.z = 0.08;
    double yaw = 0.08 * t_;
    im.orientation.z = std::sin(yaw / 2.0);
    im.orientation.w = std::cos(yaw / 2.0);
    im.orientation_covariance[8] = 0.01;
    im.angular_velocity_covariance[8] = 0.01;
    imu_pub_->publish(im);
  }

  rclcpp::Publisher<std_msgs::msg::Float64>::SharedPtr left_pub_, right_pub_;
  rclcpp::Publisher<sensor_msgs::msg::Imu>::SharedPtr imu_pub_;
  rclcpp::TimerBase::SharedPtr timer_;
  double t_;
};

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<FakeSensors>());
  rclcpp::shutdown();
  return 0;
}
