#pragma once

struct Coordinates {
    double lat_, lon_;
    static double dist(const Coordinates &lhs, const Coordinates &rhs);
};
