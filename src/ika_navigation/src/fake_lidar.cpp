// Test icin "temiz" sahte /scan - engel YOK (max menzil her yonde).
// ONEMLI: Onceki versiyon frame_id=base_link ile sabit "1.5m onde engel"
// yayinliyordu - bu, robotla birlikte hareket eden hayali bir engel
// yaratip path-following testini bozuyordu. Gercek engelden kacinma
// testi (statik, dunya-sabit engelle) AYRI bir is olarak planlanmali.
#include <chrono>
#include <cmath>
#include "rclcpp/rclcpp.hpp"
#include "sensor_msgs/msg/laser_scan.hpp"

using namespace std::chrono_literals;

class FakeLidar : public rclcpp::Node
{
public:
  FakeLidar() : Node("fake_lidar")
  {
    pub_ = this->create_publisher<sensor_msgs::msg::LaserScan>("/scan", 10);
    timer_ = this->create_wall_timer(100ms, std::bind(&FakeLidar::tick, this));
    RCLCPP_INFO(this->get_logger(), "fake_lidar basladi (10 Hz, ENGELSIZ - path-following testi icin).");
  }

private:
  void tick()
  {
    sensor_msgs::msg::LaserScan scan;
    scan.header.stamp = this->get_clock()->now();
    scan.header.frame_id = "base_link";
    scan.angle_min = -M_PI;
    scan.angle_max = M_PI;
    scan.angle_increment = M_PI / 180.0;
    scan.range_min = 0.15;
    scan.range_max = 12.0;
    scan.scan_time = 0.1;
    int n = static_cast<int>((scan.angle_max - scan.angle_min) / scan.angle_increment) + 1;
    scan.ranges.assign(n, scan.range_max);  // hepsi max menzil = engel yok
    pub_->publish(scan);
  }

  rclcpp::Publisher<sensor_msgs::msg::LaserScan>::SharedPtr pub_;
  rclcpp::TimerBase::SharedPtr timer_;
};

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<FakeLidar>());
  rclcpp::shutdown();
  return 0;
}
