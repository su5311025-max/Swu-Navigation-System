CREATE DATABASE IF NOT EXISTS l_engine
  DEFAULT CHARACTER SET utf8mb4
  DEFAULT COLLATE utf8mb4_unicode_ci;

USE l_engine;

DROP TABLE IF EXISTS tasks;
DROP TABLE IF EXISTS edges;
DROP TABLE IF EXISTS nodes;

CREATE TABLE nodes (
    id INT PRIMARY KEY,
    osm_node_id BIGINT NULL,
    name VARCHAR(128) NOT NULL DEFAULT '',
    lon DOUBLE NOT NULL,
    lat DOUBLE NOT NULL,
    kind VARCHAR(32) NOT NULL DEFAULT 'waypoint',
    UNIQUE KEY uniq_nodes_osm_node_id (osm_node_id),
    INDEX idx_nodes_lat_lon (lat, lon)
);

CREATE TABLE edges (
    id INT PRIMARY KEY AUTO_INCREMENT,
    from_node INT NOT NULL,
    to_node INT NOT NULL,
    distance_m DOUBLE NOT NULL,
    travel_time_sec DOUBLE NOT NULL,
    way_type VARCHAR(32) NOT NULL,
    name VARCHAR(191) NULL,
    geometry_json LONGTEXT NOT NULL,
    CONSTRAINT fk_edges_from_node FOREIGN KEY (from_node) REFERENCES nodes(id),
    CONSTRAINT fk_edges_to_node FOREIGN KEY (to_node) REFERENCES nodes(id),
    INDEX idx_edges_from_node (from_node),
    INDEX idx_edges_to_node (to_node)
);

CREATE TABLE tasks (
    id INT PRIMARY KEY AUTO_INCREMENT,
    start_node INT NOT NULL,
    end_node INT NOT NULL,
    start_lon DOUBLE NOT NULL,
    start_lat DOUBLE NOT NULL,
    end_lon DOUBLE NOT NULL,
    end_lat DOUBLE NOT NULL,
    path_result TEXT NULL,
    route_geometry LONGTEXT NULL,
    distance_m DOUBLE NULL,
    travel_time_sec DOUBLE NULL,
    cost DOUBLE NULL,
    status ENUM('pending', 'processing', 'completed', 'failed') NOT NULL DEFAULT 'pending',
    error_message VARCHAR(255) NULL,
    created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    finished_at TIMESTAMP NULL DEFAULT NULL,
    CONSTRAINT fk_tasks_start FOREIGN KEY (start_node) REFERENCES nodes(id),
    CONSTRAINT fk_tasks_end FOREIGN KEY (end_node) REFERENCES nodes(id),
    INDEX idx_tasks_status (status)
);
