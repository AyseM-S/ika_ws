// Kisa mesafeli (~3.6m) test rotasi - tam BEKLEME->TAMAMLANDI dongusunu
// hizlica (saniyeler icinde) kanitlamak icin. Origin (local_origin_lat/lon)
// ile AYNI baslangic noktasindan, kucuk offsetlerle ilerler.
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
  route["route_id"] = "route_test_short";
  route["route_difficulty"] = "kolay";
  route["detection_confidence"] = 0.98;
  route["detection_source"] = "test_manual";
  route["coordinate_frame"] = "wgs84";
  // Origin (39.925101, 32.836742) etrafinda ~3.6m'lik kisa rota
  route["waypoints"] = json::array({
    {{"seq", 0}, {"lat", 39.925101}, {"lon", 32.836742}},
    {{"seq", 1}, {"lat", 39.9251100}, {"lon", 32.8367537}},
    {{"seq", 2}, {"lat", 39.9251190}, {"lon", 32.8367537}},
    {{"seq", 3}, {"lat", 39.9251279}, {"lon", 32.8367655}}});

  uint32_t crc = ika_comms::crc32(route.dump());
  std::ostringstream oss; oss << std::hex << std::setw(8) << std::setfill('0') << crc;

  json msg;
  msg["msg_type"] = "route_update";
  msg["msg_id"] = "IHA01-TESTSHORT";
  msg["seq_no"] = 100;
  msg["timestamp_utc"] = "2026-07-12T00:00:00.000Z";
  msg["sender"] = "IHA-01";
  msg["receiver"] = "IKA-01";
  msg["mission_id"] = "test-run-short";
  msg["iha_state"] = {{"airborne", true}, {"altitude_agl_m", 82.4},
                       {"flight_mode", "AUTO"}, {"gps_fix", "rtk_fixed"}};
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
  std::cout << "KISA rota testi gonderildi (~3.6m, seq=100)\n";
  return 0;
}
