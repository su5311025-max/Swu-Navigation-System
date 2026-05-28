#include "ConfigLoader.h"
#include "MySQLManager.h"

#include <exception>
#include <iostream>

int main() {
    try {
        const DBConfig config = ConfigLoader::load("engine/config.ini");
        MySQLManager manager(config);
        manager.connect();

        const Graph graph = manager.loadGraph();
        std::cout << "MySQL connection succeeded." << std::endl;
        std::cout << "Loaded nodes: " << graph.getNodes().size() << std::endl;
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << std::endl;
    }

    return 1;
}
