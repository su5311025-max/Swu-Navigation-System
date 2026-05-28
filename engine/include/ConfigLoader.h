#pragma once

#include "Models.h"

#include <string>

class ConfigLoader {
public:
    static DBConfig load(const std::string& path);
};
