import {useCallback, useEffect, useRef, useState} from 'react';

import {useNotification} from '../contexts/NotificationContext';

const DEFAULT_WS_URL = import.meta.env.VITE_WS_URL || 'ws://127.0.0.1:8765';

export default function useGraphWebSocket(url = DEFAULT_WS_URL) {
  const {notify} = useNotification();
  const socketRef = useRef(null);
  const reconnectTimerRef = useRef(null);
  const [status, setStatus] = useState('connecting');
  const [lastMessage, setLastMessage] = useState(null);

  const sendMessage = useCallback((message) => {
    const socket = socketRef.current;
    if (!socket || socket.readyState !== WebSocket.OPEN) {
      notify(
          'Impossible de communiquer avec le serveur, action annulée.',
          'error');
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
      setStatus('connecting');

      socket.onopen = () => {
        if (!cancelled) {
          setStatus('connected');
        }
        sendMessage({type: 'request_snapshot'});
      };

      socket.onmessage = (event) => {
        try {
          const payload = JSON.parse(event.data);
          console.log('Received WebSocket message:', payload);

          if (payload.status === 'error' || payload.type === 'error') {
            notify(
                `Erreur: ${payload.message || 'Action échouée'}`, 'error',
                payload.error || payload);
          } else if (payload.type === 'action_success') {
            notify(
                payload.message || 'Action réussie', 'success',
                payload.details);
          }

          setLastMessage(payload);
        } catch (_error) {
          setLastMessage({type: 'raw', payload: event.data});
        }
      };

      socket.onerror = () => {
        console.error('WebSocket error occurred');
        if (!cancelled) {
          setStatus('error');
        }
      };

      socket.onclose = () => {
        console.warn('WebSocket connection closed');
        if (cancelled) {
          return;
        }

        setStatus('disconnected');
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
