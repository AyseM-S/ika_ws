// 6 tekerlekli skid-steer IKA icin standart diferansiyel odometri.
// Rocker-bogie SADECE suspansiyon (dikey uyum), donus sol/sag hiz farkiyla.
#include <cmath>
#include <memory>
#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/float64.hpp"
#include "nav_msgs/msg/odometry.hpp"

using std::placeholders::_1;

class WheelOdomNode : public rclcpp::Node
{
public:
  WheelOdomNode() : Node("wheel_odom_node"),
    theta_(0.0), x_(0.0), y_(0.0), v_left_(0.0), v_right_(0.0),
    have_left_(false), have_right_(false)
  {
    this->declare_parameter("wheel_radius", 0.065);
    this->declare_parameter("track_width", 0.0);
    this->declare_parameter("encoder_ppr", 0.0);
    this->declare_parameter("publish_rate_hz", 30.0);

    track_width_ = this->get_parameter("track_width").as_double();
    if (track_width_ <= 0.0) {
      RCLCPP_WARN(this->get_logger(),
        "track_width HENUZ GIRILMEDI (vehicle_params.yaml) - odometri YANLIS olacak!");
    }

    // Not: encoder_ppr burada degil, ham enkoder->hiz donusumunu yapan
    // ayri bir surucu katmaninda kullanilacak (donanim gelince eklenir).
    // Bu node, zaten m/s'ye cevrilmis sol/sag hizi tuketir.

    left_sub_ = this->create_subscription<std_msgs::msg::Float64>(
      "/wheel/left_speed", 10, std::bind(&WheelOdomNode::onLeft, this, _1));
    right_sub_ = this->create_subscription<std_msgs::msg::Float64>(
      "/wheel/right_speed", 10, std::bind(&WheelOdomNode::onRight, this, _1));
    odom_pub_ = this->create_publisher<nav_msgs::msg::Odometry>("/wheel/odom", 10);

    double rate = this->get_parameter("publish_rate_hz").as_double();
    last_time_ = this->get_clock()->now();
    timer_ = this->create_wall_timer(
      std::chrono::duration<double>(1.0 / rate), std::bind(&WheelOdomNode::tick, this));

    RCLCPP_INFO(this->get_logger(), "wheel_odom_node basladi. track_width=%.3f", track_width_);
  }

private:
  void onLeft(const std_msgs::msg::Float64::SharedPtr msg) { v_left_ = msg->data; have_left_ = true; }
  void onRight(const std_msgs::msg::Float64::SharedPtr msg) { v_right_ = msg->data; have_right_ = true; }

  void tick()
  {
    if (!have_left_ || !have_right_) return;
    auto now = this->get_clock()->now();
    double dt = (now - last_time_).seconds();
    last_time_ = now;
    if (dt <= 0.0 || dt > 0.5) return;

    // --- Standart skid-steer / diferansiyel surus odometrisi ---
    double v = (v_left_ + v_right_) / 2.0;
    double omega = (track_width_ > 1e-6) ? (v_right_ - v_left_) / track_width_ : 0.0;

    theta_ += omega * dt;
    x_ += v * std::cos(theta_) * dt;
    y_ += v * std::sin(theta_) * dt;

    nav_msgs::msg::Odometry odom;
    odom.header.stamp = now;
    odom.header.frame_id = "odom";
    odom.child_frame_id = "base_link";
    odom.pose.pose.position.x = x_;
    odom.pose.pose.position.y = y_;
    odom.pose.pose.orientation.z = std::sin(theta_ / 2.0);
    odom.pose.pose.orientation.w = std::cos(theta_ / 2.0);
    odom.twist.twist.linear.x = v;
    odom.twist.twist.angular.z = omega;
    odom.twist.covariance[0] = 0.02;   // sadece hiza guven (yaw IMU'dan gelecek)
    odom_pub_->publish(odom);
  }

  double theta_, x_, y_, v_left_, v_right_, track_width_;
  bool have_left_, have_right_;
  rclcpp::Time last_time_;
  rclcpp::Subscription<std_msgs::msg::Float64>::SharedPtr left_sub_, right_sub_;
  rclcpp::Publisher<nav_msgs::msg::Odometry>::SharedPtr odom_pub_;
  rclcpp::TimerBase::SharedPtr timer_;
};

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<WheelOdomNode>());
  rclcpp::shutdown();
  return 0;
}
