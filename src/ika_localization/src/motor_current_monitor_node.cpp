// 3x ACS712 (motor surucu hatlarinda) -> MAVLink BATTERY_STATUS (id 1,2,3) -> mavros
// -> patinaj/sikisma tespiti -> /wheel_odom_covariance_scale
//
// Mantik: komut edilen hiza gore BEKLENEN akim araligi var (bos donen teker dusuk
// akim ceker, sikisan/patinaj yapan teker STALL akimina yaklasir ya da komut edilen
// hiza ragmen anormal dusuk kalir). Olcum bu araligin disina cikarsa tekerlek
// odometrisine guveni (covariance carpani) yukseltiyoruz.
#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/battery_state.hpp>
#include <geometry_msgs/msg/twist.hpp>
#include <std_msgs/msg/float64.hpp>

class MotorCurrentMonitor : public rclcpp::Node
{
public:
  MotorCurrentMonitor() : Node("motor_current_monitor_node")
  {
    nominal_current_per_mps_ = declare_parameter("nominal_current_per_mps", 2.0); // A per m/s, tahmini - gercek testle kalibre edilecek
    stall_current_threshold_ = declare_parameter("stall_current_threshold", 5.5);  // A - motor BOM'undaki stall tahminine yakin
    idle_current_threshold_  = declare_parameter("idle_current_threshold", 0.3);   // A - komut var ama akim yoksa (baglanti/surucu arizasi)
    scale_min_ = declare_parameter("scale_min", 1.0);
    scale_max_ = declare_parameter("scale_max", 15.0); // asiri patinajda EKF tekerlege neredeyse hic guvenmesin

    // Not (#9 bagimliligi): topic isimleri mavros'un gercek donanimla kurulan
    // battery instance eslemesine gore DEGISEBILIR - gercek Pixhawk baglaninca
    // `ros2 topic list | grep battery` ile teyit edip remap edin.
    for (int i = 1; i <= 3; ++i) {
      auto topic = "/mavros/battery" + std::to_string(i);
      current_subs_.push_back(create_subscription<sensor_msgs::msg::BatteryState>(
        topic, 10,
        [this, i](sensor_msgs::msg::BatteryState::SharedPtr msg) { on_current(i, msg); }));
    }
    cmd_sub_ = create_subscription<geometry_msgs::msg::Twist>(
      "/cmd_vel", 10, std::bind(&MotorCurrentMonitor::on_cmd, this, std::placeholders::_1));

    scale_pub_ = create_publisher<std_msgs::msg::Float64>("/wheel_odom_covariance_scale", 10);
    timer_ = create_wall_timer(std::chrono::milliseconds(100),
      std::bind(&MotorCurrentMonitor::evaluate, this));
  }

private:
  void on_current(int idx, sensor_msgs::msg::BatteryState::SharedPtr msg) {
    currents_[idx] = msg->current; // Amper, mavros zaten kalibre edilmis deger verir (#9 firmware tarafinda cozulur)
  }
  void on_cmd(geometry_msgs::msg::Twist::SharedPtr msg) {
    last_cmd_speed_ = std::fabs(msg->linear.x);
  }

  void evaluate() {
    if (currents_.size() < 3) return; // henuz tum kanallardan veri gelmedi

    double max_current = 0.0, min_current = 1e9;
    for (auto &[idx, c] : currents_) {
      max_current = std::max(max_current, c);
      min_current = std::min(min_current, c);
    }
    double expected = last_cmd_speed_ * nominal_current_per_mps_;
    double scale = scale_min_;
    std::string reason = "normal";

    if (max_current > stall_current_threshold_) {
      // en az bir motor stall/sikisma akimina yakin -> o tekerlek muhtemelen donmuyor
      scale = scale_max_;
      reason = "stall_supheli";
    } else if (last_cmd_speed_ > 0.05 && max_current < idle_current_threshold_) {
      // hiz komutu var ama hicbir motor akim cekmiyor -> surucu/baglanti kopuk olabilir
      scale = scale_max_;
      reason = "akim_yok_ama_komut_var";
    } else if (expected > 0.1) {
      // motorlar arasi akim dengesizligi (bazisi cok fazla, bazisi cok az cekiyor) -> patinaj/asimetrik yuk
      double imbalance = (max_current - min_current) / std::max(expected, 0.1);
      if (imbalance > 1.5) {
        scale = scale_min_ + (scale_max_ - scale_min_) * std::min(imbalance / 4.0, 1.0);
        reason = "akim_dengesizligi";
      }
    }

    if (scale > scale_min_ * 1.01) {
      RCLCPP_WARN_THROTTLE(get_logger(), *get_clock(), 2000,
        "Olasi patinaj/sikisma (%s): akim[min=%.2f max=%.2f]A, komut hiz=%.2f m/s -> covariance x%.1f",
        reason.c_str(), min_current, max_current, last_cmd_speed_, scale);
    }

    std_msgs::msg::Float64 out;
    out.data = scale;
    scale_pub_->publish(out);
  }

  double nominal_current_per_mps_, stall_current_threshold_, idle_current_threshold_;
  double scale_min_, scale_max_, last_cmd_speed_ = 0.0;
  std::map<int, double> currents_;
  std::vector<rclcpp::Subscription<sensor_msgs::msg::BatteryState>::SharedPtr> current_subs_;
  rclcpp::Subscription<geometry_msgs::msg::Twist>::SharedPtr cmd_sub_;
  rclcpp::Publisher<std_msgs::msg::Float64>::SharedPtr scale_pub_;
  rclcpp::TimerBase::SharedPtr timer_;
};

int main(int argc, char **argv) {
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<MotorCurrentMonitor>());
  rclcpp::shutdown();
  return 0;
}
