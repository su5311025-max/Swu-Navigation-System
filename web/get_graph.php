<?php

declare(strict_types=1);

require __DIR__ . '/bootstrap.php';

try {
    $pdo = app_pdo();

    $nodes = $pdo->query('SELECT id, name, lon, lat FROM nodes ORDER BY id')->fetchAll();
    $edges = $pdo->query(
        'SELECT id, from_node, to_node, distance_m, travel_time_sec, way_type, name, geometry_json
         FROM edges
         ORDER BY id'
    )->fetchAll();

    json_response(true, 'Graph loaded.', [
        'nodes' => $nodes,
        'edges' => $edges,
        'campus' => app_map_config(),
    ]);
} catch (Throwable $e) {
    json_response(false, 'Failed to load graph.', [
        'error' => $e->getMessage(),
    ], 500);
}
