// Kapali-dongu sahte sensor: Nav2'nin gonderdigi /cmd_vel komutuna
// "ideal" (gecikmesiz, mukemmel takip eden) tekerlek+IMU yaniti uretir.
// Amac: ust katmanlarin (EKF+Nav2+comms+interlock) mantigini uctan uca
// test etmek. Gercek motor dinamigini/gecikmesini MODELLEMEZ - bilincli
// basitlestirme (Gun8'deki acik-donguden farki budur, o testler de
// ayrica gecerliligini korur).
#include <chrono>
#include <cmath>
#include <memory>
#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/float64.hpp"
#include "sensor_msgs/msg/imu.hpp"
#include "geometry_msgs/msg/twist.hpp"

using namespace std::chrono_literals;
using std::placeholders::_1;

class FakeSensors : public rclcpp::Node
{
public:
  FakeSensors() : Node("fake_sensors"), cmd_v_(0.0), cmd_omega_(0.0), yaw_(0.0)
  {
    this->declare_parameter("track_width", 0.40);
    track_width_ = this->get_parameter("track_width").as_double();

    left_pub_ = this->create_publisher<std_msgs::msg::Float64>("/wheel/left_speed", 10);
    right_pub_ = this->create_publisher<std_msgs::msg::Float64>("/wheel/right_speed", 10);
    imu_pub_ = this->create_publisher<sensor_msgs::msg::Imu>("/imu/data", 10);
    cmd_sub_ = this->create_subscription<geometry_msgs::msg::Twist>(
      "/cmd_vel", 10, std::bind(&FakeSensors::onCmd, this, _1));

    timer_ = this->create_wall_timer(33ms, std::bind(&FakeSensors::tick, this));
    RCLCPP_INFO(this->get_logger(),
      "Kapali-dongu sahte sensor basladi (/cmd_vel dinleniyor, track_width=%.2f).", track_width_);
  }

private:
  void onCmd(const geometry_msgs::msg::Twist::SharedPtr msg)
  {
    cmd_v_ = msg->linear.x;
    cmd_omega_ = msg->angular.z;
  }

  void tick()
  {
    auto now = this->get_clock()->now();
    double dt = 0.033;
    yaw_ += cmd_omega_ * dt;

    // Komut edilen v,omega'dan "ideal" sol/sag tekerlek hizi (ters kinematik)
    double v_left = cmd_v_ - (cmd_omega_ * track_width_ / 2.0);
    double v_right = cmd_v_ + (cmd_omega_ * track_width_ / 2.0);

    std_msgs::msg::Float64 lmsg; lmsg.data = v_left; left_pub_->publish(lmsg);
    std_msgs::msg::Float64 rmsg; rmsg.data = v_right; right_pub_->publish(rmsg);

    sensor_msgs::msg::Imu im;
    im.header.stamp = now;
    im.header.frame_id = "imu_link";
    im.angular_velocity.z = cmd_omega_;
    im.orientation.z = std::sin(yaw_ / 2.0);
    im.orientation.w = std::cos(yaw_ / 2.0);
    im.orientation_covariance[8] = 0.01;
    im.angular_velocity_covariance[8] = 0.01;
    imu_pub_->publish(im);
  }

  rclcpp::Publisher<std_msgs::msg::Float64>::SharedPtr left_pub_, right_pub_;
  rclcpp::Publisher<sensor_msgs::msg::Imu>::SharedPtr imu_pub_;
  rclcpp::Subscription<geometry_msgs::msg::Twist>::SharedPtr cmd_sub_;
  rclcpp::TimerBase::SharedPtr timer_;
  double cmd_v_, cmd_omega_, yaw_, track_width_;
};

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<FakeSensors>());
  rclcpp::shutdown();
  return 0;
}
