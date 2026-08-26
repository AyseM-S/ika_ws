// RViz'in RobotModel eklentisinde tespit edilen render sorununa (ortama
// ozgu, nedeni tam teshis edilemedi) pragmatik cozum: URDF gorsellerinin
// AYNISINI Marker mesajlariyla ciziyoruz. tf_static zaten dogru (test
// edildi), her marker kendi link frame_id'sine baglaniyor - RViz otomatik
// dogru konuma yerlestiriyor.
#include <chrono>
#include "rclcpp/rclcpp.hpp"
#include "visualization_msgs/msg/marker_array.hpp"

using namespace std::chrono_literals;

class MarkerRobotViz : public rclcpp::Node
{
public:
  MarkerRobotViz() : Node("marker_robot_viz")
  {
    pub_ = this->create_publisher<visualization_msgs::msg::MarkerArray>("/robot_markers", 10);
    timer_ = this->create_wall_timer(200ms, std::bind(&MarkerRobotViz::tick, this));
    RCLCPP_INFO(this->get_logger(), "marker_robot_viz basladi - /robot_markers yayinlaniyor.");
  }

private:
  visualization_msgs::msg::Marker makeBox(const std::string & frame, int id,
      double sx, double sy, double sz, float r, float g, float b)
  {
    visualization_msgs::msg::Marker m;
    m.header.frame_id = frame;
    m.header.stamp = rclcpp::Time(0);
    m.ns = "ika"; m.id = id;
    m.type = visualization_msgs::msg::Marker::CUBE;
    m.action = visualization_msgs::msg::Marker::ADD;
    m.pose.orientation.w = 1.0;
    m.scale.x = sx; m.scale.y = sy; m.scale.z = sz;
    m.color.r = r; m.color.g = g; m.color.b = b; m.color.a = 1.0;
    m.lifetime = rclcpp::Duration(0, 500000000);
    return m;
  }

  visualization_msgs::msg::Marker makeCyl(const std::string & frame, int id,
      double radius, double length, float r, float g, float b)
  {
    visualization_msgs::msg::Marker m;
    m.header.frame_id = frame;
    m.header.stamp = rclcpp::Time(0);
    m.ns = "ika"; m.id = id;
    m.type = visualization_msgs::msg::Marker::CYLINDER;
    m.action = visualization_msgs::msg::Marker::ADD;
    m.pose.orientation.w = 1.0;
    m.scale.x = radius * 2; m.scale.y = radius * 2; m.scale.z = length;
    m.color.r = r; m.color.g = g; m.color.b = b; m.color.a = 1.0;
    m.lifetime = rclcpp::Duration(0, 500000000);
    return m;
  }

  void tick()
  {
    visualization_msgs::msg::MarkerArray arr;
    arr.markers.push_back(makeBox("base_link", 0, 0.5, 0.35, 0.15, 0.5f, 0.5f, 0.5f));
    arr.markers.push_back(makeBox("imu_link", 1, 0.03, 0.03, 0.01, 0.9f, 0.4f, 0.1f));
    arr.markers.push_back(makeCyl("laser_link", 2, 0.04, 0.05, 0.1f, 0.1f, 0.1f));
    arr.markers.push_back(makeCyl("wheel_fl", 3, 0.065, 0.05, 0.05f, 0.05f, 0.05f));
    arr.markers.push_back(makeCyl("wheel_fr", 4, 0.065, 0.05, 0.05f, 0.05f, 0.05f));
    arr.markers.push_back(makeCyl("wheel_ml", 5, 0.065, 0.05, 0.05f, 0.05f, 0.05f));
    arr.markers.push_back(makeCyl("wheel_mr", 6, 0.065, 0.05, 0.05f, 0.05f, 0.05f));
    arr.markers.push_back(makeCyl("wheel_rl", 7, 0.065, 0.05, 0.05f, 0.05f, 0.05f));
    arr.markers.push_back(makeCyl("wheel_rr", 8, 0.065, 0.05, 0.05f, 0.05f, 0.05f));
    pub_->publish(arr);
  }

  rclcpp::Publisher<visualization_msgs::msg::MarkerArray>::SharedPtr pub_;
  rclcpp::TimerBase::SharedPtr timer_;
};

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<MarkerRobotViz>());
  rclcpp::shutdown();
  return 0;
}
