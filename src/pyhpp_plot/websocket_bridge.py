"""WebSocket bridge used to exchange messages between Python and a React UI."""

from __future__ import annotations

import asyncio
import json
import threading
from collections.abc import Callable
from typing import Any

from websockets.asyncio.server import ServerConnection, serve

from .utils import _jsonable, _serialize_graph


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

    def start(self) -> GraphWebSocketBridge:
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

    def send_viewer_snapshot(self, graph: Any) -> None:
        self.broadcast(
            {
                "type": "viewer_snapshot",
                "graph": _serialize_graph(graph),
            }
        )

    def _run(self) -> None:
        self._loop = asyncio.new_event_loop()
        asyncio.set_event_loop(self._loop)
        try:
            self._loop.run_until_complete(self._serve())
        except OSError as exc:
            self._ready.set()
            print(f"WebSocket bridge could not start on {self.host}:{self.port}: {exc}")
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
        async for raw_message in websocket:
            try:
                message = json.loads(raw_message)
            except json.JSONDecodeError:
                message = {"type": "text", "payload": raw_message}

            response = self._handle_message(message)
            if response is None and self.on_message is not None:
                try:
                    response = self.on_message(message)
                except Exception:
                    response = None
            if response is not None:
                await websocket.send(json.dumps(_jsonable(response)))
                continue
            await websocket.send(
                json.dumps(
                    {"status": "error", "message": "Received unknown message type"}
                )
            )

    async def _broadcast(self, payload: dict[str, Any]) -> None:
        if self._server is None or not getattr(self._server, "connections", None):
            return

        message = json.dumps(_jsonable(payload))
        await asyncio.gather(
            *(
                connection.send(message)
                for connection in list(self._server.connections)
            ),
            return_exceptions=True,
        )

    def _handle_message(self, message: dict[str, Any]) -> dict[str, Any] | None:
        if (
            message.get("type") == "request_snapshot"
            and self.snapshot_provider is not None
        ):
            try:
                snapshot = self.snapshot_provider()
            except Exception:
                snapshot = None
            if snapshot is not None:
                return snapshot
        return None
