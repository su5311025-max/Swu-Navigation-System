#include "MySQLManager.h"

#include <cstdlib>
#include <cstring>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace {
std::string jsonEscape(const std::string& input) {
    std::ostringstream escaped;
    for (std::size_t i = 0; i < input.size(); ++i) {
        const char ch = input[i];
        switch (ch) {
        case '\\':
            escaped << "\\\\";
            break;
        case '"':
            escaped << "\\\"";
            break;
        case '\b':
            escaped << "\\b";
            break;
        case '\f':
            escaped << "\\f";
            break;
        case '\n':
            escaped << "\\n";
            break;
        case '\r':
            escaped << "\\r";
            break;
        case '\t':
            escaped << "\\t";
            break;
        default:
            escaped << ch;
            break;
        }
    }
    return escaped.str();
}
}

MySQLManager::MySQLManager(const DBConfig& config)
    : config_(config), connection_(0) {
}

MySQLManager::~MySQLManager() {
    if (connection_ != 0) {
        mysql_close(connection_);
        connection_ = 0;
    }
}

void MySQLManager::connect() {
    connection_ = mysql_init(0);
    if (connection_ == 0) {
        throw std::runtime_error("mysql_init failed.");
    }

    const unsigned int port = static_cast<unsigned int>(config_.port);
    if (mysql_real_connect(
            connection_,
            config_.host.c_str(),
            config_.user.c_str(),
            config_.password.c_str(),
            config_.database.c_str(),
            port,
            0,
            0) == 0) {
        const std::string error = mysql_error(connection_);
        mysql_close(connection_);
        connection_ = 0;
        throw std::runtime_error("mysql_real_connect failed: " + error);
    }

    if (mysql_set_character_set(connection_, "utf8mb4") != 0) {
        throw std::runtime_error("Failed to set utf8mb4 charset: " + std::string(mysql_error(connection_)));
    }
}

bool MySQLManager::isConnected() const {
    return connection_ != 0;
}

Graph MySQLManager::loadGraph() {
    if (!isConnected()) {
        throw std::runtime_error("Database connection is not available.");
    }

    Graph graph;

    MYSQL_RES* nodesResult = query("SELECT id, name, lon, lat FROM nodes ORDER BY id");
    MYSQL_ROW nodeRow = 0;
    while ((nodeRow = mysql_fetch_row(nodesResult)) != 0) {
        Node node;
        node.id = std::atoi(nodeRow[0]);
        node.name = nodeRow[1] != 0 ? nodeRow[1] : "";
        node.lon = std::atof(nodeRow[2]);
        node.lat = std::atof(nodeRow[3]);
        graph.addNode(node);
    }
    mysql_free_result(nodesResult);

    MYSQL_RES* edgesResult = query(
        "SELECT id, from_node, to_node, distance_m, travel_time_sec, way_type, name, geometry_json "
        "FROM edges ORDER BY id");
    MYSQL_ROW edgeRow = 0;
    while ((edgeRow = mysql_fetch_row(edgesResult)) != 0) {
        Edge edge;
        edge.id = std::atoi(edgeRow[0]);
        edge.from = std::atoi(edgeRow[1]);
        edge.to = std::atoi(edgeRow[2]);
        edge.distanceMeters = std::atof(edgeRow[3]);
        edge.travelTimeSeconds = std::atof(edgeRow[4]);
        edge.wayType = edgeRow[5] != 0 ? edgeRow[5] : "";
        edge.name = edgeRow[6] != 0 ? edgeRow[6] : "";
        edge.geometryJson = edgeRow[7] != 0 ? edgeRow[7] : "[]";
        graph.addEdge(edge, false);
    }
    mysql_free_result(edgesResult);

    return graph;
}

