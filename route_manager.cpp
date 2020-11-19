#include "route_manager.h"
#include "Response.h"


#include <map>
#include <regex>
#include <tuple>
#include <variant>
using namespace std;

RouteManager &RouteManager::MakeUpdate(const map<std::string, Json::Node> &update) {
    if (update.at("type").AsString() == "Stop") {
        AddStop(update);
    } else {
        AddBus(update);
    }
    return *this;
}
std::unique_ptr<Response> RouteManager::MakeCall(const map<std::string, Json::Node> &call) const {
    if (call.at("type").AsString() == "Bus") {
        return FindBusInfo(call);
    } else if (call.at("type").AsString() == "Stop") {
        return FindStopInfo(call);
    } else {
        return FindRoute(call);
    }
}

RouteManager &RouteManager::AddStop(const map<std::string, Json::Node> &update) {
    auto &new_stop = stops_[update.at("name").AsString()];
    new_stop.SetName(update.at("name").AsString());
    auto lat = [&]() -> variant<int, double> {
        try {
            return update.at("latitude").AsDouble();
        } catch (...) {
            return update.at("latitude").AsInt();
        }
    }();
    auto lon = [&]() -> variant<int, double> {
        try {
            return update.at("longitude").AsDouble();
        } catch (...) {
            return update.at("longitude").AsInt();
        }
    }();
    new_stop.SetCoordinates(
            {holds_alternative<int>(lat)
                     ? get<int>(lat)
                     : get<double>(lat),
             holds_alternative<int>(lon)
                     ? get<int>(lon)
                     : get<double>(lon)});


    for (auto &[stop_name, dist] : update.at("road_distances").AsMap()) {
        auto stop_ptr = &stops_[stop_name];
        int new_dist = dist.AsInt();

        new_stop.AddNewAdjacentStop(stop_ptr, new_dist);
        if (!stop_ptr->IsAdjacentTo(&new_stop)) {
            stop_ptr->AddNewAdjacentStop(&new_stop, new_dist);
        }
    }
    return *this;
}

RouteManager &RouteManager::AddBus(const map<std::string, Json::Node> &update) {
    auto &new_bus = buses_[update.at("name").AsString()];
    new_bus.SetId(update.at("name").AsString());

    for (auto &stop_name : update.at("stops").AsArray()) {
        new_bus.AddStop(&stops_[stop_name.AsString()]);
        stops_[stop_name.AsString()].AddBus(&new_bus);
    }
    if (update.at("is_roundtrip").AsBool())
        new_bus.MakeCircular();
    return *this;
}
std::unique_ptr<Response> RouteManager::FindBusInfo(const map<std::string, Json::Node> &call) const {
    if (buses_.find(call.at("name").AsString()) != buses_.end())
        return std::make_unique<BusStats>(buses_.at(call.at("name").AsString()), call.at("id").AsInt());
    return std::make_unique<StatsNotFound>(call.at("id").AsInt());
}

