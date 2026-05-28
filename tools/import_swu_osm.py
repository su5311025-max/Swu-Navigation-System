#!/usr/bin/env python3
"""Import Southwest University campus walking roads from an OSM XML export."""

from __future__ import annotations

import argparse
import json
import math
import os
import subprocess
import sys
import xml.etree.ElementTree as ET
from dataclasses import dataclass
from pathlib import Path


ALLOWED_HIGHWAYS = {
    "footway",
    "path",
    "pedestrian",
    "living_street",
    "service",
    "residential",
    "unclassified",
    "tertiary",
    "steps",
}

DISALLOWED_HIGHWAYS = {
    "motorway",
    "motorway_link",
    "trunk",
    "trunk_link",
    "construction",
}

SPEED_MPS = {
    "steps": 0.75,
    "footway": 1.35,
    "path": 1.35,
    "pedestrian": 1.35,
    "service": 1.25,
    "living_street": 1.25,
    "residential": 1.25,
    "unclassified": 1.25,
    "tertiary": 1.25,
}


@dataclass
class EdgeRow:
    from_node: str
    to_node: str
    distance_m: float
    travel_time_sec: float
    way_type: str
    name: str
    geometry_json: str


def parse_args() -> argparse.Namespace:
    root = Path(__file__).resolve().parents[1]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--input", required=True, help="Path to an .osm XML export for the campus area.")
    parser.add_argument("--schema", default=str(root / "sql" / "schema.sql"))
    parser.add_argument("--boundary", default=str(root / "data" / "southwest_university_boundary.geojson"))
    parser.add_argument("--mysql-bin", default=os.getenv("L_ENGINE_MYSQL_BIN", ""))
    parser.add_argument("--db-host", default=os.getenv("L_ENGINE_DB_HOST", "127.0.0.1"))
    parser.add_argument("--db-port", default=os.getenv("L_ENGINE_DB_PORT", "3307"))
    parser.add_argument("--db-name", default=os.getenv("L_ENGINE_DB_NAME", "l_engine"))
    parser.add_argument("--db-user", default=os.getenv("L_ENGINE_DB_USER", "root"))
    parser.add_argument("--db-password", default=os.getenv("L_ENGINE_DB_PASSWORD", "mysql"))
    return parser.parse_args()


def load_boundary(boundary_path: Path) -> list[tuple[float, float]]:
    payload = json.loads(boundary_path.read_text(encoding="utf-8"))
    coordinates = payload["features"][0]["geometry"]["coordinates"][0]
    return [(float(lon), float(lat)) for lon, lat in coordinates]


def point_in_polygon(lon: float, lat: float, polygon: list[tuple[float, float]]) -> bool:
    inside = False
    j = len(polygon) - 1
    for i in range(len(polygon)):
        xi, yi = polygon[i]
        xj, yj = polygon[j]
        intersects = ((yi > lat) != (yj > lat)) and (
            lon < (xj - xi) * (lat - yi) / ((yj - yi) or 1e-12) + xi
        )
        if intersects:
            inside = not inside
        j = i
    return inside


def haversine_meters(lon1: float, lat1: float, lon2: float, lat2: float) -> float:
    radius = 6371000.0
    phi1 = math.radians(lat1)
    phi2 = math.radians(lat2)
    delta_phi = math.radians(lat2 - lat1)
    delta_lambda = math.radians(lon2 - lon1)

    a = (
        math.sin(delta_phi / 2.0) ** 2
        + math.cos(phi1) * math.cos(phi2) * math.sin(delta_lambda / 2.0) ** 2
    )
    c = 2.0 * math.atan2(math.sqrt(a), math.sqrt(1.0 - a))
    return radius * c


def is_walkable(tags: dict[str, str]) -> bool:
    highway = tags.get("highway", "")
    if highway in DISALLOWED_HIGHWAYS:
        return False
    if highway not in ALLOWED_HIGHWAYS:
        return False
    if tags.get("area") == "yes":
        return False
    if tags.get("foot") == "no":
        return False
    if tags.get("access") == "private" and tags.get("foot") not in {"yes", "permissive", "designated"}:
        return False
    return True


