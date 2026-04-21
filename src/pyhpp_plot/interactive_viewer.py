"""Interactive constraint graph viewer with Python callbacks.

This module provides a wrapper around the native graph viewer that allows
Python code to add custom actions to context menus for nodes and edges.
Implements all features from the CORBA hpp-monitoring-plugin.
"""

import http.server
import os
import socket
import threading
import time
from pathlib import Path

from .websocket_bridge import GraphWebSocketBridge, _serialize_graph


def _find_webapp_dist() -> Path | None:
    """Locate the built React app (dist/)."""

    module_path = Path(__file__).resolve()
    for depth in range(3, 7):
        try:
            prefix = module_path.parents[depth]
        except IndexError:
            break
        candidate = prefix / "share" / "hpp-plot" / "webapp"
        if (candidate / "index.html").is_file():
            return candidate

    return None


def _serve_webapp(dist_dir: Path, host: str, port: int) -> http.server.HTTPServer:
    """Serve static files from dist_dir using stdlib HTTP server.

    Returns the HTTPServer instance (already started in a daemon thread).
    Logs HTTP requests are suppressed.
    """

    class _QuietHandler(http.server.SimpleHTTPRequestHandler):
        def __init__(self, *args, **kwargs):
            super().__init__(*args, directory=str(dist_dir), **kwargs)

        def log_message(self, *args):
            pass

    server = http.server.HTTPServer((host, port), _QuietHandler)
    t = threading.Thread(target=server.serve_forever, daemon=True)
    t.name = "ReactStaticServer"
    t.start()
    return server


class InteractiveGraphViewer:
    """Wrapper for HppNativeGraphWidget with Python-based context menu actions."""

    def __init__(self, graph, problem, config_callback=None):
        """Initialize the interactive graph viewer.

        Args:
            graph: PyWGraph from pyhpp.manipulation
            problem: PyWProblem from pyhpp.manipulation
            config_callback: Optional callable(config, label) that receives
                           generated configurations
        """
        self.graph = graph
        self.problem = problem
        self.config_callback = config_callback or (lambda config, label: None)
        self.current_config = None

    def show(self):
        """Show graph viewer (blocking - runs Qt event loop until window closes)."""
        from pyhpp_plot.graph_viewer import show_interactive_graph

        show_interactive_graph(
            self.graph,
            node_callback=self._on_node_context_menu,
            edge_callback=self._on_edge_context_menu,
        )

    def _on_node_context_menu(self, node_id, node_name, menu):
        """Add custom actions to node context menu.

        Args:
            node_id: ID of the node (state) - from C++ graph
            node_name: Name of the node (state) - from C++ graph
            menu: MenuActionProxy for adding actions
        """
        menu.addSeparator()

        menu.addAction(
            "&Generate random config", lambda: self._generate_random_config(node_name)
        )

        menu.addAction(
            "Generate from &current config",
            lambda: self._generate_from_current_config(node_name),
        )

        menu.addAction(
            "Set as &target state", lambda: self._set_target_state(node_name)
        )

    def _on_edge_context_menu(self, edge_id, edge_name, menu):
        """Add custom actions to edge context menu.

        Args:
            edge_id: ID of the edge (transition) - from C++ graph
            edge_name: Name of the edge (transition) - from C++ graph
            menu: MenuActionProxy for adding actions
        """
        menu.addSeparator()

        menu.addAction(
            "&Extend current config", lambda: self._extend_current_to_current(edge_name)
        )

        menu.addAction(
            "&Extend current config to random config",
            lambda: self._extend_current_to_random(edge_name),
        )

    def _generate_random_config(self, state_name):
        """Generate random config and project to state.

        Args:
            state_name: Name of the state
        """
        try:
            state = self.graph.getState(state_name)
            shooter = self.problem.configurationShooter()

            min_error = float("inf")
            for i in range(20):
                q_random = shooter.shoot()
                success, q_proj, error = self.graph.applyStateConstraints(
                    state, q_random
                )

                if success:
                    self.current_config = q_proj
                    self.config_callback(
                        q_proj, f"Random config in state: {state_name}"
                    )
                    return

                if error < min_error:
                    min_error = error
        except Exception:
            pass

    def _generate_from_current_config(self, state_name):
        """Project current config to state.

        Args:
            state_name: Name of the state
        """
        try:
            if self.current_config is None:
                return

            state = self.graph.getState(state_name)
            success, q_proj, _error = self.graph.applyStateConstraints(
                state, self.current_config
            )

            if success:
                self.current_config = q_proj
                self.config_callback(
                    q_proj, f"Current config projected to state: {state_name}"
                )
        except Exception:
            pass

    def _set_target_state(self, state_name):
        """Set state as goal for planning.

        Args:
            state_name: Name of the state
        """
        try:
            state = self.graph.getState(state_name)
            shooter = self.problem.configurationShooter()

            for i in range(20):
                q_random = shooter.shoot()
                success, q_goal, _error = self.graph.applyStateConstraints(
                    state, q_random
                )

                if success:
                    self.problem.addGoalConfig(q_goal)
                    return
        except Exception:
            pass

    def _extend_current_to_current(self, edge_name):
        """Generate target config along edge from current config.
        calls generateTargetConfig(edge, current, current).

        Args:
            edge_name: Name of the edge
        """
        try:
            if self.current_config is None:
                return

            edge = self.graph.getTransition(edge_name)
            success, q_out, _error = self.graph.generateTargetConfig(
                edge, self.current_config, self.current_config
            )

            if success:
                self.current_config = q_out
                self.config_callback(q_out, f"Extended along edge: {edge_name}")
        except Exception:
            pass

    def _extend_current_to_random(self, edge_name):
        """Generate target config along edge to random config.
        calls generateTargetConfig(edge, current, random).

        Args:
            edge_name: Name of the edge
        """
        try:
            if self.current_config is None:
                return

            edge = self.graph.getTransition(edge_name)
            shooter = self.problem.configurationShooter()
            q_random = shooter.shoot()

            success, q_out, _error = self.graph.generateTargetConfig(
                edge, self.current_config, q_random
            )

            if success:
                self.current_config = q_out
                self.config_callback(
                    q_out, f"Extended along edge to random: {edge_name}"
                )
        except Exception:
            pass

    def _resolve_state_name(self, state_identifier):
        """Resolve a state identifier from React (id or name) to a graph state name."""
        if state_identifier is None:
            return None

        token = str(state_identifier)

        try:
            self.graph.getState(token)
            return token
        except Exception:
            pass

        try:
            states = self.graph.getStates() or []
        except Exception:
            states = []

        for state in states:
            try:
                if str(state.id()) == token:
                    return state.name()
            except Exception:
                continue

        return None

    def _resolve_edge_name(self, edge_identifier):
        """Resolve an edge identifier from React (id or name) to a graph edge name."""
        if edge_identifier is None:
            return None

        token = str(edge_identifier)

        try:
            self.graph.getTransition(token)
            return token
        except Exception:
            pass

        try:
            transitions = self.graph.getTransitions() or []
        except Exception:
            transitions = []

        for edge in transitions:
            try:
                if str(edge.id()) == token:
                    return edge.name()
            except Exception:
                continue

        return None

    def handle_react_message(self, message):
        """Handle websocket actions coming from the React app."""
        if not isinstance(message, dict):
            return False

        if message.get("type") != "menu_action":
            return False

        action = message.get("action")
        element_kind = str(message.get("elementKind", "")).lower()
        element_id = message.get("elementId")

        # UI-only actions stay handled in React.
        if action in {"inspect", "highlight", "remove"}:
            return True

        if element_kind == "node":
            state_name = self._resolve_state_name(element_id)
            if state_name is None:
                return False

            if action == "generate_random_config":
                self._generate_random_config(state_name)
                return True
            if action == "generate_from_current_config":
                self._generate_from_current_config(state_name)
                return True
            if action == "set_target_state":
                self._set_target_state(state_name)
                return True

            return False

        if element_kind == "edge":
            edge_name = self._resolve_edge_name(element_id)
            if edge_name is None:
                return False

            if action == "extend_current_to_current":
                self._extend_current_to_current(edge_name)
                return True
            if action == "extend_current_to_random":
                self._extend_current_to_random(edge_name)
                return True

            return False

        return False


