// ICD v2 (WGS84 + iha_state + zengin status enum) UDP koprusu.
#include <sstream>
#include <iomanip>
#include <ctime>
#include <chrono>
#include <cstring>
#include <cerrno>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "rclcpp/rclcpp.hpp"
#include "nav_msgs/msg/path.hpp"
#include "geometry_msgs/msg/pose_stamped.hpp"
#include "nlohmann/json.hpp"
#include "ika_comms/checksum.hpp"
#include "ika_comms/geo_utils.hpp"

using json = nlohmann::json;

class RouteBridgeNode : public rclcpp::Node
{
public:
  RouteBridgeNode() : Node("route_bridge_node"), last_seq_no_(-1), ack_counter_(0)
  {
    this->declare_parameter("listen_port", 5005);
    this->declare_parameter("iha_ip", std::string("127.0.0.1"));
    this->declare_parameter("iha_port", 5006);
    this->declare_parameter("yki_ip", std::string("127.0.0.1"));
    this->declare_parameter("yki_port", 5007);
    this->declare_parameter("local_origin_lat", 0.0);
    this->declare_parameter("local_origin_lon", 0.0);

    listen_port_ = this->get_parameter("listen_port").as_int();
    iha_ip_ = this->get_parameter("iha_ip").as_string();
    iha_port_ = this->get_parameter("iha_port").as_int();
    yki_ip_ = this->get_parameter("yki_ip").as_string();
    yki_port_ = this->get_parameter("yki_port").as_int();
    origin_lat_ = this->get_parameter("local_origin_lat").as_double();
    origin_lon_ = this->get_parameter("local_origin_lon").as_double();

    if (origin_lat_ == 0.0 && origin_lon_ == 0.0) {
      RCLCPP_WARN(this->get_logger(),
        "local_origin_lat/lon HALA PLACEHOLDER (0,0) - gercek IKA-KB koordinati girilmeden "
        "waypoint donusumu YANLIS olacak!");
    }

    path_pub_ = this->create_publisher<nav_msgs::msg::Path>("/route/waypoints", 10);

    if (!setupSocket()) {
      RCLCPP_FATAL(this->get_logger(),
        "UDP soket kurulamadi - route_bridge_node ISLEVSIZ durumda! Node yine de acik kalacak "
        "ama HICBIR PAKET ALAMAYACAK.");
    }

    timer_ = this->create_wall_timer(
      std::chrono::milliseconds(50), std::bind(&RouteBridgeNode::pollSocket, this));
    RCLCPP_INFO(this->get_logger(), "route_bridge_node (ICD v2) basladi, port %d dinleniyor.", listen_port_);
  }
  ~RouteBridgeNode() { if (sock_fd_ >= 0) close(sock_fd_); }

private:
  bool setupSocket()
  {
    sock_fd_ = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock_fd_ < 0) {
      RCLCPP_ERROR(this->get_logger(), "socket() basarisiz: %s", strerror(errno));
      return false;
    }