def is_bidirectional_for_foot(tags: dict[str, str]) -> bool:
    if tags.get("oneway:foot") == "yes":
        return False
    if tags.get("foot:backward") == "no":
        return False
    if tags.get("oneway") == "yes" and tags.get("foot") not in {"yes", "designated", "permissive"}:
        return False
    return True


def pick_speed_mps(highway: str) -> float:
    return SPEED_MPS.get(highway, 1.25)


def sql_string(value: str) -> str:
    return value.replace("\\", "\\\\").replace("'", "\\'")


def chunked(items: list[str], size: int) -> list[list[str]]:
    return [items[index:index + size] for index in range(0, len(items), size)]


def find_mysql_binary(explicit_path: str) -> str:
    if explicit_path:
        return explicit_path

    candidates = [
        r"D:\ampps\Ampps\mysql\bin\mysql.exe",
        r"C:\ampps\Ampps\mysql\bin\mysql.exe",
        "mysql",
    ]
    for candidate in candidates:
        if candidate == "mysql" or Path(candidate).exists():
            return candidate
    raise FileNotFoundError("Unable to locate mysql.exe. Pass --mysql-bin explicitly.")


def load_osm_nodes(osm_path: Path) -> dict[str, tuple[float, float]]:
    tree = ET.parse(osm_path)
    root = tree.getroot()
    nodes: dict[str, tuple[float, float]] = {}
    for node in root.findall("node"):
        node_id = node.attrib["id"]
        lon = float(node.attrib["lon"])
        lat = float(node.attrib["lat"])
        nodes[node_id] = (lon, lat)
    return nodes


def load_walk_edges(osm_path: Path, polygon: list[tuple[float, float]]) -> tuple[dict[str, tuple[float, float]], list[EdgeRow]]:
    tree = ET.parse(osm_path)
    root = tree.getroot()
    nodes = load_osm_nodes(osm_path)

    used_osm_nodes: dict[str, tuple[float, float]] = {}
    edges: list[EdgeRow] = []

    for way in root.findall("way"):
        tags = {tag.attrib["k"]: tag.attrib["v"] for tag in way.findall("tag")}
        if not is_walkable(tags):
            continue

        refs = [nd.attrib["ref"] for nd in way.findall("nd") if nd.attrib["ref"] in nodes]
        if len(refs) < 2:
            continue

        speed_mps = pick_speed_mps(tags.get("highway", ""))
        is_bidirectional = is_bidirectional_for_foot(tags)
        way_type = tags.get("highway", "path")
        way_name = tags.get("name", "")

        for index in range(len(refs) - 1):
            from_ref = refs[index]
            to_ref = refs[index + 1]
            lon1, lat1 = nodes[from_ref]
            lon2, lat2 = nodes[to_ref]
            if not point_in_polygon(lon1, lat1, polygon) or not point_in_polygon(lon2, lat2, polygon):
                continue
            distance_m = haversine_meters(lon1, lat1, lon2, lat2)
            if distance_m <= 0:
                continue

            used_osm_nodes[from_ref] = nodes[from_ref]
            used_osm_nodes[to_ref] = nodes[to_ref]

            geometry_json = json.dumps([[lon1, lat1], [lon2, lat2]], ensure_ascii=False)
            travel_time_sec = distance_m / speed_mps

            edges.append(
                EdgeRow(
                    from_node=from_ref,
                    to_node=to_ref,
                    distance_m=distance_m,
                    travel_time_sec=travel_time_sec,
                    way_type=way_type,
                    name=way_name,
                    geometry_json=geometry_json,
                )
            )

            if is_bidirectional:
                reverse_geometry = json.dumps([[lon2, lat2], [lon1, lat1]], ensure_ascii=False)
                edges.append(
                    EdgeRow(
                        from_node=to_ref,
                        to_node=from_ref,
                        distance_m=distance_m,
                        travel_time_sec=travel_time_sec,
                        way_type=way_type,
                        name=way_name,
                        geometry_json=reverse_geometry,
                    )
                )

    return used_osm_nodes, edges


