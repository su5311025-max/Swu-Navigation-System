#pragma once

#include "Graph.h"
#include "Models.h"

#include <string>

#include <mysql.h>

class MySQLManager {
public:
    explicit MySQLManager(const DBConfig& config);
    ~MySQLManager();

    void connect();
    bool isConnected() const;

    Graph loadGraph();
    bool claimNextPendingTask(Task& task);
    void completeTask(int taskId, const PathResult& result);
    void failTask(int taskId, const std::string& errorMessage);

private:
    void execute(const std::string& sql);
    MYSQL_RES* query(const std::string& sql);
    std::string escapeString(const std::string& input);
    std::string buildRouteGeometryJson(const PathResult& result) const;

    DBConfig config_;
    MYSQL* connection_;
};
