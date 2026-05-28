#include "Graph.h"

#include <stdexcept>

void Graph::addNode(const Node& node) {
    nodes_[node.id] = node;
    if (adjacency_.find(node.id) == adjacency_.end()) {
        adjacency_[node.id] = std::vector<Edge>();
    }
}

void Graph::addEdge(const Edge& edge, bool bidirectional) {
    if (!hasNode(edge.from) || !hasNode(edge.to)) {
        throw std::runtime_error("Edge references a missing node.");
    }

    adjacency_[edge.from].push_back(edge);
    if (bidirectional) {
        Edge reverse = edge;
        reverse.from = edge.to;
        reverse.to = edge.from;
        adjacency_[edge.to].push_back(reverse);
    }
}

bool Graph::hasNode(int nodeId) const {
    return nodes_.find(nodeId) != nodes_.end();
}

const Node* Graph::getNode(int nodeId) const {
    std::unordered_map<int, Node>::const_iterator it = nodes_.find(nodeId);
    if (it == nodes_.end()) {
        return 0;
    }
    return &it->second;
}

const std::unordered_map<int, Node>& Graph::getNodes() const {
    return nodes_;
}

const std::vector<Edge>& Graph::getNeighbors(int nodeId) const {
    static const std::vector<Edge> empty;
    std::unordered_map<int, std::vector<Edge> >::const_iterator it = adjacency_.find(nodeId);
    if (it == adjacency_.end()) {
        return empty;
    }
    return it->second;
}
