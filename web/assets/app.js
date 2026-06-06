const state = {
  graph: null,
  campus: null,
  map: null,
  tileLayer: null,
  campusLayer: null,
  roadLayer: null,
  selectionLayer: null,
  routeLayer: null,
  tileWarning: null,
  tilesFailed: false,
  currentTaskId: null,
  pollingHandle: null,
  pollingAttempts: 0,
  startSelection: null,
  endSelection: null,
  startMarker: null,
  endMarker: null,
};

const submitButton = document.getElementById("submit-task");
const taskMeta = document.getElementById("task-meta");
const routeOutput = document.getElementById("route-output");
const startName = document.getElementById("start-node-name");
const startCoords = document.getElementById("start-node-coords");
const endName = document.getElementById("end-node-name");
const endCoords = document.getElementById("end-node-coords");

function currentSelectionMode() {
  const checked = document.querySelector('input[name="selectionMode"]:checked');
  return checked ? checked.value : "start";
}

function setTaskMeta(html) {
  taskMeta.innerHTML = html;
}

function setRouteText(text) {
  routeOutput.textContent = text;
}

async function readJson(response) {
  let payload = null;
  try {
    payload = await response.json();
  } catch (error) {
    throw new Error(`服务器返回了无法解析的响应。HTTP ${response.status}`);
  }

  if (!response.ok) {
    throw new Error(payload.data?.error || payload.message || `请求失败。HTTP ${response.status}`);
  }

  return payload;
}

function formatCoordinate(lat, lon) {
  return `${lat.toFixed(6)}, ${lon.toFixed(6)}`;
}

function formatDistance(distanceMeters) {
  if (distanceMeters >= 1000) {
    return `${(distanceMeters / 1000).toFixed(2)} km`;
  }
  return `${Math.round(distanceMeters)} m`;
}

function formatDuration(seconds) {
  const minutes = seconds / 60;
  if (minutes < 1) {
    return `${Math.round(seconds)} 秒`;
  }
  if (minutes < 60) {
    return `${minutes.toFixed(1)} 分钟`;
  }
  const hours = Math.floor(minutes / 60);
  const remainMinutes = Math.round(minutes % 60);
  return `${hours} 小时 ${remainMinutes} 分钟`;
}

function markerStyle(kind) {
  if (kind === "start") {
    return { color: "#ffffff", fillColor: "#4d6a4f" };
  }
  return { color: "#ffffff", fillColor: "#ab3f39" };
}

function upsertSelectionMarker(kind, lat, lon) {
  const markerKey = kind === "start" ? "startMarker" : "endMarker";
  const styles = markerStyle(kind);
  if (!state[markerKey]) {
    state[markerKey] = L.circleMarker([lat, lon], {
      radius: 9,
      weight: 3,
      fillOpacity: 1,
      ...styles,
    }).addTo(state.selectionLayer);
    return;
  }
  state[markerKey].setLatLng([lat, lon]);
  state[markerKey].setStyle(styles);
}

function syncSelectionCards() {
  if (state.startSelection) {
    startName.textContent = state.startSelection.label;
    startCoords.textContent = formatCoordinate(state.startSelection.lat, state.startSelection.lon);
  } else {
    startName.textContent = "尚未选择";
    startCoords.textContent = "点击地图设置起点";
  }

  if (state.endSelection) {
    endName.textContent = state.endSelection.label;
    endCoords.textContent = formatCoordinate(state.endSelection.lat, state.endSelection.lon);
  } else {
    endName.textContent = "尚未选择";
    endCoords.textContent = "点击地图设置终点";
  }
}

function parseGeometry(geometry) {
  if (!geometry) return [];
  if (Array.isArray(geometry)) return geometry;
  if (typeof geometry === "string") {
    try {
      const parsed = JSON.parse(geometry);
      return Array.isArray(parsed) ? parsed : [];
    } catch (error) {
      return [];
    }
  }
  return [];
}

function toLatLngs(geometry) {
  return parseGeometry(geometry)
    .map((point) => {
      if (Array.isArray(point) && point.length >= 2) {
        return [Number(point[1]), Number(point[0])];
      }
      if (point && typeof point === "object" && "lat" in point && "lon" in point) {
        return [Number(point.lat), Number(point.lon)];
      }
      return null;
    })
    .filter(Boolean);
}

function flattenRouteSegments(routeSegments) {
  const points = [];
  if (!Array.isArray(routeSegments)) return points;

  routeSegments.forEach((segment) => {
    const segmentPoints = toLatLngs(segment.geometry);
    segmentPoints.forEach((point, index) => {
      const isDuplicate =
        points.length > 0 &&
        index === 0 &&
        points[points.length - 1][0] === point[0] &&
        points[points.length - 1][1] === point[1];
      if (!isDuplicate) {
        points.push(point);
      }
    });
  });
  return points;
}

