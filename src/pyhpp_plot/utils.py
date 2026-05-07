from __future__ import annotations

from collections.abc import Iterable
from typing import Any

_U64_MOD = 1 << 64
_I64_MAX = (1 << 63) - 1


def _jsonable(value: Any) -> Any:
    if value is None or isinstance(value, (str, int, float, bool)):
        return value

    if isinstance(value, dict):
        return {str(key): _jsonable(item) for key, item in value.items()}

    if isinstance(value, (list, tuple, set)):
        return [_jsonable(item) for item in value]

    to_list = getattr(value, "tolist", None)
    if callable(to_list):
        return _jsonable(to_list())

    return str(value)


def _call_optional(obj: Any, names: Iterable[str], default: Any = None) -> Any:
    if isinstance(names, str):
        names = (names,)

    for name in names:
        candidate = getattr(obj, name, None)
        if candidate is None:
            continue

        if not callable(candidate):
            return candidate

        try:
            return candidate()
        except TypeError:
            continue
        except Exception:
            continue

    return default


def _normalize_weight(raw_weight: Any) -> int:
    """Convert C++ weight to signed 64-bit when Python exposes it as unsigned."""
    weight = int(raw_weight)
    if weight > _I64_MAX:
        weight -= _U64_MOD
    return weight


def _get_nc_names(nc_list: Any) -> list[str]:
    result = []
    for nc in nc_list:
        try:
            result.append(nc.function().name())
        except Exception:
            result.append("Unknown")
    return result


def _serialize_graph(graph: Any) -> dict[str, Any]:
    """Serialize graph structure: states and edges with metadata."""
    if graph is None:
        return {}
    states = graph.getStates() or []
    transitions = graph.getTransitions() or []

    # Pre-compute waypoint states similarly to C++ implementation
    nodeIsWaypointByName = set()
    for edge in transitions:
        try:
            if edge.isWaypointEdge():
                nb = _call_optional(edge, ("nbWaypoints",), default=0)
                # target of inner edges (except the last one) are waypoint states
                for i in range(nb):
                    inner_edge = edge.waypoint(i)
                    _, target_name = graph.getNodesConnectedByTransition(inner_edge)
                    nodeIsWaypointByName.add(target_name)
        except Exception:
            continue

    def _serialize_state(state: Any) -> dict[str, Any] | None:
        try:
            state_id = str(state.id())
            state_name = state.name()
        except Exception:
            return None

        try:
            constraints = graph.displayStateConstraints(state)
        except Exception:
            constraints = None

        try:
            numerical_constraints = _get_nc_names(
                graph.getNumericalConstraintsForState(state)
            )
        except Exception:
            numerical_constraints = None

        return {
            "id": state_id,
            "name": state_name,
            "isWaypointState": state_name in nodeIsWaypointByName,
            "constraints": constraints,
            "numericalConstraints": numerical_constraints,
        }

    def _serialize_edge(edge: Any) -> dict[str, Any] | None:
        try:
            source, target = graph.getNodesConnectedByTransition(edge)
        except Exception:
            return None

        try:
            weight = _normalize_weight(graph.getWeight(edge))
        except Exception:
            weight = None

        try:
            constraints = graph.displayEdgeConstraints(edge)
        except Exception:
            constraints = None

        try:
            numerical_constraints = _get_nc_names(
                graph.getNumericalConstraintsForEdge(edge)
            )
        except Exception:
            numerical_constraints = None

        return {
            "id": edge.id(),
            "name": _call_optional(edge, ("name",), default=None),
            "source": str(source),
            "target": str(target),
            "nbWaypoints": _call_optional(edge, ("nbWaypoints",), default=0),
            "weight": weight,
            "constraints": constraints,
            "numericalConstraints": numerical_constraints,
        }

    serialized_states = [s for state in states if (s := _serialize_state(state))]
    serialized_edges = [e for edge in transitions if (e := _serialize_edge(edge))]

    return {
        "name": _call_optional(graph, ("name",), default=""),
        "id": _call_optional(graph, ("id",), default=None),
        "states": serialized_states,
        "edges": serialized_edges,
    }
