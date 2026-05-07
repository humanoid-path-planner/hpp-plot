import { forwardRef } from "react";
import Legend from "./Legend";
import * as Type from '../utils/Type';



const GraphCanvas = forwardRef(function GraphCanvas({ toolbar, info, graphSnapshot }, ref) {
  const isGroup = info?.type === Type.GROUP_TYPE;
  const isEdge = info?.type === Type.EDGE_TYPE;
  const waypointCount = Number(info?.nbWaypoints);
  const constraints =  String(info?.constraints);
  const constraints_fuctions = info?.constraints_functions ?? [];
  const normalizedType = String(info?.type ?? "")
    .trim()
    .toLowerCase()
    .replace(/[^a-z0-9_-]+/g, "-");

  return (
    <div className="graph-container">
      <div className="graph-info" aria-live="polite">
        <h3 className="graph-info-title">Element Details</h3>
        {!info ? (
          <p className="graph-info-empty">Click a node or edge to inspect it.</p>
        ) : (
          <>
            {/* Basic Info Section */}
            <section className="graph-info-section">
              <h4 className="graph-info-section-title">General Info</h4>
              <dl className="graph-info-list">
                <div className="graph-info-row " >
                  <dt>Type</dt>
                  <dd className="info-badge" data-type={normalizedType}>{info.type}</dd>
                </div>
                <div className="graph-info-row">
                  <dt>ID</dt>
                  <dd className="info-id">{info.id}</dd>
                </div>
                <div className="graph-info-row">
                  <dt>Name</dt>
                  <dd>{info.name ?? "-"}</dd>
                </div>
              </dl>
            </section>

            {/* Edge-Specific Info Section */}
            {isEdge && (
              <section className="graph-info-section">
                <h4 className="graph-info-section-title">Edge Properties</h4>
                <dl className="graph-info-list">
                  <div className="graph-info-row">
                    <dt>Source</dt>
                    <dd>{info.source ?? "-"}</dd>
                  </div>
                  <div className="graph-info-row">
                    <dt>Target</dt>
                    <dd>{info.target ?? "-"}</dd>
                  </div>
                  <div className="graph-info-row">
                    <dt>Weight</dt>
                    <dd>{Number.isFinite(info.weight) ? info.weight : "-"}</dd>
                  </div>
                  <div className="graph-info-row">
                    <dt>Waypoints</dt>
                    <dd>
                      {waypointCount > 0 ? (
                        <span className="waypoint-badge">{waypointCount} waypoints</span>
                      ) : (
                        <span className="waypoint-empty">None</span>
                      )}
                    </dd>
                  </div>
                </dl>
              </section>
            )}

            {/* Constraints Section */}
            {(constraints_fuctions.length > 0 || constraints.length > 0) && (
              <section className="graph-info-section">
                <h4 className="graph-info-section-title">Constraints</h4>

                {constraints_fuctions.length > 0 && (
                  <div className="graph-info-constraints">
                    <h5>Applied Constraints:</h5>
                    <ul className="constraints-list">
                      {constraints_fuctions.map((constraint, index) => (
                        <li key={index} className="constraint-item">{constraint}</li>
                      ))}
                    </ul>
                  </div>
                )}

                {constraints.length > 0 && (
                  <div className="graph-info-constraints">
                    <h5>Details:</h5>
                    <pre className="constraint-details">{constraints}</pre>
                  </div>
                )}
              </section>
            )}
          </>
        )}
      </div>
      <div className="graph-main">
        {toolbar}
        <section ref={ref} id="cy" aria-label="Graph Cytoscape" />
        <Legend />
      </div>
    </div>
  );
});

export default GraphCanvas;