def build_import_sql(
    used_osm_nodes: dict[str, tuple[float, float]],
    edges: list[EdgeRow],
    database_name: str,
) -> str:
    ordered_osm_ids = sorted(used_osm_nodes.keys(), key=lambda value: int(value))
    node_id_map = {osm_id: index + 1 for index, osm_id in enumerate(ordered_osm_ids)}

    lines = [
        "SET NAMES utf8mb4;",
        f"USE `{database_name}`;",
        "DELETE FROM tasks;",
        "DELETE FROM edges;",
        "DELETE FROM nodes;",
    ]

    if ordered_osm_ids:
        node_values = []
        for osm_id in ordered_osm_ids:
            lon, lat = used_osm_nodes[osm_id]
            node_values.append(
                f"({node_id_map[osm_id]}, {osm_id}, '', {lon:.8f}, {lat:.8f}, 'waypoint')"
            )
        for batch in chunked(node_values, 1000):
            lines.append(
                "INSERT INTO nodes (id, osm_node_id, name, lon, lat, kind) VALUES\n  "
                + ",\n  ".join(batch)
                + ";"
            )

    if edges:
        edge_values = []
        for edge in edges:
            edge_values.append(
                "("
                f"{node_id_map[str(edge.from_node)]}, "
                f"{node_id_map[str(edge.to_node)]}, "
                f"{edge.distance_m:.3f}, "
                f"{edge.travel_time_sec:.3f}, "
                f"'{sql_string(edge.way_type)}', "
                f"'{sql_string(edge.name)}', "
                f"'{sql_string(edge.geometry_json)}'"
                ")"
            )
        for batch in chunked(edge_values, 1000):
            lines.append(
                "INSERT INTO edges (from_node, to_node, distance_m, travel_time_sec, way_type, name, geometry_json) VALUES\n  "
                + ",\n  ".join(batch)
                + ";"
            )

    return "\n".join(lines) + "\n"


def run_mysql(mysql_bin: str, args: argparse.Namespace, sql_text: str) -> None:
    command = [
        mysql_bin,
        f"--host={args.db_host}",
        f"--port={args.db_port}",
        f"--user={args.db_user}",
        f"--password={args.db_password}",
        "--default-character-set=utf8mb4",
    ]
    subprocess.run(
        command,
        input=sql_text.encode("utf-8"),
        check=True,
    )


def main() -> int:
    args = parse_args()
    root = Path(__file__).resolve().parents[1]
    osm_path = Path(args.input)
    schema_path = Path(args.schema)
    boundary_path = Path(args.boundary)

    if not osm_path.exists():
        raise FileNotFoundError(f"OSM input not found: {osm_path}")
    if not schema_path.exists():
        raise FileNotFoundError(f"Schema file not found: {schema_path}")
    if not boundary_path.exists():
        raise FileNotFoundError(f"Boundary file not found: {boundary_path}")

    polygon = load_boundary(boundary_path)
    used_osm_nodes, edges = load_walk_edges(osm_path, polygon)
    if not used_osm_nodes or not edges:
        raise RuntimeError("No walkable campus roads were found in the supplied OSM export.")

    mysql_bin = find_mysql_binary(args.mysql_bin)
    schema_sql = schema_path.read_text(encoding="utf-8")
    import_sql = build_import_sql(used_osm_nodes, edges, args.db_name)

    run_mysql(mysql_bin, args, schema_sql)
    run_mysql(mysql_bin, args, import_sql)

    print(
        f"Imported {len(used_osm_nodes)} nodes and {len(edges)} directed edges "
        f"into {args.db_name} from {osm_path}."
    )
    print(f"Boundary source: {boundary_path}")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except subprocess.CalledProcessError as error:
        print(f"mysql command failed with exit code {error.returncode}.", file=sys.stderr)
        raise SystemExit(error.returncode)
    except Exception as error:  # pragma: no cover - CLI fallback
        print(str(error), file=sys.stderr)
        raise SystemExit(1)
