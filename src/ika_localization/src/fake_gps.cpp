// Test icin sahte NavSatFix yayinlayici (gercek Here3+ gelene kadar).
// EKF'in yerel tahminini WGS84'e cevirip kucuk gurultu ekleyerek yayinlar -
// boylece navsat_transform_node'un ve global EKF'in mantigi test edilebilir.
#include <chrono>
#include <cmath>
#include <random>
#include "rclcpp/rclcpp.hpp"
#include "sensor_msgs/msg/nav_sat_fix.hpp"
#include "nav_msgs/msg/odometry.hpp"

using namespace std::chrono_literals;
using std::placeholders::_1;

class FakeGps : public rclcpp::Node
{
public:
  FakeGps() : Node("fake_gps"), x_(0.0), y_(0.0), gen_(42), noise_(0.0, 0.35)
  {
    this->declare_parameter("origin_lat", 39.925101);
    this->declare_parameter("origin_lon", 32.836742);
    origin_lat_ = this->get_parameter("origin_lat").as_double();
    origin_lon_ = this->get_parameter("origin_lon").as_double();

    pub_ = this->create_publisher<sensor_msgs::msg::NavSatFix>("/gps/fix", 10);
    odom_sub_ = this->create_subscription<nav_msgs::msg::Odometry>(
      "/odometry/filtered", 10, std::bind(&FakeGps::onOdom, this, _1));
    timer_ = this->create_wall_timer(200ms, std::bind(&FakeGps::tick, this));
    RCLCPP_INFO(this->get_logger(), "fake_gps basladi (5 Hz, ~0.35m gurultu).");
  }

private:
  void onOdom(const nav_msgs::msg::Odometry::SharedPtr msg)
  {
    x_ = msg->pose.pose.position.x;
    y_ = msg->pose.pose.position.y;
  }

  void tick()
  {
    constexpr double R = 6371000.0;
    constexpr double DEG2RAD = M_PI / 180.0;
    double lat = origin_lat_ + ((y_ + noise_(gen_)) / R) / DEG2RAD;
    double lon = origin_lon_ + ((x_ + noise_(gen_)) / (R * std::cos(origin_lat_ * DEG2RAD))) / DEG2RAD;

    sensor_msgs::msg::NavSatFix fix;
    fix.header.stamp = this->get_clock()->now();
    fix.header.frame_id = "gps_link";
    fix.status.status = sensor_msgs::msg::NavSatStatus::STATUS_FIX;
    fix.status.service = sensor_msgs::msg::NavSatStatus::SERVICE_GPS;
    fix.latitude = lat;
    fix.longitude = lon;
    fix.altitude = 0.0;
    fix.position_covariance[0] = 0.5;
    fix.position_covariance[4] = 0.5;
    fix.position_covariance[8] = 1.0;
    fix.position_covariance_type = sensor_msgs::msg::NavSatFix::COVARIANCE_TYPE_DIAGONAL_KNOWN;
    pub_->publish(fix);
  }

  double x_, y_, origin_lat_, origin_lon_;
  std::mt19937 gen_;
  std::normal_distribution<double> noise_;
  rclcpp::Publisher<sensor_msgs::msg::NavSatFix>::SharedPtr pub_;
  rclcpp::Subscription<nav_msgs::msg::Odometry>::SharedPtr odom_sub_;
  rclcpp::TimerBase::SharedPtr timer_;
};

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<FakeGps>());
  rclcpp::shutdown();
  return 0;
}