function showTileFallback() {
  if (state.tilesFailed) return;

  state.tilesFailed = true;
  document.getElementById("map").classList.add("map-tiles-failed");

  const TileWarning = L.Control.extend({
    options: { position: "bottomleft" },
    onAdd() {
      const container = L.DomUtil.create("div", "tile-warning");
      container.textContent = "在线底图暂不可用，已使用本地校园底色。";
      return container;
    },
  });

  state.tileWarning = new TileWarning();
  state.tileWarning.addTo(state.map);
}

function drawCampusBackdrop(campus) {
  state.campusLayer.clearLayers();
  if (!campus?.bounds || campus.bounds.length !== 2) return;

  const southWest = campus.bounds[0];
  const northEast = campus.bounds[1];
  const bounds = [
    [southWest.lat, southWest.lon],
    [northEast.lat, northEast.lon],
  ];

  L.rectangle(bounds, {
    color: "#7d8f68",
    weight: 1,
    fillColor: "#eef5df",
    fillOpacity: 0.22,
    interactive: false,
  }).addTo(state.campusLayer);

  const latStep = 0.0025;
  const lonStep = 0.003;
  for (let lat = southWest.lat + latStep; lat < northEast.lat; lat += latStep) {
    L.polyline(
      [
        [lat, southWest.lon],
        [lat, northEast.lon],
      ],
      { color: "#9caf88", weight: 0.7, opacity: 0.18, interactive: false }
    ).addTo(state.campusLayer);
  }

  for (let lon = southWest.lon + lonStep; lon < northEast.lon; lon += lonStep) {
    L.polyline(
      [
        [southWest.lat, lon],
        [northEast.lat, lon],
      ],
      { color: "#9caf88", weight: 0.7, opacity: 0.18, interactive: false }
    ).addTo(state.campusLayer);
  }
}

function buildMap() {
  state.map = L.map("map", {
    zoomControl: true,
    minZoom: 14,
    maxZoom: 19,
  });

  state.tileLayer = L.tileLayer("https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png", {
    maxZoom: 19,
    attribution: '&copy; <a href="https://www.openstreetmap.org/copyright">OpenStreetMap</a>',
  })
    .on("tileerror", showTileFallback)
    .addTo(state.map);

  state.campusLayer = L.layerGroup().addTo(state.map);
  state.roadLayer = L.layerGroup().addTo(state.map);
  state.selectionLayer = L.layerGroup().addTo(state.map);
  state.routeLayer = L.layerGroup().addTo(state.map);
}

function drawGraph(graph) {
  state.graph = graph;
  state.campus = graph.campus;
  drawCampusBackdrop(graph.campus);
  state.roadLayer.clearLayers();

  graph.edges.forEach((edge) => {
    const latLngs = toLatLngs(edge.geometry_json);
    if (latLngs.length < 2) return;

    L.polyline(latLngs, {
      color: "#2f6f8f",
      weight: 2.2,
      opacity: 0.58,
    }).addTo(state.roadLayer);
  });

  if (graph.campus?.bounds?.length === 2) {
    const southWest = graph.campus.bounds[0];
    const northEast = graph.campus.bounds[1];
    state.map.fitBounds([
      [southWest.lat, southWest.lon],
      [northEast.lat, northEast.lon],
    ]);
  } else if (graph.campus?.center) {
    state.map.setView([graph.campus.center.lat, graph.campus.center.lon], graph.campus.zoom || 16);
  }
}

function clearRoute() {
  state.routeLayer.clearLayers();
}

function clearSelections() {
  state.selectionLayer.clearLayers();
  state.startMarker = null;
  state.endMarker = null;
}

function drawRoute(routeSegments) {
  clearRoute();
  const routePoints = flattenRouteSegments(routeSegments);
  if (routePoints.length < 2) return;

  L.polyline(routePoints, {
    color: "#d96b2b",
    weight: 5,
    opacity: 0.95,
  }).addTo(state.routeLayer);

  if (state.startSelection) {
    upsertSelectionMarker("start", state.startSelection.lat, state.startSelection.lon);
  }
  if (state.endSelection) {
    upsertSelectionMarker("end", state.endSelection.lat, state.endSelection.lon);
  }
}

async function loadGraph() {
  const response = await fetch("./get_graph.php");
  const payload = await readJson(response);
  if (!payload.success) {
    throw new Error(payload.message || "Failed to load campus graph.");
  }
  drawGraph(payload.data);
}

function updateSelection(mode, latlng) {
  stopPolling();
  state.currentTaskId = null;
  state.pollingAttempts = 0;
  clearRoute();

  const selection = {
    label: mode === "start" ? "起点候选" : "终点候选",
    lat: latlng.lat,
    lon: latlng.lng,
  };

  if (mode === "start") {
    state.startSelection = selection;
    upsertSelectionMarker("start", selection.lat, selection.lon);
  } else {
    state.endSelection = selection;
    upsertSelectionMarker("end", selection.lat, selection.lon);
  }

  syncSelectionCards();
  setTaskMeta("<p>选点已更新。请继续选择另一个端点，或直接提交路径任务。</p>");
  setRouteText("等待新的路径任务...");
}

