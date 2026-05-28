#include "ConfigLoader.h"
#include "MySQLManager.h"
#include "PathFinder.h"

#include <chrono>
#include <exception>
#include <iostream>
#include <thread>

int main() {
    try {
        const DBConfig config = ConfigLoader::load("engine/config.ini");
        MySQLManager manager(config);
        manager.connect();

        std::cout << "Connected to MySQL. Loading graph..." << std::endl;
        const Graph graph = manager.loadGraph();
        const PathFinder pathFinder(graph);
        std::cout << "Graph ready. Node count: " << graph.getNodes().size() << std::endl;

        while (true) {
            try {
                Task task;
                if (manager.claimNextPendingTask(task)) {
                    std::cout << "Processing task #" << task.id
                              << " from " << task.startNode
                              << " to " << task.endNode << std::endl;

                    const PathResult result = pathFinder.findShortestPath(task.startNode, task.endNode);
                    if (result.found) {
                        manager.completeTask(task.id, result);
                        std::cout << "Task #" << task.id
                                  << " completed. Distance: " << result.distanceMeters
                                  << "m, ETA: " << result.travelTimeSeconds
                                  << "s" << std::endl;
                    } else {
                        manager.failTask(task.id, result.error);
                        std::cout << "Task #" << task.id << " failed: " << result.error << std::endl;
                    }
                }
            } catch (const std::exception& ex) {
                std::cerr << "Processing error: " << ex.what() << std::endl;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(config.pollIntervalMs));
        }
    } catch (const std::exception& ex) {
        std::cerr << "Startup error: " << ex.what() << std::endl;
        return 1;
    }
}
