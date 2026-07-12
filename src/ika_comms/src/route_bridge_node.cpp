// Gun1 ICD'sindeki route_update/route_ack UDP koprusu.
#include <sstream>
#include <iomanip>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "rclcpp/rclcpp.hpp"
#include "nav_msgs/msg/path.hpp"
#include "geometry_msgs/msg/pose_stamped.hpp"
#include "nlohmann/json.hpp"
#include "ika_comms/checksum.hpp"

using json = nlohmann::json;

class RouteBridgeNode : public rclcpp::Node
{
public:
  RouteBridgeNode() : Node("route_bridge_node")
  {
    this->declare_parameter("listen_port", 5005);
    this->declare_parameter("iha_ip", std::string("127.0.0.1"));
    this->declare_parameter("iha_port", 5006);
    this->declare_parameter("yki_ip", std::string("127.0.0.1"));
    this->declare_parameter("yki_port", 5007);

    listen_port_ = this->get_parameter("listen_port").as_int();
    iha_ip_ = this->get_parameter("iha_ip").as_string();
    iha_port_ = this->get_parameter("iha_port").as_int();
    yki_ip_ = this->get_parameter("yki_ip").as_string();
    yki_port_ = this->get_parameter("yki_port").as_int();

    path_pub_ = this->create_publisher<nav_msgs::msg::Path>("/route/waypoints", 10);
    setupSocket();
    timer_ = this->create_wall_timer(
      std::chrono::milliseconds(50), std::bind(&RouteBridgeNode::pollSocket, this));
    RCLCPP_INFO(this->get_logger(), "route_bridge_node basladi, port %d dinleniyor.", listen_port_);
  }
  ~RouteBridgeNode() { if (sock_fd_ >= 0) close(sock_fd_); }

private:
  void setupSocket()
  {
    sock_fd_ = socket(AF_INET, SOCK_DGRAM, 0);
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(listen_port_);
    bind(sock_fd_, (struct sockaddr*)&addr, sizeof(addr));
  }

  void pollSocket()
  {
    char buf[4096];
    sockaddr_in from{}; socklen_t fromlen = sizeof(from);
    ssize_t n = recvfrom(sock_fd_, buf, sizeof(buf) - 1, MSG_DONTWAIT, (struct sockaddr*)&from, &fromlen);
    if (n <= 0) return;
    buf[n] = '\0';

    try {
      json msg = json::parse(std::string(buf, n));
      if (msg.value("msg_type", "") != "route_update") return;

      std::string received = msg.value("checksum", "");
      json route = msg["route"];
      std::string computed = toHex(ika_comms::crc32(route.dump()));

      if (computed != received) {
        RCLCPP_WARN(this->get_logger(), "Checksum uyusmadi (beklenen=%s gelen=%s) - REDDEDILDI.",
          computed.c_str(), received.c_str());
        return;
      }
      publishPath(route);
      sendAck(msg.value("msg_id", ""));
      RCLCPP_INFO(this->get_logger(), "route_update dogrulandi, ack gonderildi.");
    } catch (const std::exception & e) {
      RCLCPP_WARN(this->get_logger(), "JSON parse hatasi: %s", e.what());
    }
  }

  void publishPath(const json & route)
  {
    nav_msgs::msg::Path path;
    path.header.stamp = this->get_clock()->now();
    path.header.frame_id = "odom";
    for (const auto & wp : route["waypoints"]) {
      geometry_msgs::msg::PoseStamped ps;
      ps.header = path.header;
      ps.pose.position.x = wp.value("x", 0.0);
      ps.pose.position.y = wp.value("y", 0.0);
      ps.pose.orientation.w = 1.0;
      path.poses.push_back(ps);
    }
    path_pub_->publish(path);
  }

  void sendAck(const std::string & ref_id)
  {
    json ack;
    ack["msg_type"] = "route_ack";
    ack["ref_msg_id"] = ref_id;
    ack["received_by"] = "IKA-01";
    ack["status"] = "accepted";
    std::string payload = ack.dump();
    sendTo(iha_ip_, iha_port_, payload);
    sendTo(yki_ip_, yki_port_, payload);
  }

  void sendTo(const std::string & ip, int port, const std::string & payload)
  {
    sockaddr_in dest{};
    dest.sin_family = AF_INET;
    dest.sin_port = htons(port);
    inet_pton(AF_INET, ip.c_str(), &dest.sin_addr);
    sendto(sock_fd_, payload.c_str(), payload.size(), 0, (struct sockaddr*)&dest, sizeof(dest));
  }

  std::string toHex(uint32_t v)
  {
    std::ostringstream oss;
    oss << std::hex << std::setw(8) << std::setfill('0') << v;
    return oss.str();
  }

  int sock_fd_{-1}, listen_port_, iha_port_, yki_port_;
  std::string iha_ip_, yki_ip_;
  rclcpp::Publisher<nav_msgs::msg::Path>::SharedPtr path_pub_;
  rclcpp::TimerBase::SharedPtr timer_;
};

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<RouteBridgeNode>());
  rclcpp::shutdown();
  return 0;
}
