// Gun4 mantik semasi: touchdown onayi gelmeden motor komutlari bloklanir.
#include "rclcpp/rclcpp.hpp"
#include "geometry_msgs/msg/twist.hpp"
#include "std_msgs/msg/bool.hpp"

using std::placeholders::_1;

class InterlockNode : public rclcpp::Node
{
public:
  InterlockNode() : Node("interlock_node"), unlocked_(false)
  {
    cmd_sub_ = this->create_subscription<geometry_msgs::msg::Twist>(
      "/cmd_vel_nav", 10, std::bind(&InterlockNode::onCmd, this, _1));
    touchdown_sub_ = this->create_subscription<std_msgs::msg::Bool>(
      "/iha/touchdown_status", 10, std::bind(&InterlockNode::onTouchdown, this, _1));
    cmd_pub_ = this->create_publisher<geometry_msgs::msg::Twist>("/cmd_vel", 10);
    RCLCPP_INFO(this->get_logger(), "interlock_node basladi. Durum: KILITLI.");
  }

private:
  void onTouchdown(const std_msgs::msg::Bool::SharedPtr msg)
  {
    if (msg->data && !unlocked_) {
      unlocked_ = true;
      RCLCPP_INFO(this->get_logger(), "Touchdown onaylandi. Durum: ACIK.");
    }
  }

  void onCmd(const geometry_msgs::msg::Twist::SharedPtr msg)
  {
    if (unlocked_) {
      cmd_pub_->publish(*msg);
    } else {
      geometry_msgs::msg::Twist zero;
      cmd_pub_->publish(zero);
      RCLCPP_WARN_THROTTLE(this->get_logger(), *this->get_clock(), 2000,
        "Istenmeyen komut reddedildi - interlock KILITLI.");
    }
  }

  bool unlocked_;
  rclcpp::Subscription<geometry_msgs::msg::Twist>::SharedPtr cmd_sub_;
  rclcpp::Subscription<std_msgs::msg::Bool>::SharedPtr touchdown_sub_;
  rclcpp::Publisher<geometry_msgs::msg::Twist>::SharedPtr cmd_pub_;
};

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<InterlockNode>());
  rclcpp::shutdown();
  return 0;
}
