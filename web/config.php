<?php

return [
    'db' => [
        'host' => getenv('L_ENGINE_DB_HOST') ?: '127.0.0.1',
        'port' => getenv('L_ENGINE_DB_PORT') ?: '3307',
        'name' => getenv('L_ENGINE_DB_NAME') ?: 'l_engine',
        'user' => getenv('L_ENGINE_DB_USER') ?: 'root',
        'password' => getenv('L_ENGINE_DB_PASSWORD') ?: 'mysql',
        'charset' => 'utf8mb4',
    ],
    'campus' => [
        'name' => '西南大学',
        'center' => [
            'lat' => 29.8213,
            'lon' => 106.4195,
        ],
        'zoom' => 16,
        'bounds' => [
            ['lat' => 29.8118, 'lon' => 106.4075],
            ['lat' => 29.8308, 'lon' => 106.4315],
        ],
    ],
];
