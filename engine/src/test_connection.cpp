#include "AppPaths.h"
#include "ConfigLoader.h"
#include "MySQLManager.h"

#include <exception>
#include <iostream>

int main(int argc, char* argv[]) {
    try {
        const std::string configPath = resolveConfigPath(argc, argv);
        const DBConfig config = ConfigLoader::load(configPath);
        MySQLManager manager(config);
        manager.connect();

        const Graph graph = manager.loadGraph();
        std::cout << "MySQL connection succeeded." << std::endl;
        std::cout << "Loaded config: " << configPath << std::endl;
        std::cout << "Loaded nodes: " << graph.getNodes().size() << std::endl;
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << std::endl;
    }

    return 1;
}
