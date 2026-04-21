export const graphStyle = [
  {
    selector: "node",
    style: {
      label: "data(label)",
      "font-size": 12,
      color: "#1f2a30",
      "text-valign": "center",
      "text-halign": "center",
      "text-wrap": "wrap",
      "text-max-width": 180,
      width: "label",
      height: "label",
      padding: "12px",
      "border-width": 2,
      "border-color": "#2b3a40",
      "background-color": "#f2b84b",
    },
  },
  {
    selector: "node.waypoint",
    style: {
      shape: "hexagon",
      "background-color": "#e0665b",
    },
  },
  {
    selector: "node.hovered",
    style: {
      "border-color": "var(--accent)",
      "border-width": 3,
    },
  },
  {
    selector: "edge",
    style: {
      label: "data(label)",
      width: "data(weight) * 2",
      "curve-style": "bezier",
      "target-arrow-shape": "triangle",
    },
  },
  {
    selector: "edge.dotted",
    style: {
      "line-style": "dotted",
    },
  },
  {
    selector: "edge.self-loop",
    style: {
      "curve-style": "bezier",
      "loop-direction": 90,
      "loop-sweep": "-25deg",
      "control-point-step-size": "data(controlPointStepSize)",
      width: 2,
    },
  },
  {
    selector: "node.highlighted, edge.highlighted",
    style: {
      "overlay-opacity": 0.18,
      "overlay-padding": 10,
      "line-color": "black",
    },
  },
];
