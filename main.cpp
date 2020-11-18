#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <regex>


#include "route_manager.h"


using namespace std;

#include <istream>
#include <map>
#include <string>
#include <variant>
#include <vector>

void serve_requests(istream &input = cin, ostream &output = cout) {
    const Json::Document input_doc = Json::Load(input);
    RouteManager mg(
            input_doc.GetRoot().AsMap().at("routing_settings").AsMap().at("bus_wait_time").AsInt(),
            input_doc.GetRoot().AsMap().at("routing_settings").AsMap().at("bus_velocity").AsInt());
    const auto &update_requests = input_doc.GetRoot().AsMap().at("base_requests").AsArray();
    for (const auto &update_request : update_requests) {
        mg.MakeUpdate(update_request.AsMap());
    }

    const auto &stats_requests = input_doc.GetRoot().AsMap().at("stat_requests").AsArray();

    output << "[\n";
    for (int i = 0; i != stats_requests.size(); ++i) {
        auto elem = stats_requests[i].AsMap();
        auto res = mg.MakeCall(elem);
        res->operator<<(output);
        if (i != stats_requests.size() - 1)
            output << ", \n";
    }
    output << "\n]";
}
// 5 -> 4

int main() {
    ifstream file;
    file.open(R"(C:\Users\mikes\CLionProjects\route_manager\tests.json)");
    if (!file) {
        cout << "Unable to open file";
        exit(1);// terminate with error
    }

    try {
        serve_requests(file);
    } catch (...) {
        file.close();
        cerr << "An error occured!\n";
    }
    file.close();
}