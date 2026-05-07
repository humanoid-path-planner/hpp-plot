import threading
import time

from .interactive_viewer import InteractiveGraphViewer
from .utils import _serialize_graph
from .web_app_server import StaticWebAppService
from .websocket_bridge import GraphWebSocketBridge


class GraphViewerThread(threading.Thread):
    """Runs graph viewer in separate daemon"""

    def __init__(
        self,
        graph,
        problem,
        config_callback,
        start_qt_viewer=False,
        ws_host="127.0.0.1",
        ws_port=8765,
        react_host="127.0.0.1",
        react_port=5177,
    ):
        super().__init__(daemon=True, name="GraphViewerThread")
        self.graph = graph
        self.problem = problem
        self.config_callback = config_callback
        self.ws_host = ws_host
        self.ws_port = ws_port
        self.start_qt_viewer = start_qt_viewer
        self.react_host = react_host
        self.react_port = react_port
        self._viewer = None
        self._ws_bridge = None
        self._web_app_service = StaticWebAppService(
            host=self.react_host,
            port=self.react_port,
        )
        self._lock = threading.Lock()
        self._stop_event = threading.Event()

    def run(self):
        self._viewer = InteractiveGraphViewer(
            self.graph, self.problem, self.config_callback
        )

        if self.start_qt_viewer:
            self._viewer.show()

        else:
            # start web app static server
            self._web_app_service.start()

            ## start websocket bridge
            self._ws_bridge = GraphWebSocketBridge(
                host=self.ws_host,
                port=self.ws_port,
                on_message=self.handle_web_app_message,
                snapshot_provider=self._build_snapshot_payload,
            )

            self._ws_bridge.start()
            if self._ws_bridge.is_running:
                self._ws_bridge.send_viewer_snapshot(self.graph)

            try:
                while (
                    not self._stop_event.is_set()
                    and self._ws_bridge is not None
                    and self._ws_bridge.is_running
                ):
                    time.sleep(0.2)
            finally:
                if self._ws_bridge is not None:
                    self._ws_bridge.stop()
                self._web_app_service.stop()

    def _build_snapshot_payload(self):
        return {
            "type": "viewer_snapshot",
            "graph": _serialize_graph(self.graph),
        }

    @property
    def ws_bridge(self):
        return self._ws_bridge

    @property
    def is_ws_running(self):
        return self._ws_bridge is not None and self._ws_bridge.is_running

    def send_viewer_snapshot(self, graph=None, problem=None):
        if self._ws_bridge is None or not self._ws_bridge.is_running:
            print("WebSocket bridge not running, cannot send viewer snapshot.")
            return
        self._viewer.problem = problem if problem is not None else self._viewer.problem
        self._viewer.graph = graph if graph is not None else self._viewer.graph
        if self._viewer.graph is not None:
            self._ws_bridge.send_viewer_snapshot(graph)

    def handle_web_app_message(self, message):
        """Dispatch a React websocket message to the interactive viewer."""
        if self._viewer is None:
            return False
        with self._lock:
            return self._viewer.handle_web_app_message(message)

    def stop(self):
        """Request the thread to stop (headless mode)."""
        self._stop_event.set()