bool MySQLManager::claimNextPendingTask(Task& task) {
    if (!isConnected()) {
        throw std::runtime_error("Database connection is not available.");
    }

    execute("START TRANSACTION");

    MYSQL_RES* result = 0;
    try {
        result = query(
            "SELECT id, start_node, end_node FROM tasks "
            "WHERE status = 'pending' "
            "ORDER BY created_at ASC, id ASC "
            "LIMIT 1 FOR UPDATE");
    } catch (...) {
        execute("ROLLBACK");
        throw;
    }

    MYSQL_ROW row = mysql_fetch_row(result);
    if (row == 0) {
        mysql_free_result(result);
        execute("COMMIT");
        return false;
    }

    task.id = std::atoi(row[0]);
    task.startNode = std::atoi(row[1]);
    task.endNode = std::atoi(row[2]);
    mysql_free_result(result);

    std::ostringstream updateSql;
    updateSql
        << "UPDATE tasks SET status = 'processing', error_message = NULL "
        << "WHERE id = " << task.id << " AND status = 'pending'";

    try {
        execute(updateSql.str());
        const my_ulonglong affectedRows = mysql_affected_rows(connection_);
        if (affectedRows != 1) {
            execute("ROLLBACK");
            return false;
        }
        execute("COMMIT");
        return true;
    } catch (...) {
        execute("ROLLBACK");
        throw;
    }
}

void MySQLManager::completeTask(int taskId, const PathResult& result) {
    std::ostringstream pathBuffer;
    for (std::size_t i = 0; i < result.path.size(); ++i) {
        if (i > 0) {
            pathBuffer << ",";
        }
        pathBuffer << result.path[i];
    }

    std::ostringstream distanceBuffer;
    distanceBuffer << std::fixed << std::setprecision(3) << result.distanceMeters;

    std::ostringstream timeBuffer;
    timeBuffer << std::fixed << std::setprecision(3) << result.travelTimeSeconds;

    std::ostringstream sql;
    sql
        << "UPDATE tasks "
        << "SET path_result = '" << escapeString(pathBuffer.str()) << "', "
        << "route_geometry = '" << escapeString(buildRouteGeometryJson(result)) << "', "
        << "distance_m = " << distanceBuffer.str() << ", "
        << "travel_time_sec = " << timeBuffer.str() << ", "
        << "cost = " << timeBuffer.str() << ", "
        << "status = 'completed', "
        << "finished_at = NOW(), "
        << "error_message = NULL "
        << "WHERE id = " << taskId;
    execute(sql.str());
}

void MySQLManager::failTask(int taskId, const std::string& errorMessage) {
    std::ostringstream sql;
    sql
        << "UPDATE tasks "
        << "SET status = 'failed', "
        << "path_result = NULL, "
        << "route_geometry = NULL, "
        << "distance_m = NULL, "
        << "travel_time_sec = NULL, "
        << "cost = NULL, "
        << "error_message = '" << escapeString(errorMessage) << "', "
        << "finished_at = NOW() "
        << "WHERE id = " << taskId;
    execute(sql.str());
}

void MySQLManager::execute(const std::string& sql) {
    if (mysql_query(connection_, sql.c_str()) != 0) {
        throw std::runtime_error("MySQL query failed: " + std::string(mysql_error(connection_)) + " SQL: " + sql);
    }
}

MYSQL_RES* MySQLManager::query(const std::string& sql) {
    execute(sql);
    MYSQL_RES* result = mysql_store_result(connection_);
    if (result == 0 && mysql_field_count(connection_) != 0) {
        throw std::runtime_error("Failed to fetch MySQL result: " + std::string(mysql_error(connection_)));
    }
    return result;
}

std::string MySQLManager::escapeString(const std::string& input) {
    if (!isConnected()) {
        throw std::runtime_error("Database connection is not available.");
    }

    std::string escaped;
    escaped.resize(input.size() * 2 + 1);
    const unsigned long escapedLength = mysql_real_escape_string(
        connection_,
        &escaped[0],
        input.c_str(),
        static_cast<unsigned long>(input.size()));
    escaped.resize(escapedLength);
    return escaped;
}

std::string MySQLManager::buildRouteGeometryJson(const PathResult& result) const {
    std::ostringstream buffer;
    buffer << "[";

    for (std::size_t i = 0; i < result.segments.size(); ++i) {
        const Edge& edge = result.segments[i];
        if (i > 0) {
            buffer << ",";
        }

        buffer
            << "{\"edge_id\":" << edge.id
            << ",\"from_node\":" << edge.from
            << ",\"to_node\":" << edge.to
            << ",\"distance_m\":" << edge.distanceMeters
            << ",\"travel_time_sec\":" << edge.travelTimeSeconds
            << ",\"way_type\":\"" << jsonEscape(edge.wayType) << "\""
            << ",\"name\":\"" << jsonEscape(edge.name) << "\""
            << ",\"geometry\":" << edge.geometryJson
            << "}";
    }

    buffer << "]";
    return buffer.str();
}
