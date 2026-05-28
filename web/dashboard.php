<?php

declare(strict_types=1);

require __DIR__ . '/bootstrap.php';

$stats = [
    'nodes' => 0,
    'edges' => 0,
    'tasks' => 0,
    'pending' => 0,
    'completed' => 0,
    'failed' => 0,
];
$recentTasks = [];
$error = null;

try {
    $pdo = app_pdo();

    $stats['nodes'] = (int) $pdo->query('SELECT COUNT(*) FROM nodes')->fetchColumn();
    $stats['edges'] = (int) $pdo->query('SELECT COUNT(*) FROM edges')->fetchColumn();
    $stats['tasks'] = (int) $pdo->query('SELECT COUNT(*) FROM tasks')->fetchColumn();
    $stats['pending'] = (int) $pdo->query("SELECT COUNT(*) FROM tasks WHERE status = 'pending'")->fetchColumn();
    $stats['completed'] = (int) $pdo->query("SELECT COUNT(*) FROM tasks WHERE status = 'completed'")->fetchColumn();
    $stats['failed'] = (int) $pdo->query("SELECT COUNT(*) FROM tasks WHERE status = 'failed'")->fetchColumn();

    $statement = $pdo->query(
        'SELECT id, start_node, end_node, status, distance_m, travel_time_sec, error_message, created_at, finished_at
         FROM tasks
         ORDER BY id DESC
         LIMIT 10'
    );
    $recentTasks = $statement->fetchAll();
} catch (Throwable $e) {
    $error = $e->getMessage();
}
?>
<!DOCTYPE html>
<html lang="zh-CN">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>西南大学步行导航面板</title>
    <link rel="stylesheet" href="./assets/styles.css">
</head>
<body class="dashboard-shell">
    <main class="dashboard">
        <header class="dashboard-head">
            <div>
                <p class="eyebrow">Operations</p>
                <h1>西南大学步行导航概览</h1>
            </div>
            <div class="head-links">
                <a class="ghost-link" href="./index.php">返回地图</a>
                <button type="button" class="ghost-link button-reset" onclick="window.location.reload()">刷新</button>
            </div>
        </header>

        <?php if ($error !== null): ?>
            <section class="card error-box">
                <h2>数据库连接失败</h2>
                <p><?= htmlspecialchars($error, ENT_QUOTES, 'UTF-8') ?></p>
            </section>
        <?php else: ?>
            <section class="stats-grid">
                <article class="card stat-card"><span>节点数</span><strong><?= $stats['nodes'] ?></strong></article>
                <article class="card stat-card"><span>边数</span><strong><?= $stats['edges'] ?></strong></article>
                <article class="card stat-card"><span>任务总数</span><strong><?= $stats['tasks'] ?></strong></article>
                <article class="card stat-card"><span>等待中</span><strong><?= $stats['pending'] ?></strong></article>
                <article class="card stat-card"><span>已完成</span><strong><?= $stats['completed'] ?></strong></article>
                <article class="card stat-card"><span>失败</span><strong><?= $stats['failed'] ?></strong></article>
            </section>

            <section class="card">
                <h2>最近 10 条任务</h2>
                <div class="table-wrap">
                    <table>
                        <thead>
                            <tr>
                                <th>ID</th>
                                <th>起点节点</th>
                                <th>终点节点</th>
                                <th>状态</th>
                                <th>距离</th>
                                <th>预计时间</th>
                                <th>错误</th>
                                <th>创建时间</th>
                                <th>完成时间</th>
                            </tr>
                        </thead>
                        <tbody>
                        <?php foreach ($recentTasks as $task): ?>
                            <tr>
                                <td><?= (int) $task['id'] ?></td>
                                <td><?= (int) $task['start_node'] ?></td>
                                <td><?= (int) $task['end_node'] ?></td>
                                <td><?= htmlspecialchars($task['status'], ENT_QUOTES, 'UTF-8') ?></td>
                                <td><?= $task['distance_m'] !== null ? number_format((float) $task['distance_m'], 0) . ' m' : '-' ?></td>
                                <td><?= $task['travel_time_sec'] !== null ? number_format((float) $task['travel_time_sec'] / 60, 1) . ' min' : '-' ?></td>
                                <td><?= htmlspecialchars((string) ($task['error_message'] ?? '-'), ENT_QUOTES, 'UTF-8') ?></td>
                                <td><?= htmlspecialchars($task['created_at'], ENT_QUOTES, 'UTF-8') ?></td>
                                <td><?= htmlspecialchars((string) ($task['finished_at'] ?? '-'), ENT_QUOTES, 'UTF-8') ?></td>
                            </tr>
                        <?php endforeach; ?>
                        </tbody>
                    </table>
                </div>
            </section>
        <?php endif; ?>
    </main>
</body>
</html>
