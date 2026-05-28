<?php

declare(strict_types=1);

require __DIR__ . '/bootstrap.php';

if ($_SERVER['REQUEST_METHOD'] !== 'POST') {
    json_response(false, 'Method not allowed.', [], 405);
}

try {
    $payload = request_json();
    $startLon = isset($payload['start_lon']) ? (float) $payload['start_lon'] : (float) ($_POST['start_lon'] ?? 0);
    $startLat = isset($payload['start_lat']) ? (float) $payload['start_lat'] : (float) ($_POST['start_lat'] ?? 0);
    $endLon = isset($payload['end_lon']) ? (float) $payload['end_lon'] : (float) ($_POST['end_lon'] ?? 0);
    $endLat = isset($payload['end_lat']) ? (float) $payload['end_lat'] : (float) ($_POST['end_lat'] ?? 0);

    if ($startLon == 0.0 || $startLat == 0.0 || $endLon == 0.0 || $endLat == 0.0) {
        json_response(false, 'Please provide valid start and end coordinates.', [], 422);
    }

    $pdo = app_pdo();
    $startNode = nearest_node($pdo, $startLon, $startLat);
    $endNode = nearest_node($pdo, $endLon, $endLat);
    if ($startNode === null || $endNode === null) {
        json_response(false, 'Campus graph is empty. Import map data first.', [], 404);
    }
    if ($startNode['id'] === $endNode['id']) {
        json_response(false, 'Start and end points snapped to the same campus node.', [], 422);
    }

    $statement = $pdo->prepare(
        'INSERT INTO tasks (start_node, end_node, start_lon, start_lat, end_lon, end_lat, status)
         VALUES (?, ?, ?, ?, ?, ?, \'pending\')'
    );
    $statement->execute([
        $startNode['id'],
        $endNode['id'],
        $startLon,
        $startLat,
        $endLon,
        $endLat,
    ]);

    json_response(true, 'Task created.', [
        'task_id' => (int) $pdo->lastInsertId(),
        'start_node' => $startNode,
        'end_node' => $endNode,
        'start_input' => ['lon' => $startLon, 'lat' => $startLat],
        'end_input' => ['lon' => $endLon, 'lat' => $endLat],
    ], 201);
} catch (Throwable $e) {
    json_response(false, 'Failed to create task.', [
        'error' => $e->getMessage(),
    ], 500);
}
