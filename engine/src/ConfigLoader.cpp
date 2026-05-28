#include "ConfigLoader.h"

#include <fstream>
#include <sstream>
#include <stdexcept>

namespace {
std::string trim(const std::string& input) {
    const std::string whitespace = " \t\r\n";
    const std::size_t begin = input.find_first_not_of(whitespace);
    if (begin == std::string::npos) {
        return "";
    }
    const std::size_t end = input.find_last_not_of(whitespace);
    return input.substr(begin, end - begin + 1);
}
}

DBConfig ConfigLoader::load(const std::string& path) {
    std::ifstream file(path.c_str());
    if (!file.is_open()) {
        throw std::runtime_error("Unable to open config file: " + path);
    }

    DBConfig config;
    std::string line;
    while (std::getline(file, line)) {
        const std::string cleaned = trim(line);
        if (cleaned.empty() || cleaned[0] == '#') {
            continue;
        }

        const std::size_t equalsPos = cleaned.find('=');
        if (equalsPos == std::string::npos) {
            continue;
        }

        const std::string key = trim(cleaned.substr(0, equalsPos));
        const std::string value = trim(cleaned.substr(equalsPos + 1));

        if (key == "db_host") {
            config.host = value;
        } else if (key == "db_port") {
            config.port = std::atoi(value.c_str());
        } else if (key == "db_name") {
            config.database = value;
        } else if (key == "db_user") {
            config.user = value;
        } else if (key == "db_password") {
            config.password = value;
        } else if (key == "poll_interval_ms") {
            config.pollIntervalMs = std::atoi(value.c_str());
        }
    }

    return config;
}
