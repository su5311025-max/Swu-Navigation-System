#pragma once

#include <fstream>
#include <string>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#endif

inline bool fileExists(const std::string& path) {
    std::ifstream file(path.c_str());
    return file.good();
}

inline std::string parentDirectory(const std::string& path) {
    const std::string::size_type position = path.find_last_of("\\/");
    if (position == std::string::npos) {
        return "";
    }
    return path.substr(0, position);
}

inline std::string joinPath(const std::string& left, const std::string& right) {
    if (left.empty()) {
        return right;
    }
    const char last = left[left.size() - 1];
    if (last == '\\' || last == '/') {
        return left + right;
    }
    return left + "\\" + right;
}

inline std::string executablePath(const char* argv0) {
#ifdef _WIN32
    char buffer[MAX_PATH];
    const DWORD length = GetModuleFileNameA(NULL, buffer, MAX_PATH);
    if (length > 0 && length < MAX_PATH) {
        return std::string(buffer, length);
    }
#endif
    return argv0 != 0 ? std::string(argv0) : "";
}

inline std::string resolveConfigPath(int argc, char* argv[]) {
    if (argc > 1 && argv[1] != 0 && fileExists(argv[1])) {
        return argv[1];
    }

    std::vector<std::string> candidates;
    candidates.push_back("engine\\config.ini");
    candidates.push_back("config.ini");

    const std::string exeDir = parentDirectory(executablePath(argc > 0 ? argv[0] : 0));
    if (!exeDir.empty()) {
        candidates.push_back(joinPath(exeDir, "..\\config.ini"));
        candidates.push_back(joinPath(exeDir, "..\\..\\engine\\config.ini"));
    }

    for (std::size_t i = 0; i < candidates.size(); ++i) {
        if (fileExists(candidates[i])) {
            return candidates[i];
        }
    }

    return "engine\\config.ini";
}
