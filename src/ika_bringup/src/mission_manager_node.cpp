// Gun3 gorev akisi: BEKLEME -> HAZIR -> GOREV_AKTIF -> TAMAMLANDI
#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/bool.hpp"
#include "std_msgs/msg/string.hpp"
#include "nav_msgs/msg/path.hpp"

using std::placeholders::_1;
enum class MissionState { BEKLEME, HAZIR, GOREV_AKTIF, TAMAMLANDI };

class MissionManagerNode : public rclcpp::Node
{
public:
  MissionManagerNode() : Node("mission_manager_node"),
    state_(MissionState::BEKLEME), have_route_(false), have_touchdown_(false)
  {
    touchdown_sub_ = this->create_subscription<std_msgs::msg::Bool>(
      "/iha/touchdown_status", 10, std::bind(&MissionManagerNode::onTouchdown, this, _1));
    route_sub_ = this->create_subscription<nav_msgs::msg::Path>(
      "/route/waypoints", 10, std::bind(&MissionManagerNode::onRoute, this, _1));
    state_pub_ = this->create_publisher<std_msgs::msg::String>("/mission/state", 10);
    timer_ = this->create_wall_timer(
      std::chrono::milliseconds(500), std::bind(&MissionManagerNode::publishState, this));
    RCLCPP_INFO(this->get_logger(), "mission_manager_node basladi. Durum: BEKLEME.");
  }

private:
  void onTouchdown(const std_msgs::msg::Bool::SharedPtr msg) { have_touchdown_ = msg->data; checkReady(); }
  void onRoute(const nav_msgs::msg::Path::SharedPtr msg)
  {
    if (!msg->poses.empty()) { have_route_ = true; checkReady(); }
  }

  void checkReady()
  {
    if (have_route_ && have_touchdown_ && state_ == MissionState::BEKLEME) {
      state_ = MissionState::HAZIR;
      RCLCPP_INFO(this->get_logger(), "Guzergah+touchdown hazir. Durum: HAZIR.");
      startMission();
    }
  }

  void startMission()
  {
    state_ = MissionState::GOREV_AKTIF;
    RCLCPP_INFO(this->get_logger(), "Durum: GOREV_AKTIF.");
    RCLCPP_WARN(this->get_logger(),
      "STUB: Nav2 action client henuz baglanmadi (Gun9 isi) - hedef gonderilmedi.");
  }

  void publishState()
  {
    std_msgs::msg::String s;
    switch (state_) {
      case MissionState::BEKLEME: s.data = "BEKLEME"; break;
      case MissionState::HAZIR: s.data = "HAZIR"; break;
      case MissionState::GOREV_AKTIF: s.data = "GOREV_AKTIF"; break;
      case MissionState::TAMAMLANDI: s.data = "TAMAMLANDI"; break;
    }
    state_pub_->publish(s);
  }

  MissionState state_;
  bool have_route_, have_touchdown_;
  rclcpp::Subscription<std_msgs::msg::Bool>::SharedPtr touchdown_sub_;
  rclcpp::Subscription<nav_msgs::msg::Path>::SharedPtr route_sub_;
  rclcpp::Publisher<std_msgs::msg::String>::SharedPtr state_pub_;
  rclcpp::TimerBase::SharedPtr timer_;
};

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<MissionManagerNode>());
  rclcpp::shutdown();
  return 0;
}
