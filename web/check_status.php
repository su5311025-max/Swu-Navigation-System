<?php

declare(strict_types=1);

require __DIR__ . '/bootstrap.php';

try {
    $taskId = isset($_GET['task_id']) ? (int) $_GET['task_id'] : 0;
    if ($taskId <= 0) {
        json_response(false, 'task_id is required.', [], 422);
    }

    $pdo = app_pdo();
    $statement = $pdo->prepare(
        'SELECT id, start_node, end_node, start_lon, start_lat, end_lon, end_lat,
                path_result, route_geometry, distance_m, travel_time_sec, cost,
                status, error_message, created_at, finished_at
         FROM tasks
         WHERE id = ?'
    );
    $statement->execute([$taskId]);
    $task = $statement->fetch();

    if (!$task) {
        json_response(false, 'Task not found.', [], 404);
    }

    $path = [];
    if (!empty($task['path_result'])) {
        foreach (explode(',', $task['path_result']) as $nodeId) {
            $path[] = (int) $nodeId;
        }
    }

    $routeGeometry = [];
    if (!empty($task['route_geometry'])) {
        $decoded = json_decode((string) $task['route_geometry'], true);
        if (is_array($decoded)) {
            $routeGeometry = $decoded;
        }
    }

    json_response(true, 'Task loaded.', [
        'task' => [
            'id' => (int) $task['id'],
            'start_node' => (int) $task['start_node'],
            'end_node' => (int) $task['end_node'],
            'start_lon' => (float) $task['start_lon'],
            'start_lat' => (float) $task['start_lat'],
            'end_lon' => (float) $task['end_lon'],
            'end_lat' => (float) $task['end_lat'],
            'status' => $task['status'],
            'cost' => $task['cost'] !== null ? (float) $task['cost'] : null,
            'distance_m' => $task['distance_m'] !== null ? (float) $task['distance_m'] : null,
            'travel_time_sec' => $task['travel_time_sec'] !== null ? (float) $task['travel_time_sec'] : null,
            'path_result' => $task['path_result'],
            'path' => $path,
            'route_geometry' => $routeGeometry,
            'error_message' => $task['error_message'],
            'created_at' => $task['created_at'],
            'finished_at' => $task['finished_at'],
        ],
    ]);
} catch (Throwable $e) {
    json_response(false, 'Failed to check task status.', [
        'error' => $e->getMessage(),
    ], 500);
}
