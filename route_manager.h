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
    //velocity in metres per min
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
    std::unique_ptr<Response> FindRoute(const std::map<std::string, Json::Node> &call) const;

    std::unordered_map<const Stop *, VertexId> EnumerateStops() const;
    static std::unordered_map<VertexId, const Stop *> InvertMap(const std::unordered_map<const Stop *, VertexId> &stops_enumeration);

    template<typename It>
    Graph::EdgeId CalculateEdgeId(It it1, It it2, const std::vector<Stop *> &stops, int initial_number_of_edges) const;

    const Bus *FindTheBus(const Stop *from, const Stop *to, double weight) const;

public:
    RouteManager(double wait_time, double velocity) : routingSettings({wait_time, velocity * 16.66}) {}
    RouteManager &MakeUpdate(const std::map<std::string, Json::Node> &update);
    std::unique_ptr<Response> MakeCall(const std::map<std::string, Json::Node> &call) const;
};
