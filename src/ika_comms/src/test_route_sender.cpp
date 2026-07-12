// Tek seferlik test UDP gonderici - route_bridge_node'u dogrulamak icin.
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
  route["detection_confidence"] = 0.94;
  route["coordinate_frame"] = "local_enu";
  route["start_point"] = {{"x", 0.0}, {"y", 0.0}};
  route["end_point"] = {{"x", 10.0}, {"y", 5.0}};
  route["waypoints"] = json::array({
    {{"seq", 0}, {"x", 0.0}, {"y", 0.0}},
    {{"seq", 1}, {"x", 5.0}, {"y", 2.5}},
    {{"seq", 2}, {"x", 10.0}, {"y", 5.0}}});

  uint32_t crc = ika_comms::crc32(route.dump());
  std::ostringstream oss; oss << std::hex << std::setw(8) << std::setfill('0') << crc;

  json msg;
  msg["msg_type"] = "route_update";
  msg["msg_id"] = "TEST-0001";
  msg["timestamp_utc"] = "2026-07-12T00:00:00Z";
  msg["sender"] = "IHA-TEST";
  msg["mission_id"] = "test-run";
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
  std::cout << "Test route_update gonderildi (checksum=" << oss.str() << ")\n";
  return 0;
}