std::unique_ptr<Response> RouteManager::FindStopInfo(const map<std::string, Json::Node> &call) const {
    if (stops_.find(call.at("name").AsString()) != stops_.end())
        return std::make_unique<StopStats>(stops_.at(call.at("name").AsString()), call.at("id").AsInt());
    return std::make_unique<StatsNotFound>(call.at("id").AsInt());
}
std::unique_ptr<Response> RouteManager::FindRoute(const map<std::string, Json::Node> &call) const {
    if (stops_enumeration.empty()) {
        stops_enumeration = EnumerateStops();
        inv_stops_enumeration = InvertMap(stops_enumeration);
    }

    if (!graph.has_value()) {
        graph.emplace(stops_.size());
        for (const auto &[name, curr_bus] : buses_) {
            const auto &curr_bus_stops = curr_bus.GetStops();
            int initial_number_of_edges = graph->GetEdgeCount();
            for (auto it1 = curr_bus_stops.begin(); it1 != curr_bus_stops.end(); ++it1) {
                for (auto it2 = it1; it2 != curr_bus_stops.end(); ++it2) {
                    graph->AddEdge(
                            {stops_enumeration.at(*it1),
                             stops_enumeration.at(*it2),
                             *it1 == *it2
                                     ? routingSettings.wait_time
                                     : (graph->GetEdge(graph->GetEdgeCount() - 1).weight +
                                        (Stop::dist(*prev(it2), *it2) / routingSettings.velocity))});
                }
            }
            if (!curr_bus.isCircular()) {
                for (auto it1 = curr_bus_stops.rbegin(); it1 != curr_bus_stops.rend(); ++it1) {
                    for (auto it2 = it1; it2 != curr_bus_stops.rend(); ++it2) {
                        graph->AddEdge(
                                {stops_enumeration.at(*it1),
                                 stops_enumeration.at(*it2),
                                 *it1 == *it2
                                         ? routingSettings.wait_time
                                         : (graph->GetEdge(graph->GetEdgeCount() - 1).weight +
                                            (Stop::dist(*prev(it2), *it2) / routingSettings.velocity))});
                    }
                }
            } else {
                for (auto it1 = curr_bus_stops.end() - 2; it1 > curr_bus_stops.begin(); --it1) {
                    for (auto it2 = curr_bus_stops.begin() + 1; it2 < it1; ++it2) {
                        auto edge_to_end = graph->GetEdge(CalculateEdgeId(it1, curr_bus_stops.end() - 1, curr_bus_stops, initial_number_of_edges));
                        auto edge_from_start = graph->GetEdge(CalculateEdgeId(curr_bus_stops.begin(), it2, curr_bus_stops, initial_number_of_edges));
                        auto edge_to_add = Graph::Edge<double>{stops_enumeration.at(*it1),
                                                               stops_enumeration.at(*it2),
                                                               edge_from_start.weight + edge_to_end.weight - routingSettings.wait_time};
                        graph->AddEdge(edge_to_add);
                    }
                }
            }
        }
    }

    if (!router.has_value()) {
        router.emplace(*graph);
    }
    const auto route = router->BuildRoute(
            stops_enumeration.at(&stops_.at(call.at("from").AsString())),
            stops_enumeration.at(&stops_.at(call.at("to").AsString())));
    bool RouteWasFound = (route != std::nullopt);
    if (!RouteWasFound)
        return std::make_unique<StatsNotFound>(call.at("request_id").AsInt());

    std::vector<std::shared_ptr<RouteItem::Item>> route_items;
    for (int i = 0; i != route->edge_count; ++i) {
        int edgeId = router->GetRouteEdge(route->id, i);
        auto *stop_from = inv_stops_enumeration.at(graph->GetEdge(edgeId).from);
        auto *stop_to = inv_stops_enumeration.at(graph->GetEdge(edgeId).to);
        auto weight = graph->GetEdge(edgeId).weight;

        const Bus *curr_bus = FindTheBus(stop_from, stop_to, weight);

        const auto &stops = curr_bus->GetStops();
        int span_cnt = abs(std::find(curr_bus->GetStops().begin(),
                                     curr_bus->GetStops().end(),
                                     stop_to) -
                           std::find(curr_bus->GetStops().begin(),
                                     curr_bus->GetStops().end(),
                                     stop_from));
        if (span_cnt) {
            route_items.push_back(
                    std::make_shared<RouteItem::Wait>(stop_from->GetName(),
                                                      routingSettings.wait_time));
            route_items.push_back(
                    std::make_shared<RouteItem::Bus>(curr_bus->GetId(),
                                                     span_cnt,
                                                     weight - routingSettings.wait_time));
        }
    }
    router->ReleaseRoute(route->id);
    return std::make_unique<Route>(route_items, call.at("id").AsInt());
}
unordered_map<const Stop *, Graph::VertexId> RouteManager::EnumerateStops() const {
    int cnt = 0;
    std::unordered_map<const Stop *, VertexId> res;
    for (auto &[name, stop] : stops_) {
        res[&stop] = cnt++;
    }
    return res;
}
unordered_map<Graph::VertexId, const Stop *> RouteManager::InvertMap(const unordered_map<const Stop *, VertexId> &stops_enumeration) {
    std::unordered_map<VertexId, const Stop *> res;
    for (auto &[stop, id] : stops_enumeration)
        res[id] = stop;
    return res;
}
template<typename It>
Graph::EdgeId RouteManager::CalculateEdgeId(It it1, It it2, const vector<Stop *> &stops, int initial_number_of_edges) const {
    size_t i = it1 - stops.begin() + 1;
    size_t j = it2 - stops.begin() + 1;
    return (i - 1) * stops.size() - std::max({0, static_cast<int>((i - 1) * (i - 2) / 2)}) + (j - i + 1) + initial_number_of_edges;
}
const Bus *RouteManager::FindTheBus(const Stop *from, const Stop *to, double weight) const {
    for (const auto &curr_bus : from->GetBuses()) {
        double curr_bus_time = 0;
        bool HasRoute = std::find(curr_bus->GetStops().begin(),
                                  curr_bus->GetStops().end(),
                                  to) != curr_bus->GetStops().end();
        if (!HasRoute) continue;
        if (!curr_bus->isCircular()) {
            auto it1 = std::min(std::find(curr_bus->GetStops().begin(),
                                          curr_bus->GetStops().end(),
                                          from),
                                std::find(curr_bus->GetStops().begin(),
                                          curr_bus->GetStops().end(),
                                          to));
            auto it2 = std::max(std::find(curr_bus->GetStops().begin(),
                                          curr_bus->GetStops().end(),
                                          from),
                                std::find(curr_bus->GetStops().begin(),
                                          curr_bus->GetStops().end(),
                                          to));
            if (*it1 == from && *it2 == to)
                for (auto it = it1; it != it2; ++it) {
                    curr_bus_time += Stop::dist(*it, *next(it));
                }
            else if (*it2 == from && *it1 == to)
                for (auto it = it2; it != it1; it--) {
                    curr_bus_time += Stop::dist(*it, *prev(it));
                }
        } else {
            auto it1 = std::find(curr_bus->GetStops().begin(),
                                 curr_bus->GetStops().end(),
                                 from);
            auto it2 = std::find(curr_bus->GetStops().begin(),
                                 curr_bus->GetStops().end(),
                                 to);
            if (it1 < it2) {
                for (auto it = it1; it != it2; ++it) {
                    curr_bus_time += Stop::dist(*it, *next(it));
                }
            } else {
                for (auto it = it1; it != std::prev(curr_bus->GetStops().end()); ++it) {
                    curr_bus_time += Stop::dist(*it, *std::next(it));
                }
                for (auto it = curr_bus->GetStops().begin(); it != it2; ++it) {
                    curr_bus_time += Stop::dist(*it, *std::next(it));
                }
            }
        }
        curr_bus_time /= routingSettings.velocity;

        if (static_cast<float>(weight) == static_cast<float>(curr_bus_time + routingSettings.wait_time))
            return curr_bus;
        else
            int i = 10;
    }
    throw std::runtime_error("No buses found!");
}
