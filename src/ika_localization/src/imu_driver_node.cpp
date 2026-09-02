// BNO055 IMU surucusu - NDOF (fusion) modu
// Pusula kaynagi NETLESTIRILDI: BNO055'in dahili manyetometresi, ivmeolcer ve
// jiroskopla birlikte cip-ici sensor fuzyonuna (NDOF) katiliyor; EKF'e giden
// /imu/data orientation alani bu fuzyonun ciktisi - yani mutlak yon referansi
// GNSS degil, BNO055'in kendi pusulasidir. (Rapor Bol. 3.3.5.3 icin.)
#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/imu.hpp>
#include <diagnostic_msgs/msg/diagnostic_status.hpp>
extern "C" {
  #include <linux/i2c-dev.h>
  #include <sys/ioctl.h>
  #include <fcntl.h>
  #include <unistd.h>
}

class Bno055Node : public rclcpp::Node
{
public:
  Bno055Node() : Node("imu_driver_node")
  {
    i2c_bus_   = declare_parameter("i2c_bus", "/dev/i2c-1");
    i2c_addr_  = declare_parameter("i2c_address", 0x28);
    frame_id_  = declare_parameter("frame_id", "imu_link");

    fd_ = open(i2c_bus_.c_str(), O_RDWR);
    if (fd_ < 0 || ioctl(fd_, I2C_SLAVE, i2c_addr_) < 0) {
      RCLCPP_FATAL(get_logger(), "BNO055 I2C acilamadi (%s @0x%02X) - bagli mi?",
                   i2c_bus_.c_str(), i2c_addr_);
      throw std::runtime_error("I2C init failed");
    }

    // 1) CONFIG moduna al, 2) NDOF moduna gecir (datasheet: mod degisimi >=19ms surer)
    write_reg(OPR_MODE, 0x00);
    rclcpp::sleep_for(std::chrono::milliseconds(25));
    write_reg(OPR_MODE, 0x0C); // 0x0C = NDOF (accel+gyro+mag fusion)
    rclcpp::sleep_for(std::chrono::milliseconds(25));

    uint8_t mode_check = read_reg(OPR_MODE);
    if (mode_check != 0x0C) {
      RCLCPP_ERROR(get_logger(),
        "BNO055 NDOF moduna gecemedi (okunan mod=0x%02X) - pusula EKF'e GIRMIYOR olabilir!",
        mode_check);
    } else {
      RCLCPP_INFO(get_logger(), "BNO055 NDOF (manyetometre dahil fuzyon) modunda calisiyor.");
    }

    imu_pub_ = create_publisher<sensor_msgs::msg::Imu>("/imu/data", 10);
    diag_pub_ = create_publisher<diagnostic_msgs::msg::DiagnosticStatus>("/imu/calib_status", 10);

    timer_ = create_wall_timer(std::chrono::milliseconds(20), // ~50 Hz
      std::bind(&Bno055Node::publish_imu, this));
    calib_timer_ = create_wall_timer(std::chrono::seconds(2),
      std::bind(&Bno055Node::check_calibration, this));
  }

private:
  static constexpr uint8_t OPR_MODE   = 0x3D;
  static constexpr uint8_t CALIB_STAT = 0x35;
  static constexpr uint8_t QUAT_DATA  = 0x20; // 8 byte: w,x,y,z (Q16 format)
  static constexpr uint8_t GYRO_DATA  = 0x14; // 6 byte: x,y,z (1/16 dps)

  void write_reg(uint8_t reg, uint8_t val) {
    uint8_t buf[2] = {reg, val};
    write(fd_, buf, 2);
  }
  uint8_t read_reg(uint8_t reg) {
    write(fd_, &reg, 1);
    uint8_t val = 0;
    read(fd_, &val, 1);
    return val;
  }
  void read_block(uint8_t reg, uint8_t *buf, size_t len) {
    write(fd_, &reg, 1);
    read(fd_, buf, len);
  }

