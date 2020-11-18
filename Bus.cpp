#include "Bus.h"

#include <utility>


const std::string &Bus::GetId() const {
    return id_;
}
const std::vector<Stop *> &Bus::GetStops() const {
    return places_where_bus_stops;
}

std::vector<Stop *> &Bus::GetStops() {
    return places_where_bus_stops;
}
void Bus::MakeCircular() {
    isCircular_ = true;
}
bool Bus::isCircular() const {
    return isCircular_;
}
Bus &Bus::SetId(std::string new_id) {
    id_ = std::move(new_id);
    return *this;
}
