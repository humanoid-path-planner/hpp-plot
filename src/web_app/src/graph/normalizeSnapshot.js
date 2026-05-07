// JSON STATE
// "id": state_id,
// "name": state_name,
// "isWaypointState": state_name in nodeIsWaypointByName,
// "constraints": constraints,
// "numericalConstraints": numerical_constraints,

// JSON EDGE
// "id": _component_id(edge),
// "name": _call_optional(edge, ("name",), default=None),
// "source": str(source),
// "target": str(target),
// "nbWaypoints": _call_optional(edge, ("nbWaypoints",), default=0),
// "weight": weight,
// "constraints": constraints,
// "numericalConstraints": numerical_constraints,
import * as Type from '../utils/Type';


export function elementsFromGraphSnapshot(graphSnapshot) {
  if (!graphSnapshot) {
    return null;
  }

  const states =
      Array.isArray(graphSnapshot.states) ? graphSnapshot.states : [];
  const snapshotEdges =
      Array.isArray(graphSnapshot.edges) ? graphSnapshot.edges : [];

  const nodes = states.map((state) => {
    return {
      data: {
        type: state.isWaypointState ? Type.WAYPOINT_STATE_TYPE :
                                      Type.STATE_TYPE,
        id: state.name,
        label: state.name,
        name: state.name,
        constraints: state.constraints,
        constraints_functions: state.numericalConstraints,
      },
      classes: state.isWaypointState ? 'waypoint' : 'state',
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
        type: Type.EDGE_TYPE,
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
