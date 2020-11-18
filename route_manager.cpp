#include "route_manager.h"
#include "Response.h"


#include <regex>
#include <tuple>
#include <map>
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

