#pragma once

#include <functional>
#include <string>
#include <vector>
struct Stop;

struct Bus {
private:
    std::string id_;
    bool isCircular_ = false;
    std::vector<Stop *> places_where_bus_stops{};

public:
    Bus() = default;


    const std::string &GetId() const;
    Bus &SetId(std::string new_id);

    const std::vector<Stop *> &GetStops() const;
    std::vector<Stop *> &GetStops();

    void MakeCircular();
    bool isCircular() const;

    void AddStop(Stop *stop) {
        places_where_bus_stops.push_back(stop);
    }
};