    int reuse = 1;
    if (setsockopt(sock_fd_, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
      RCLCPP_WARN(this->get_logger(), "SO_REUSEADDR ayarlanamadi: %s", strerror(errno));
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(listen_port_);

    if (bind(sock_fd_, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
      RCLCPP_ERROR(this->get_logger(),
        "UDP bind BASARISIZ (port %d): %s (errno=%d) - port baska bir process'te olabilir, "
        "'sudo lsof -i :%d' ile kontrol edin.", listen_port_, strerror(errno), errno, listen_port_);
      close(sock_fd_);
      sock_fd_ = -1;
      return false;
    }

    RCLCPP_INFO(this->get_logger(), "UDP port %d basariyla bind edildi.", listen_port_);
    return true;
  }

  void pollSocket()
  {
    if (sock_fd_ < 0) return;  // bind basarisizdi, dinleyecek soket yok

    char buf[8192];
    sockaddr_in from{}; socklen_t fromlen = sizeof(from);
    ssize_t n = recvfrom(sock_fd_, buf, sizeof(buf) - 1, MSG_DONTWAIT, (struct sockaddr*)&from, &fromlen);
    if (n <= 0) return;
    buf[n] = '\0';

    json msg;
    try {
      msg = json::parse(std::string(buf, n));
    } catch (const std::exception & e) {
      RCLCPP_WARN(this->get_logger(), "JSON parse edilemedi (atildi, ack gonderilemez): %s", e.what());
      return;
    }
    if (msg.value("msg_type", "") != "route_update") return;

    std::string sender_msg_id = msg.value("msg_id", "");
    long sender_seq = msg.value("seq_no", -1L);

    std::string malformed_reason;
    if (!msg.contains("msg_id")) malformed_reason = "msg_id eksik";
    if (!msg.contains("seq_no")) malformed_reason = "seq_no eksik";
    if (!msg.contains("checksum")) malformed_reason = "checksum alani eksik";
    if (!msg.contains("iha_state") || !msg["iha_state"].contains("airborne"))
      malformed_reason = "iha_state.airborne eksik";
    if (!msg.contains("route") || !msg["route"].contains("waypoints") ||
        !msg["route"]["waypoints"].is_array() || msg["route"]["waypoints"].empty()) {
      malformed_reason = "route.waypoints eksik veya bos";
    } else {
      for (const auto & wp : msg["route"]["waypoints"]) {
        if (!wp.contains("lat") || !wp.contains("lon") || !wp.contains("seq")) {
          malformed_reason = "waypoint icinde seq/lat/lon eksik";
          break;
        }
      }
    }

    if (!malformed_reason.empty()) {
      sendAck(sender_msg_id, sender_seq, "rejected_malformed", malformed_reason, "", 0);
      RCLCPP_WARN(this->get_logger(), "Paket reddedildi (malformed): %s", malformed_reason.c_str());
      return;
    }

    std::string received_crc = msg.value("checksum", "");
    std::string computed_crc = toHex(ika_comms::crc32(msg["route"].dump()));
    int wp_count = static_cast<int>(msg["route"]["waypoints"].size());

    if (computed_crc != received_crc) {
      sendAck(sender_msg_id, sender_seq, "rejected_checksum",
        "crc mismatch: expected " + computed_crc + ", got " + received_crc, computed_crc, wp_count);
      RCLCPP_WARN(this->get_logger(), "Checksum uyusmadi - REDDEDILDI.");
      return;
    }

    if (sender_seq >= 0 && sender_seq <= last_seq_no_) {
      sendAck(sender_msg_id, sender_seq, "rejected_stale",
        "seq_no " + std::to_string(sender_seq) + " <= son islenen " + std::to_string(last_seq_no_),
        computed_crc, wp_count);
      RCLCPP_WARN(this->get_logger(), "Eski/tekrar paket (seq=%ld) - REDDEDILDI.", sender_seq);
      return;
    }

    bool airborne = msg["iha_state"].value("airborne", false);
    if (!airborne) {
      sendAck(sender_msg_id, sender_seq, "rejected_not_airborne",
        "Md.16 ihlali: iha_state.airborne=false", computed_crc, wp_count);
      RCLCPP_WARN(this->get_logger(), "IHA havada degil (Md.16) - REDDEDILDI.");
      return;
    }

    if (sender_seq >= 0) last_seq_no_ = sender_seq;
    publishPath(msg["route"]);
    sendAck(sender_msg_id, sender_seq, "accepted", "", computed_crc, wp_count);
    RCLCPP_INFO(this->get_logger(), "route_update KABUL EDILDI (seq=%ld, %d waypoint).", sender_seq, wp_count);
  }

  void publishPath(const json & route)
  {
    nav_msgs::msg::Path path;
    path.header.stamp = this->get_clock()->now();
    path.header.frame_id = "odom";
    for (const auto & wp : route["waypoints"]) {
      auto local = ika_comms::latlonToLocalXY(
        wp.value("lat", 0.0), wp.value("lon", 0.0), origin_lat_, origin_lon_);
      geometry_msgs::msg::PoseStamped ps;
      ps.header = path.header;
      ps.pose.position.x = local.x;
      ps.pose.position.y = local.y;
      ps.pose.orientation.w = 1.0;
      path.poses.push_back(ps);
    }
    path_pub_->publish(path);
  }

  void sendAck(const std::string & ref_msg_id, long ref_seq, const std::string & status,
               const std::string & reason, const std::string & computed_checksum, int wp_count)
  {
    ack_counter_++;
    json body;
    body["msg_type"] = "route_ack";
    body["msg_id"] = "IKA01-" + std::to_string(100000 + ack_counter_);
    body["seq_no"] = ack_counter_;
    body["ref_msg_id"] = ref_msg_id;
    body["ref_seq_no"] = ref_seq;
    body["timestamp_utc"] = nowIso8601();
    body["sender"] = "IKA-01";
    body["receiver"] = "IHA-01";
    body["status"] = status;
    body["reason"] = reason.empty() ? json(nullptr) : json(reason);
    body["computed_checksum"] = computed_checksum;
    body["waypoint_count_received"] = wp_count;

    std::string ack_crc = toHex(ika_comms::crc32(body.dump()));
    body["checksum"] = ack_crc;

    std::string payload = body.dump();
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

  std::string nowIso8601()
  {
    auto now = std::chrono::system_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tm_utc; gmtime_r(&t, &tm_utc);
    std::ostringstream oss;
    oss << std::put_time(&tm_utc, "%Y-%m-%dT%H:%M:%S");
    oss << '.' << std::setfill('0') << std::setw(3) << ms.count() << 'Z';
    return oss.str();
  }

  int sock_fd_{-1}, listen_port_, iha_port_, yki_port_;
  std::string iha_ip_, yki_ip_;
  double origin_lat_, origin_lon_;
  long last_seq_no_;
  long ack_counter_;
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
