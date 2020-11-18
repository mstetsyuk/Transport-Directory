#pragma once
#include "Bus.h"
#include "json.h"
#include "Coordinates.h"
#include <functional>
#include <set>
#include <string>
#include <utility>

struct Stop {

private:
    using distances_map = std::unordered_map<Stop *, int>;

    std::string name_;
    Coordinates coords_;
    std::set<Bus *> buses_that_stop_;
    std::unordered_map<Stop *, int> distances_to_adj_stops_;

public:
    Stop() = default;
    Stop(std::string name, Coordinates coords);

    const std::string &GetName() const;
    std::string &GetName();

    void SetName(std::string name_to_set) {
        name_ = std::move(name_to_set);
    }
    void SetCoordinates(const Coordinates &coords) {
        coords_ = coords;
    }
    void AddNewAdjacentStop(Stop *adj_stop_ptr, int dist_to_adj_stop) {
        distances_to_adj_stops_[adj_stop_ptr] = dist_to_adj_stop;
    }
    bool IsAdjacentTo(Stop *other) {
        return distances_to_adj_stops_.find(other) != distances_to_adj_stops_.end();
    }

    const std::set<Bus *> &GetBuses() const;

    Coordinates &GetCoordinates();
    Coordinates GetCoordinates() const;

    distances_map &GetDistances();
    const distances_map &GetDistances() const;

    std::set<Bus *> const *GetBusesPtr() const;
    std::set<Bus *> &GetBuses();

    void AddBus(Bus *bus_ptr) {
        buses_that_stop_.insert(bus_ptr);
    }

    static int dist(Stop *stop_from, Stop *stop_to);
};
