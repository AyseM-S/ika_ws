#pragma once
#include <cmath>

namespace ika_comms {
struct LocalXY { double x, y; };

// Equirectangular (duz-dunya) yaklasimi - HTAB olceginde (~100m) hata milimetrik, ihmal edilebilir.
inline LocalXY latlonToLocalXY(double lat, double lon, double origin_lat, double origin_lon)
{
  constexpr double R = 6371000.0;
  constexpr double DEG2RAD = M_PI / 180.0;
  double origin_lat_rad = origin_lat * DEG2RAD;
  double x = (lon - origin_lon) * DEG2RAD * R * std::cos(origin_lat_rad);
  double y = (lat - origin_lat) * DEG2RAD * R;
  return {x, y};
}
}
