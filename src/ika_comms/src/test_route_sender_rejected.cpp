#include <iostream>
#include <sstream>
#include <iomanip>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "nlohmann/json.hpp"
#include "ika_comms/checksum.hpp"

using json = nlohmann::json;

int main()
{
  json route;
  route["route_id"] = "route_2";
  route["route_difficulty"] = "orta";
  route["detection_confidence"] = 0.94;
  route["detection_source"] = "yolov8n+lidar_raycast";
  route["coordinate_frame"] = "wgs84";
  route["waypoints"] = json::array({
    {{"seq", 0}, {"lat", 39.925101}, {"lon", 32.836742}},
    {{"seq", 1}, {"lat", 39.925178}, {"lon", 32.836801}}});

  uint32_t crc = ika_comms::crc32(route.dump());
  std::ostringstream oss; oss << std::hex << std::setw(8) << std::setfill('0') << crc;

  json msg;
  msg["msg_type"] = "route_update";
  msg["msg_id"] = "IHA01-TEST02";
  msg["seq_no"] = 2;
  msg["timestamp_utc"] = "2026-07-12T00:00:01.000Z";
  msg["sender"] = "IHA-01";
  msg["receiver"] = "IKA-01";
  msg["mission_id"] = "test-run";
  msg["iha_state"] = {{"airborne", false}, {"altitude_agl_m", 0.0},
                       {"flight_mode", "LAND"}, {"gps_fix", "rtk_fixed"}};
  msg["route"] = route;
  msg["checksum"] = oss.str();
  std::string payload = msg.dump();

  int sock = socket(AF_INET, SOCK_DGRAM, 0);
  sockaddr_in dest{};
  dest.sin_family = AF_INET;
  dest.sin_port = htons(5005);
  inet_pton(AF_INET, "127.0.0.1", &dest.sin_addr);
  sendto(sock, payload.c_str(), payload.size(), 0, (struct sockaddr*)&dest, sizeof(dest));
  close(sock);
  std::cout << "RED testi gonderildi (seq=2, airborne=FALSE - Md.16 ihlali beklenmeli)\n";
  return 0;
}
