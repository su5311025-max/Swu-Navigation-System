#pragma once

#include <map>
#include <string>
#include <utility>
#include <vector>

struct Node {
    int id;
    std::string name;
    double lon;
    double lat;
};

struct Edge {
    int id;
    int from;
    int to;
    double distanceMeters;
    double travelTimeSeconds;
    std::string wayType;
    std::string name;
    std::string geometryJson;
    double effectiveWeight() const {
        return travelTimeSeconds;
    }
};

struct Task {
    int id;
    int startNode;
    int endNode;
};

struct PathResult {
    bool found = false;
    double cost = 0.0;
    double distanceMeters = 0.0;
    double travelTimeSeconds = 0.0;
    std::vector<int> path;
    std::vector<Edge> segments;
    std::string error;
};

struct MSTResult {
    bool success = false;
    double totalCost = 0.0;
    std::vector<std::pair<int, int> > edges;
    std::string error;
};

struct DBConfig {
    std::string host = "127.0.0.1";
    int port = 3306;
    std::string database = "l_engine";
    std::string user = "root";
    std::string password;
    int pollIntervalMs = 1000;
};
