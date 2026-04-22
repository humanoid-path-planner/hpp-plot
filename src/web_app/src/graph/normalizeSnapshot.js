function inferWaypointNodeIds(snapshotEdges) {
  const waypointNodeIds = new Set();
  const edges = Array.isArray(snapshotEdges) ? snapshotEdges : [];

  const waypointParents =
      edges.filter((edge) => edge && edge.nbWaypoints > 0 && edge.id != null);

  waypointParents.forEach((parent) => {
    const parentEndpoints = new Set([
      parent.source != null ? String(parent.source) : null,
      parent.target != null ? String(parent.target) : null,
    ]);

    edges.forEach((edge) => {
      if (!edge || edge.id == null) return;

      const sourceId = edge.source != null ? String(edge.source) : null;
      const targetId = edge.target != null ? String(edge.target) : null;

      if (sourceId && !parentEndpoints.has(sourceId))
        waypointNodeIds.add(sourceId);
      if (targetId && !parentEndpoints.has(targetId))
        waypointNodeIds.add(targetId);
    });
  });

  return waypointNodeIds;
}

export function elementsFromGraphSnapshot(graphSnapshot) {
  if (!graphSnapshot) {
    return null;
  }

  const states =
      Array.isArray(graphSnapshot.states) ? graphSnapshot.states : [];
  const snapshotEdges =
      Array.isArray(graphSnapshot.edges) ? graphSnapshot.edges : [];
  const inferredWaypointNodeIds = inferWaypointNodeIds(snapshotEdges);

  const nodeIds = new Set();

  const nodes = states.map((state) => {
    const nodeId = String(state.name ?? state.id);
    nodeIds.add(nodeId);
    const isWaypoint = inferredWaypointNodeIds.has(nodeId);
    return {
      data: {
        type: isWaypoint ? 'WaypointState' : 'State',
        id: nodeId,
        label: state.name,
        name: state.name,
        constraints: state.constraints,
        constraints_functions: state.numericalConstraints,
      },
      classes: isWaypoint ? 'waypoint' : 'state',
    };
  });

  const edges = snapshotEdges.map((edge) => {
    const classNames = [];
    if (String(edge.source) === String(edge.target))
      classNames.push('self-loop');
    if (Number(edge.weight) <= 0) classNames.push('dotted');

    return {
      classes: classNames.join(' '),
      data: {
        type: 'Edge',
        id: String(edge.id),
        source: String(edge.source),
        target: String(edge.target),
        weight: Number(edge.weight),
        controlPointStepSize: String(edge.source).length > 10 ?
            String(edge.source).length * 2 :
            40,
        label: String(edge.id),
        name: edge.name,
        nbWaypoints: edge.nbWaypoints,
        constraints: edge.constraints,
        constraints_functions: edge.numericalConstraints,
      },
    };
  });

  return [...nodes, ...edges];
}