  void publish_imu() {
    uint8_t q[8], g[6];
    read_block(QUAT_DATA, q, 8);
    read_block(GYRO_DATA, g, 6);

    auto to_i16 = [](uint8_t lo, uint8_t hi) { return static_cast<int16_t>((hi << 8) | lo); };
    double scale_q = 1.0 / (1 << 14); // Q16 -> quaternion (BNO055 datasheet 3.6.5.5)
    double w = to_i16(q[0], q[1]) * scale_q;
    double x = to_i16(q[2], q[3]) * scale_q;
    double y = to_i16(q[4], q[5]) * scale_q;
    double z = to_i16(q[6], q[7]) * scale_q;

    double scale_g = (M_PI / 180.0) / 16.0; // 1/16 dps -> rad/s
    double gx = to_i16(g[0], g[1]) * scale_g;
    double gy = to_i16(g[2], g[3]) * scale_g;
    double gz = to_i16(g[4], g[5]) * scale_g;

    sensor_msgs::msg::Imu msg;
    msg.header.stamp = now();
    msg.header.frame_id = frame_id_;
    msg.orientation.w = w; msg.orientation.x = x;
    msg.orientation.y = y; msg.orientation.z = z;
    msg.angular_velocity.x = gx; msg.angular_velocity.y = gy; msg.angular_velocity.z = gz;

    // Kalibrasyon durumuna gore orientation covariance'i dinamik ayarla:
    // mag kalibre degilse (calib_status dusukse) EKF'in yaw'a olan guvenini azalt.
    double yaw_cov = last_mag_calib_ >= 2 ? 0.02 : 0.5; // kalibre degilse guveni %25'e dusur
    for (auto &c : msg.orientation_covariance) c = 0.0;
    msg.orientation_covariance[8] = yaw_cov; // [2][2] = yaw
    for (auto &c : msg.angular_velocity_covariance) c = 0.0;
    msg.angular_velocity_covariance[8] = 0.01;

    imu_pub_->publish(msg);
  }

  void check_calibration() {
    uint8_t s = read_reg(CALIB_STAT);
    uint8_t mag_calib = s & 0x03;        // bit0-1 = mag
    uint8_t sys_calib  = (s >> 6) & 0x03; // bit6-7 = system
    last_mag_calib_ = mag_calib;

    diagnostic_msgs::msg::DiagnosticStatus diag;
    diag.name = "bno055_calibration";
    diag.message = "mag=" + std::to_string(mag_calib) + " sys=" + std::to_string(sys_calib);
    diag.level = (mag_calib < 2) ? diagnostic_msgs::msg::DiagnosticStatus::WARN
                                  : diagnostic_msgs::msg::DiagnosticStatus::OK;
    diag_pub_->publish(diag);

    if (mag_calib < 2) {
      RCLCPP_WARN(get_logger(),
        "BNO055 manyetometre kalibrasyonu dusuk (%d/3) - pusula yonu guvenilmez olabilir. "
        "Araci 8 rakami cizerek kalibre edin.", mag_calib);
    }
  }

  int fd_;
  std::string i2c_bus_, frame_id_;
  int i2c_addr_;
  uint8_t last_mag_calib_ = 0;
  rclcpp::Publisher<sensor_msgs::msg::Imu>::SharedPtr imu_pub_;
  rclcpp::Publisher<diagnostic_msgs::msg::DiagnosticStatus>::SharedPtr diag_pub_;
  rclcpp::TimerBase::SharedPtr timer_, calib_timer_;
};

int main(int argc, char **argv) {
  rclcpp::init(argc, argv);
  try {
    rclcpp::spin(std::make_shared<Bno055Node>());
  } catch (const std::exception &e) {
    RCLCPP_FATAL(rclcpp::get_logger("imu_driver_node"), "%s", e.what());
  }
  rclcpp::shutdown();
  return 0;
}
