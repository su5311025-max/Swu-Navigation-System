#pragma once

#include "Graph.h"

class PathFinder {
public:
    explicit PathFinder(const Graph& graph);

    PathResult findShortestPath(int startId, int endId) const;
    MSTResult buildMST(int startId = -1) const;

private:
    const Graph& graph_;
};
