#include "Response.h"
#include "Bus.h"
#include <cmath>
#include <map>
#include <utility>

BusStats::BusStats(const Bus &bus, int request_id) : Response(request_id),
                                                     bus_name(bus.GetId()),
                                                     num_of_unique_stops([&] {
                                                         std::vector<Stop *> v = bus.GetStops();
                                                         std::sort(v.begin(), v.end());
                                                         return std::unique(v.begin(), v.end()) - v.begin();
                                                     }()),
                                                     num_of_stops_on_route(bus.isCircular() ? bus.GetStops().size() : 2 * bus.GetStops().size() - 1) {

    route_length = 0;
    const auto &stops = bus.GetStops();
    for (auto it = stops.begin(); it != std::prev(stops.end()); ++it) {
        route_length += Stop::dist(*it, *std::next(it));
    }
    if (!bus.isCircular())
        for (auto it = stops.rbegin(); it != std::prev(stops.rend()); ++it)
            route_length += Stop::dist(*it, *std::next(it));


    curvature = route_length / CoordsWiseRouteLength(bus);
}

std::ostream &StopStats::operator<<(std::ostream &os) const {
    os << "    {\n";
    os << "        \"request_id\": " << request_id << ",\n";
    os << "        \"buses\": ";
    os << "[";
    if (names_of_buses_that_stop_there.empty()) {
        os << "]\n";
    } else {
        os << "\n";
        for (auto it = names_of_buses_that_stop_there.begin(); it != names_of_buses_that_stop_there.end(); ++it) {
            if (it != prev(names_of_buses_that_stop_there.end()))
                os << "            \"" << *it << "\",\n";
            else
                os << "            \"" << *it << "\"\n";
        }
        os << "        ]\n";
    }
    os << "    }";
    return os;
}


double BusStats::CoordsWiseRouteLength(const Bus &bus) {
    double res = 0;
    const auto &stops = bus.GetStops();
    for (size_t i = 0; i != stops.size() - 1; ++i)
        res += Coordinates::dist(
                stops[i]->GetCoordinates(), stops[i + 1]->GetCoordinates());
    if (!bus.isCircular())
        res *= 2;
    return res;
}


StopStats::StopStats(const Stop &stop, int request_id)
    : Response(request_id), stop_name(stop.GetName()) {
    for (auto *bus_ptr : stop.GetBuses()) {
        names_of_buses_that_stop_there.push_back(bus_ptr->GetId());
    }
}

std::ostream &BusStats::operator<<(std::ostream &os) const {

    return os << "    {\n"
              << "        \"request_id\": " << request_id << ",\n"
              << "        \"stop_count\": " << num_of_stops_on_route << ",\n"
              << "        \"unique_stop_count\": " << num_of_unique_stops << ",\n"
              << "        \"route_length\": " << route_length << ",\n"
              << "        \"curvature\": " << std::setprecision(16) << curvature << '\n'
              << "    }";
}
Response::Response(int request_id)
    : request_id(request_id) {
}
std::ostream &StatsNotFound::operator<<(std::ostream &os) const {
    return os << "    {\n"
              << "        \"request_id\": " << request_id << ",\n"
              << "        \"error_message\": "
              << "        \"not found\""
              << "    }";
}

StatsNotFound::StatsNotFound(int request_id) : Response(request_id) {}