async function submitTask() {
  if (!state.startSelection || !state.endSelection) {
    setTaskMeta("<p>请先在地图上选择起点和终点。</p>");
    return;
  }

  clearRoute();
  upsertSelectionMarker("start", state.startSelection.lat, state.startSelection.lon);
  upsertSelectionMarker("end", state.endSelection.lat, state.endSelection.lon);

  setTaskMeta("<p>任务创建中，正在将点击位置吸附到校园步行路网...</p>");
  setRouteText("任务已提交，等待 C++ 引擎计算最优步行路径。");

  const response = await fetch("./submit_task.php", {
    method: "POST",
    headers: {
      "Content-Type": "application/json",
    },
    body: JSON.stringify({
      start_lon: state.startSelection.lon,
      start_lat: state.startSelection.lat,
      end_lon: state.endSelection.lon,
      end_lat: state.endSelection.lat,
    }),
  });

  const payload = await readJson(response);
  if (!payload.success) {
    setTaskMeta(`<p>${payload.message}</p>`);
    setRouteText(payload.data?.error || "任务创建失败。");
    return;
  }

  const startNode = payload.data.start_node;
  const endNode = payload.data.end_node;
  state.startSelection = {
    label: startNode.name || `起点节点 #${startNode.id}`,
    lat: Number(startNode.lat),
    lon: Number(startNode.lon),
  };
  state.endSelection = {
    label: endNode.name || `终点节点 #${endNode.id}`,
    lat: Number(endNode.lat),
    lon: Number(endNode.lon),
  };

  upsertSelectionMarker("start", state.startSelection.lat, state.startSelection.lon);
  upsertSelectionMarker("end", state.endSelection.lat, state.endSelection.lon);
  syncSelectionCards();

  state.currentTaskId = payload.data.task_id;
  setTaskMeta(
    `<p>任务 #${state.currentTaskId} 已创建。</p><p>起点吸附节点: ${startNode.id}，终点吸附节点: ${endNode.id}</p>`
  );
  startPolling();
}

function stopPolling() {
  if (state.pollingHandle) {
    window.clearInterval(state.pollingHandle);
    state.pollingHandle = null;
  }
}

function handlePollingError(error) {
  setRouteText(error.message || "状态查询失败。");
  stopPolling();
}

async function pollTask() {
  if (!state.currentTaskId) return;
  state.pollingAttempts += 1;

  const response = await fetch(`./check_status.php?task_id=${state.currentTaskId}`);
  const payload = await readJson(response);
  if (!payload.success) {
    setRouteText(payload.data?.error || payload.message || "状态查询失败。");
    stopPolling();
    return;
  }

  const task = payload.data.task;
  setTaskMeta(
    `<p>任务 #${task.id}</p><p>状态: <strong>${task.status}</strong></p><p>创建时间: ${task.created_at}</p>`
  );

  if (task.status === "completed") {
    const distanceText = task.distance_m ? formatDistance(task.distance_m) : "未知";
    const durationText = task.travel_time_sec ? formatDuration(task.travel_time_sec) : "未知";
    setRouteText(
      `最优步行路径已生成。\n总距离: ${distanceText}\n预计时间: ${durationText}\n路径节点数: ${task.path.length}`
    );
    drawRoute(task.route_geometry || []);
    stopPolling();
  } else if (task.status === "failed") {
    setRouteText(`任务失败: ${task.error_message || "未知错误"}`);
    stopPolling();
  } else {
    const engineHint =
      state.pollingAttempts > 25 ? "\n如果长时间没有完成，请确认 engine\\bin\\l_engine.exe 正在运行。" : "";
    setRouteText(`任务正在处理中，请稍候...${engineHint}`);
  }
}

function startPolling() {
  stopPolling();
  state.pollingAttempts = 0;
  pollTask().catch(handlePollingError);
  state.pollingHandle = window.setInterval(() => {
    pollTask().catch(handlePollingError);
  }, 1200);
}

function bindEvents() {
  state.map.on("click", (event) => {
    updateSelection(currentSelectionMode(), event.latlng);
  });

  submitButton.addEventListener("click", () => {
    submitTask().catch((error) => {
      setTaskMeta("<p>提交任务时发生错误。</p>");
      setRouteText(error.message);
      stopPolling();
    });
  });
}

async function init() {
  buildMap();
  clearSelections();
  bindEvents();
  try {
    await loadGraph();
    setTaskMeta("<p>校园步行路网已加载。请点击地图选择起点和终点。</p>");
  } catch (error) {
    setTaskMeta("<p>校园路网加载失败。</p>");
    setRouteText(error.message);
  }
}

init();
