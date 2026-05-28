#include "PathFinder.h"

#include <algorithm>
#include <functional>
#include <limits>
#include <queue>
#include <set>
#include <stdexcept>

PathFinder::PathFinder(const Graph& graph) : graph_(graph) {
}

PathResult PathFinder::findShortestPath(int startId, int endId) const {
    PathResult result;

    if (!graph_.hasNode(startId)) {
        result.error = "Start node does not exist.";
        return result;
    }
    if (!graph_.hasNode(endId)) {
        result.error = "End node does not exist.";
        return result;
    }

    typedef std::pair<double, int> QueueItem;
    std::priority_queue<QueueItem, std::vector<QueueItem>, std::greater<QueueItem> > pq;
    std::map<int, double> distance;
    std::map<int, int> previous;
    std::map<int, Edge> previousEdge;

    const std::unordered_map<int, Node>& nodes = graph_.getNodes();
    for (std::unordered_map<int, Node>::const_iterator it = nodes.begin(); it != nodes.end(); ++it) {
        distance[it->first] = std::numeric_limits<double>::infinity();
        previous[it->first] = -1;
    }

    distance[startId] = 0.0;
    pq.push(std::make_pair(0.0, startId));

    while (!pq.empty()) {
        QueueItem current = pq.top();
        pq.pop();

        const double currentDistance = current.first;
        const int nodeId = current.second;

        if (currentDistance > distance[nodeId]) {
            continue;
        }
        if (nodeId == endId) {
            break;
        }

        const std::vector<Edge>& neighbors = graph_.getNeighbors(nodeId);
        for (std::size_t i = 0; i < neighbors.size(); ++i) {
            const Edge& edge = neighbors[i];
            const double nextDistance = currentDistance + edge.effectiveWeight();
            if (nextDistance < distance[edge.to]) {
                distance[edge.to] = nextDistance;
                previous[edge.to] = nodeId;
                previousEdge[edge.to] = edge;
                pq.push(std::make_pair(nextDistance, edge.to));
            }
        }
    }

    if (distance[endId] == std::numeric_limits<double>::infinity()) {
        result.error = "No route found between the selected nodes.";
        return result;
    }

    std::vector<int> reversed;
    for (int current = endId; current != -1; current = previous[current]) {
        reversed.push_back(current);
    }

    std::reverse(reversed.begin(), reversed.end());
    for (std::size_t i = 1; i < reversed.size(); ++i) {
        const int nodeId = reversed[i];
        std::map<int, Edge>::const_iterator edgeIt = previousEdge.find(nodeId);
        if (edgeIt == previousEdge.end()) {
            continue;
        }

        result.segments.push_back(edgeIt->second);
        result.distanceMeters += edgeIt->second.distanceMeters;
        result.travelTimeSeconds += edgeIt->second.travelTimeSeconds;
    }

    result.found = true;
    result.cost = result.travelTimeSeconds;
    result.path = reversed;
    return result;
}

MSTResult PathFinder::buildMST(int startId) const {
    MSTResult result;
    const std::unordered_map<int, Node>& nodes = graph_.getNodes();
    if (nodes.empty()) {
        result.error = "Cannot build MST on an empty graph.";
        return result;
    }

    int root = startId;
    if (root == -1) {
        root = nodes.begin()->first;
    }
    if (!graph_.hasNode(root)) {
        result.error = "MST start node does not exist.";
        return result;
    }

    typedef std::pair<double, Edge> HeapItem;
    struct CompareEdge {
        bool operator()(const HeapItem& a, const HeapItem& b) const {
            return a.first > b.first;
        }
    };

    std::priority_queue<HeapItem, std::vector<HeapItem>, CompareEdge> pq;
    std::set<int> visited;

    visited.insert(root);
    const std::vector<Edge>& startEdges = graph_.getNeighbors(root);
    for (std::size_t i = 0; i < startEdges.size(); ++i) {
        pq.push(std::make_pair(startEdges[i].effectiveWeight(), startEdges[i]));
    }

    while (!pq.empty() && visited.size() < nodes.size()) {
        HeapItem current = pq.top();
        pq.pop();

        const Edge& edge = current.second;
        if (visited.find(edge.to) != visited.end()) {
            continue;
        }

        visited.insert(edge.to);
        result.edges.push_back(std::make_pair(edge.from, edge.to));
        result.totalCost += current.first;

        const std::vector<Edge>& neighbors = graph_.getNeighbors(edge.to);
        for (std::size_t i = 0; i < neighbors.size(); ++i) {
            if (visited.find(neighbors[i].to) == visited.end()) {
                pq.push(std::make_pair(neighbors[i].effectiveWeight(), neighbors[i]));
            }
        }
    }

    if (visited.size() != nodes.size()) {
        result.error = "Graph is disconnected; MST is incomplete.";
        return result;
    }

    result.success = true;
    return result;
}
