import { useCallback, useEffect, useRef, useState } from "react";

const DEFAULT_WS_URL = import.meta.env.VITE_WS_URL || "ws://127.0.0.1:8765";

export default function useGraphWebSocket(url = DEFAULT_WS_URL) {
  const socketRef = useRef(null);
  const reconnectTimerRef = useRef(null);
  const [status, setStatus] = useState("connecting");
  const [lastMessage, setLastMessage] = useState(null);

  const sendMessage = useCallback((message) => {
    const socket = socketRef.current;
    if (!socket || socket.readyState !== WebSocket.OPEN) {
      return false;
    }

    socket.send(JSON.stringify(message));
    return true;
  }, []);

  useEffect(() => {
    let cancelled = false;

    const cleanupSocket = () => {
      if (reconnectTimerRef.current) {
        window.clearTimeout(reconnectTimerRef.current);
        reconnectTimerRef.current = null;
      }

      if (socketRef.current) {
        socketRef.current.close();
        socketRef.current = null;
      }
    };

    const connect = () => {
      const socket = new WebSocket(url);
      socketRef.current = socket;
      setStatus("connecting");

      socket.onopen = () => {
        if (!cancelled) {
          setStatus("connected");
        }
        socket.send(JSON.stringify({ type: "request_snapshot" }));
      };

      socket.onmessage = (event) => {
        try {
        
            console.debug("Received WebSocket message:", event.data);
            const payload = JSON.parse(event.data);
            setLastMessage(payload);
        } catch (_error) {
            console.error("Error parsing WebSocket message:", event.data);
            setLastMessage({ type: "raw", payload: event.data });
        }
      };

      socket.onerror = () => {
        if (!cancelled) {
          setStatus("error");
        }
      };

      socket.onclose = () => {
        if (cancelled) {
          return;
        }

        setStatus("disconnected");
        reconnectTimerRef.current = window.setTimeout(connect, 1500);
      };
    };

    connect();

    return () => {
      cancelled = true;
      cleanupSocket();
    };
  }, [url]);

  return {
    status,
    lastMessage,
    sendMessage,
  };
}
