"""WebSocket bridge used to exchange messages between Python and a React UI."""

from __future__ import annotations

import asyncio
import json
import threading
from collections.abc import Iterable
from typing import Any, Callable

from websockets.asyncio.server import ServerConnection, serve


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


def _serialize_graph(graph: Any) -> dict[str, Any]:
    """Serialize graph structure: states and edges with metadata."""
    if graph is None:
        return {}

    def _component_id(component: Any) -> str | None:
        try:
            return str(component.id())
        except Exception:
            return None

    def _get_nc_names(nc_list: Any) -> list[str]:
        result = []
        for nc in nc_list:
            try:
                result.append(nc.function().name())
            except Exception:
                result.append("Unknown")
        return result

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
            "id": _component_id(edge),
            "name": _call_optional(edge, ("name",), default=None),
            "source": str(source),
            "target": str(target),
            "nbWaypoints": _call_optional(edge, ("nbWaypoints",), default=0),
            "weight": weight,
            "constraints": constraints,
            "numericalConstraints": numerical_constraints,
        }

    states = graph.getStates() or []
    transitions = graph.getTransitions() or []

    return {
        "name": _call_optional(graph, ("name",), default=""),
        "id": _call_optional(graph, ("id",), default=None),
        "states": [s for state in states if (s := _serialize_state(state))],
        "edges": [e for edge in transitions if (e := _serialize_edge(edge))],
    }


class GraphWebSocketBridge:
    """WebSocket server for sending graph events to a React frontend."""

    def __init__(
        self,
        host: str = "127.0.0.1",
        port: int = 8765,
        on_message: Callable[[dict[str, Any]], None] | None = None,
        snapshot_provider: Callable[[], dict[str, Any] | None] | None = None,
    ):
        self.host = host
        self.port = port
        self.on_message = on_message
        self.snapshot_provider = snapshot_provider
        self._loop: asyncio.AbstractEventLoop | None = None
        self._thread: threading.Thread | None = None
        self._server = None
        self._ready = threading.Event()
        self._stopped = threading.Event()
        self._stop_event: asyncio.Event | None = None

    @property
    def url(self) -> str:
        return f"ws://{self.host}:{self.port}"

    def start(self) -> "GraphWebSocketBridge":
        if self._thread is not None and self._thread.is_alive():
            return self

        self._ready.clear()
        self._stopped.clear()

        self._thread = threading.Thread(
            target=self._run,
            daemon=True,
            name=f"GraphWebSocketBridge:{self.port}",
        )
        self._thread.start()
        self._ready.wait(timeout=2.0)
        return self

    @property
    def is_running(self) -> bool:
        return (
            self._thread is not None
            and self._thread.is_alive()
            and self._loop is not None
            and not self._loop.is_closed()
            and self._server is not None
        )

    def stop(self) -> None:
        if self._loop is None:
            return

        def _shutdown() -> None:
            if self._stop_event is not None:
                self._stop_event.set()

        self._loop.call_soon_threadsafe(_shutdown)
        self._stopped.wait(timeout=2.0)
        if self._thread is not None and self._thread.is_alive():
            self._thread.join(timeout=2.0)

    def broadcast(self, payload: dict[str, Any]) -> None:
        if not self.is_running:
            return
        asyncio.run_coroutine_threadsafe(self._broadcast(payload), self._loop)

    def send_status(self, message: str) -> None:
        self.broadcast({"type": "status", "message": message})

    def send_config(self, config: Any, label: str) -> None:
        self.broadcast(
            {
                "type": "config_generated",
                "label": label,
                "config": _jsonable(config),
            }
        )

    def send_viewer_snapshot(self, graph: Any, problem: Any) -> None:
        self.broadcast(
            {
                "type": "viewer_snapshot",
                "graph": _serialize_graph(graph),
                "problem": (problem),
            }
        )

    def _run(self) -> None:
        self._loop = asyncio.new_event_loop()
        asyncio.set_event_loop(self._loop)
        try:
            self._loop.run_until_complete(self._serve())
        except OSError as exc:
            self._ready.set()
            print(
                f"WebSocket bridge could not start on {self.host}:{self.port}: {exc}"
            )
        finally:
            self._loop.close()
            self._stopped.set()

    async def _serve(self) -> None:
        self._stop_event = asyncio.Event()

        async with serve(self._handler, self.host, self.port) as server:
            self._server = server
            self._ready.set()
            await self._stop_event.wait()

    async def _handler(self, websocket: ServerConnection):
        await websocket.send(
            json.dumps({"type": "hello", "source": "python", "url": self.url})
        )

        if self.snapshot_provider is not None:
            try:
                snapshot = self.snapshot_provider()
            except Exception:
                snapshot = None
            if snapshot is not None:
                await websocket.send(json.dumps(_jsonable(snapshot)))

        async for raw_message in websocket:
            try:
                message = json.loads(raw_message)
            except json.JSONDecodeError:
                message = {"type": "text", "payload": raw_message}

            if (message.get("type") == "request_snapshot" and self.snapshot_provider is not None):
                try:
                    snapshot = self.snapshot_provider()
                except Exception:
                    snapshot = None
                if snapshot is not None:
                    await websocket.send(json.dumps(_jsonable(snapshot)))

            if self.on_message is not None:
                self.on_message(message)

            response = self._handle_message(message)
            if response is not None:
                await websocket.send(json.dumps(_jsonable(response)))

    async def _broadcast(self, payload: dict[str, Any]) -> None:
        if self._server is None or not getattr(self._server, "connections", None):
            return

        message = json.dumps(_jsonable(payload))
        await asyncio.gather(
            *(connection.send(message) for connection in list(self._server.connections)),
            return_exceptions=True,
        )

    def _handle_message(self, message: dict[str, Any]) -> dict[str, Any] | None:
        message_type = message.get("type")

        if message_type == "ping":
            return {"type": "pong", "source": "python"}

        if message_type == "menu_action":
            return {"type": "ack", "source": "python", "received": message}

        if message_type == "request_status":
            return {"type": "status", "source": "python", "message": "alive"}
        return None
