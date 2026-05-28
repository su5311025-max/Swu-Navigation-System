#pragma once

#include "Models.h"

#include <map>
#include <unordered_map>
#include <vector>

class Graph {
public:
    void addNode(const Node& node);
    void addEdge(const Edge& edge, bool bidirectional = false);

    bool hasNode(int nodeId) const;
    const Node* getNode(int nodeId) const;
    const std::unordered_map<int, Node>& getNodes() const;
    const std::vector<Edge>& getNeighbors(int nodeId) const;

private:
    std::unordered_map<int, Node> nodes_;
    std::unordered_map<int, std::vector<Edge> > adjacency_;
};
