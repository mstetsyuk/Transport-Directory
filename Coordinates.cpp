#include "Coordinates.h"
#include <cmath>


double Coordinates::dist(const Coordinates &lhs, const Coordinates &rhs) {
    const double lhs_lat = lhs.lat_ * 3.1415926535 / 180;
    const double lhs_lon = lhs.lon_ * 3.1415926535 / 180;
    const double rhs_lat = rhs.lat_ * 3.1415926535 / 180;
    const double rhs_lon = rhs.lon_ * 3.1415926535 / 180;
    return acos(
            sin(lhs_lat) * sin(rhs_lat) +
            cos(lhs_lat) * cos(rhs_lat) * cos(std::abs(lhs_lon - rhs_lon))) *
           6371000;
}