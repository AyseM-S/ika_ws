#pragma once
#include <cstdint>
#include <string>

namespace ika_comms {
inline uint32_t crc32(const std::string & data)
{
  uint32_t crc = 0xFFFFFFFF;
  for (unsigned char c : data) {
    crc ^= c;
    for (int i = 0; i < 8; ++i)
      crc = (crc >> 1) ^ (0xEDB88320 & (-(int32_t)(crc & 1)));
  }
  return ~crc;
}
}
