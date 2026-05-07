import WebSocketStatus from "./WebSocketStatus";

export default function Toolbar({
  onLayout,
  onFit,
  onDownload,
  onRefresh,
  loadJsonConfig,
  connectionStatus,
  showWayPoints,
  status
}) {
  return (
    <section className="toolbar">
      <select id="layout-select" onChange={(e) => onLayout(e.target.value)}>
        <option value="cose">Layout COSE</option>
        <option value="breadthfirst">Layout Breadthfirst</option>
        <option value="grid">Layout Grid</option>
        <option value="concentric">Layout Concentric</option>
        <option value="random">Layout Random</option>
        <option value="circle">Layout Circle</option>
      </select>
      <button type="button" onClick={loadJsonConfig}>Load Layout</button>
      <button type="button" onClick={() => showWayPoints()}>Show waypoints</button>
      <button type="button" onClick={onFit}>Center</button>
      <button type="button" onClick={onRefresh}>Refresh graph</button>
      <button type="button" onClick={onDownload}>Download graph</button>
      <WebSocketStatus status={status} />
    </section>
  );
}
