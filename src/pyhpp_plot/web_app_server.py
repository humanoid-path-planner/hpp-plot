"""Static web app server used by the graph viewer web mode."""

from __future__ import annotations

import http.server
import socket
import threading
from pathlib import Path


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


class StaticWebAppService:
    """Serve the built React app with stdlib HTTP server."""

    def __init__(self, host: str = "127.0.0.1", port: int = 5173):
        self.host = host
        self.port = port
        self._server: http.server.HTTPServer | None = None

    def _is_tcp_port_free(self, timeout: float = 0.2) -> bool:
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
            try:
                sock.bind((self.host, int(self.port)))
                return True
            except OSError:
                return False

    def start(self) -> bool:
        """Start static serving when possible.

        Returns:
            True if the server was started, False otherwise.
        """
        if not self._is_tcp_port_free():
            print(f"Port {self.port} is not free, skipping React app auto-start.")
            return False

        dist_dir = _find_webapp_dist()
        if dist_dir is None:
            print("React app dist/ not found, skipping auto-start.\n")
            return False

        class _QuietHandler(http.server.SimpleHTTPRequestHandler):
            def __init__(self, *args, **kwargs):
                super().__init__(*args, directory=str(dist_dir), **kwargs)

            def log_message(self, *args):
                pass

        self._server = http.server.HTTPServer((self.host, self.port), _QuietHandler)
        thread = threading.Thread(target=self._server.serve_forever, daemon=True)
        thread.name = "ReactStaticServer"
        thread.start()
        print(f"React app ready at http://{self.host}:{self.port}")
        return True

    def stop(self) -> None:
        """Stop static serving if it is running."""
        if self._server is None:
            return
        self._server.shutdown()
        self._server = None