class GraphViewerThread(threading.Thread):
    """Runs graph viewer in separate daemon"""

    def __init__(
        self,
        graph,
        problem,
        config_callback,
        ws_host="127.0.0.1",
        ws_port=8765,
        start_qt_viewer=False,
        react_host="127.0.0.1",
        react_port=5173,
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
        self._http_server = None
        self._lock = threading.Lock()
        self._stop_event = threading.Event()

    def run(self):
        self._viewer = InteractiveGraphViewer(self.graph, self.problem, self._on_config_generated)

        if self.start_qt_viewer:
            self._viewer.show()

        else:
            ## start webAPP
            self._start_web_app()


            ## start websocket bridge
            self._ws_bridge = GraphWebSocketBridge(host=self.ws_host,port=self.ws_port,on_message=self.handle_react_message,snapshot_provider=self._build_snapshot_payload)
            self._ws_bridge.start()
            if self._ws_bridge.is_running:
                self._ws_bridge.send_viewer_snapshot(self.graph, self.problem)
                
            try:
                while ( not self._stop_event.is_set()
                        and self._ws_bridge is not None
                        and self._ws_bridge.is_running):
                            time.sleep(0.2)
            finally:
                if self._ws_bridge is not None:
                    self._ws_bridge.stop()
                if self._http_server is not None:
                    self._http_server.shutdown()
                    self._http_server = None
      


    def _on_config_generated(self, config, label):
        self.config_callback(config, label)
        self.send_config(config, label)


    def _build_snapshot_payload(self):
        return {
            "type": "viewer_snapshot",
            "graph": _serialize_graph(self.graph),
        }



    def _is_tcp_port_free(self, host, port, timeout=0.2):
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
            sock.settimeout(timeout)
            return sock.connect_ex((host, int(port))) != 0

    def _start_web_app(self):

        if not self._is_tcp_port_free(self.react_host, self.react_port):
            print(f"Port {self.react_port} is not free, skipping React app auto-start.")
            return

        dist_dir = _find_webapp_dist()
        if dist_dir is None:
            print("React app dist/ not found, skipping auto-start.\n")
            return
        self._http_server = _serve_webapp(dist_dir, self.react_host, self.react_port)
        print(f"React app ready at http://{self.react_host}:{self.react_port}")



    @property
    def ws_bridge(self):
        return self._ws_bridge

    @property
    def is_ws_running(self):
        return self._ws_bridge is not None and self._ws_bridge.is_running

    def send_viewer_snapshot(self, graph, problem):
        if self._ws_bridge is None or not self._ws_bridge.is_running:
            return
        self._ws_bridge.send_viewer_snapshot(graph, problem) 

    def send_config(self, config, label):
        if self._ws_bridge is None or not self._ws_bridge.is_running:
            return
        self._ws_bridge.send_config(config, label)

    def handle_react_message(self, message):
        """Dispatch a React websocket message to the interactive viewer."""
        if self._viewer is None:
            return False
        with self._lock:
            return self._viewer.handle_react_message(message)

    def stop(self):
        """Request the thread to stop (headless mode)."""
        self._stop_event.set()