#pragma once

#include "Bus.h"
#include "Coordinates.h"
#include "Response.h"
#include "Stop.h"
#include "graph.h"
#include "json.h"
#include "router.h"

#include <iostream>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <utility>

struct RoutingSettings {
    double wait_time, velocity;
    //wait time in mins
    //velocity in metres / min
};


class RouteManager {
private:
    using VertexId = size_t;
    std::unordered_map<std::string, Stop> stops_;
    std::unordered_map<std::string, Bus> buses_;
    RoutingSettings routingSettings;

    mutable std::unordered_map<const Stop *, VertexId> stops_enumeration{};
    mutable std::unordered_map<VertexId, const Stop *> inv_stops_enumeration{};
    mutable std::optional<Graph::DirectedWeightedGraph<double>> graph;
    mutable std::optional<Graph::Router<double>> router;

    RouteManager &AddStop(const std::map<std::string, Json::Node> &update);
    RouteManager &AddBus(const std::map<std::string, Json::Node> &update);

    std::unique_ptr<Response> FindBusInfo(const std::map<std::string, Json::Node> &call) const;
    std::unique_ptr<Response> FindStopInfo(const std::map<std::string, Json::Node> &call) const;
    std::unique_ptr<Response> FindRoute(const std::map<std::string, Json::Node> &call) const {
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
                            auto Stop1 = *it1;
                            auto Stop2 = *it2;
                            auto id1 = CalculateEdgeId(it1, curr_bus_stops.end() - 1, curr_bus_stops, initial_number_of_edges);
                            auto id2 = CalculateEdgeId(curr_bus_stops.begin(), it2, curr_bus_stops, initial_number_of_edges);
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

            auto stops = curr_bus->GetStops();
            auto it1 = std::find(curr_bus->GetStops().begin(),
                                 curr_bus->GetStops().end(),
                                 stop_to);
            auto it2 = std::find(curr_bus->GetStops().begin(),
                                 curr_bus->GetStops().end(),
                                 stop_from);
            int span_cnt = abs(it1 - it2);
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

    std::unordered_map<const Stop *, VertexId> EnumerateStops() const {
        int cnt = 0;
        std::unordered_map<const Stop *, VertexId> res;
        for (auto &[name, stop] : stops_) {
            res[&stop] = cnt++;
        }
        return res;
    }
    static std::unordered_map<VertexId, const Stop *> InvertMap(const std::unordered_map<const Stop *, VertexId> &stops_enumeration) {
        std::unordered_map<VertexId, const Stop *> res;
        for (auto &[stop, id] : stops_enumeration)
            res[id] = stop;
        return res;
    }
    template<typename It>
    Graph::EdgeId CalculateEdgeId(It it1, It it2, const std::vector<Stop *> &stops, int initial_number_of_edges) const {
        size_t i = it1 - stops.begin() + 1;
        size_t j = it2 - stops.begin() + 1;
        return (i - 1) * stops.size() - std::max({0, static_cast<int>((i - 1) * (i - 2) / 2)}) + (j - i + 1) + initial_number_of_edges;
    }
    const Bus *FindTheBus(const Stop *from, const Stop *to, double weight) const {
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

public:
    RouteManager(double wait_time, double velocity) : routingSettings({wait_time, velocity * 16.66}) {}
    RouteManager &MakeUpdate(const std::map<std::string, Json::Node> &update);
    std::unique_ptr<Response> MakeCall(const std::map<std::string, Json::Node> &call) const;
};