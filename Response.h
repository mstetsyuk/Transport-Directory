#pragma once

#include "Bus.h"
#include "Stop.h"
#include <iomanip>
#include <iostream>
#include <memory>
#include <set>
#include <utility>

namespace RouteItem {
    struct Item {
        double time;
        Item(double time) : time(time) {}
        virtual ~Item() = default;
        virtual std::ostream &operator<<(std::ostream &ostream) const = 0;
    };
    struct Wait : public Item {
        std::string stop_name;
        Wait(std::string stop_name, double time) : Item(time), stop_name(std::move(stop_name)) {}
        std::ostream &operator<<(std::ostream &ostream) const override {
            return ostream << "        {\n"
                           << "            \"type\": \"Wait\",\n"
                           << "            \"stop_name\": " << '"' << stop_name << '"' << ",\n"
                           << "            \"time\": " << time << "\n"
                           << "        }";
        }
    };
    struct Bus : public Item {
        std::string bus_name;
        int span_cnt;

        Bus(std::string bus_name, int span_cnt, double time)
            : Item(time), bus_name(std::move(bus_name)), span_cnt(span_cnt) {}
        std::ostream &operator<<(std::ostream &ostream) const override {
            return ostream << "        {\n"
                           << "            " << R"("type": "Bus")" << ",\n"
                           << "            \"bus\": " << '"' << bus_name << ",\n"
                           << "            \"span_count\": " << span_cnt << ",\n"
                           << "            \"time\": " << time << "\n"
                           << "        }";
        }
    };
}// namespace RouteItem

struct Response {
protected:
    const int request_id;

public:
    virtual ~Response() = default;

    explicit Response(int request_id);
    virtual std::ostream &operator<<(std::ostream &os) const = 0;
};


struct BusStats : public Response {

private:
    std::string bus_name;
    int num_of_stops_on_route = 0, num_of_unique_stops = 0, route_length = 0;
    double curvature = 0;

    static double CoordsWiseRouteLength(const Bus &bus);

public:
    explicit BusStats(const Bus &bus, int request_id);

    std::ostream &operator<<(std::ostream &os) const override;
};

struct StopStats : public Response {
private:
    std::string stop_name;
    std::vector<std::string_view> names_of_buses_that_stop_there;

public:
    explicit StopStats(const Stop &stop, int request_id);
    std::ostream &operator<<(std::ostream &os) const override;
};


struct StatsNotFound : public Response {
public:
    explicit StatsNotFound(int request_id);
    std::ostream &operator<<(std::ostream &os) const override;
};


struct Route : public Response {
private:
    std::vector<std::shared_ptr<RouteItem::Item>> items_;
    double total_time = 0;

public:
    explicit Route(std::vector<std::shared_ptr<RouteItem::Item>> items, int request_id)
        : Response(request_id),
          items_(std::move(items)) {
        for (auto &item : items_)
            total_time += item->time;
    };

    std::ostream &operator<<(std::ostream &os) const override {
        os << "    {\n"
           << "        \"request_id\": " << request_id << ",\n"
           << "        \"total_time\": " << total_time << ",\n"
           << "        \"items: \" ";
        os << "[";
        if (items_.empty()) {
            os << "]\n";
        } else {
            os << "\n";
            for (const auto &item_ptr : items_) {
                item_ptr->operator<<(os);
                os << ",\n";
            }
        }
        os << "    }";
        return os;
    }
};
