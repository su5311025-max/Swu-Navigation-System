<?php
declare(strict_types=1);
?>
<!DOCTYPE html>
<html lang="zh-CN">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>西南大学步行导航</title>
    <link
        rel="stylesheet"
        href="https://unpkg.com/leaflet@1.9.4/dist/leaflet.css"
        integrity="sha256-p4NxAoJBhIIN+hmNHrzRCf9tD/miZyoHS5obTRR9BMY="
        crossorigin=""
    >
    <link rel="stylesheet" href="./assets/styles.css">
</head>
<body class="page-shell">
    <main class="layout">
        <section class="panel panel-map">
            <div class="panel-head">
                <div>
                    <p class="eyebrow">L-Engine Campus</p>
                    <h1>西南大学校内步行导航</h1>
                    <p class="panel-copy">点击地图设置起点和终点，系统会自动吸附到最近的校园步行路网节点，并按预计步行时间规划路线。</p>
                </div>
                <a class="ghost-link" href="./dashboard.php">查看调度面板</a>
            </div>
            <div id="map"></div>
        </section>

        <aside class="panel panel-controls">
            <section class="card">
                <h2>创建任务</h2>
                <p>先切换录入模式，再在地图上点击对应位置。无需手动选择大量节点。</p>

                <div class="mode-switch">
                    <label><input type="radio" name="selectionMode" value="start" checked> 选择起点</label>
                    <label><input type="radio" name="selectionMode" value="end"> 选择终点</label>
                </div>

                <div class="selection-grid">
                    <article class="selection-card">
                        <span class="selection-label">起点</span>
                        <strong id="start-node-name">尚未选择</strong>
                        <small id="start-node-coords">点击地图设置起点</small>
                    </article>
                    <article class="selection-card">
                        <span class="selection-label">终点</span>
                        <strong id="end-node-name">尚未选择</strong>
                        <small id="end-node-coords">点击地图设置终点</small>
                    </article>
                </div>

                <button id="submit-task" type="button">提交步行路径任务</button>
            </section>

            <section class="card">
                <h2>任务状态</h2>
                <div id="task-meta">
                    <p>地图加载后即可开始选择起点和终点。</p>
                </div>
                <pre id="route-output">等待任务结果...</pre>
            </section>

            <section class="card">
                <h2>图层说明</h2>
                <ul class="legend">
                    <li><span class="dot dot-blue"></span>校园步行路网</li>
                    <li><span class="dot dot-green"></span>起点</li>
                    <li><span class="dot dot-red"></span>终点</li>
                    <li><span class="line line-orange"></span>最优步行路线</li>
                </ul>
            </section>
        </aside>
    </main>

    <script
        src="https://unpkg.com/leaflet@1.9.4/dist/leaflet.js"
        integrity="sha256-20nQCchB9co0qIjJZRGuk2/Z9VM+kNiyxNV1lvTlZBo="
        crossorigin=""
    ></script>
    <script src="./assets/app.js"></script>
</body>
</html>
