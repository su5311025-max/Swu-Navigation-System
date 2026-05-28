<?php

declare(strict_types=1);

function app_config(): array
{
    static $config = null;
    if ($config === null) {
        $config = require __DIR__ . '/config.php';
    }
    return $config;
}

function app_pdo(): PDO
{
    static $pdo = null;
    if ($pdo instanceof PDO) {
        return $pdo;
    }

    $db = app_config()['db'];
    $dsn = sprintf(
        'mysql:host=%s;port=%s;dbname=%s;charset=%s',
        $db['host'],
        $db['port'],
        $db['name'],
        $db['charset']
    );

    $pdo = new PDO($dsn, $db['user'], $db['password'], [
        PDO::ATTR_ERRMODE => PDO::ERRMODE_EXCEPTION,
        PDO::ATTR_DEFAULT_FETCH_MODE => PDO::FETCH_ASSOC,
    ]);

    return $pdo;
}

function app_map_config(): array
{
    return app_config()['campus'];
}

function nearest_node(PDO $pdo, float $lon, float $lat): ?array
{
    $statement = $pdo->prepare(
        'SELECT id, name, lon, lat,
                ((lat - :lat1) * (lat - :lat2)) +
                (((lon - :lon1) * COS(RADIANS(:lat3))) * ((lon - :lon2) * COS(RADIANS(:lat4)))) AS score
         FROM nodes
         ORDER BY score ASC, id ASC
         LIMIT 1'
    );
    $statement->execute([
        'lat1' => $lat,
        'lat2' => $lat,
        'lat3' => $lat,
        'lat4' => $lat,
        'lon1' => $lon,
        'lon2' => $lon,
    ]);

    $row = $statement->fetch();
    if (!$row) {
        return null;
    }

    return [
        'id' => (int) $row['id'],
        'name' => (string) $row['name'],
        'lon' => (float) $row['lon'],
        'lat' => (float) $row['lat'],
    ];
}

function json_response(bool $success, string $message, array $data = [], int $statusCode = 200): void
{
    http_response_code($statusCode);
    header('Content-Type: application/json; charset=utf-8');
    echo json_encode([
        'success' => $success,
        'message' => $message,
        'data' => $data,
    ], JSON_UNESCAPED_UNICODE);
    exit;
}

function request_json(): array
{
    $raw = file_get_contents('php://input');
    if ($raw === false || trim($raw) === '') {
        return [];
    }

    $decoded = json_decode($raw, true);
    return is_array($decoded) ? $decoded : [];
}
