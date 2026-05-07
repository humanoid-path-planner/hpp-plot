export default function WebSocketStatus({ status }) {
  return (
    <section className="ws-status-panel" aria-label="WebSocket status">
      <span className={`ws-status ws-${status}`}>WS: {status}</span>
    </section>
  );
}
