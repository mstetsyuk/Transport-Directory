#include "Stop.h"

Stop::Stop(std::string name, Coordinates coords) : name_(std::move(name)), coords_(coords) {
}

const std::string &Stop::GetName() const {
    return name_;
}
Coordinates Stop::GetCoordinates() const {
    return coords_;
}
const std::set<Bus *> &Stop::GetBuses() const {
    return buses_that_stop_;
}
std::set<Bus *> &Stop::GetBuses() {
    return buses_that_stop_;
}

const std::set<Bus *> *Stop::GetBusesPtr() const {
    return &buses_that_stop_;
}
Coordinates &Stop::GetCoordinates() {
    return coords_;
}
std::string &Stop::GetName() {
    return name_;
}
Stop::distances_map &Stop::GetDistances() {
    return distances_to_adj_stops_;
}
const Stop::distances_map &Stop::GetDistances() const {
    return distances_to_adj_stops_;
}
int Stop::dist(Stop *stop_from, Stop *stop_to) {
    if (stop_from->GetDistances().find(stop_to) != stop_from->GetDistances().end())
        return stop_from->GetDistances().at(stop_to);
    return Coordinates::dist(stop_from->GetCoordinates(), stop_to->GetCoordinates());
}
