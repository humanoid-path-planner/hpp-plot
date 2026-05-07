export const graphStyle = [
  {
    selector: 'node',
    style: {
      label: 'data(label)',
      'font-size': 12,
      color: '#1f2a30',
      'text-valign': 'center',
      'text-halign': 'center',
      'text-wrap': 'wrap',
      'text-max-width': 180,
      width: 'label',
      height: 'label',
      padding: '12px',
      'border-width': 2,
      'border-color': '#2b3a40',
      'background-color': '#f2b84b',
    },
  },
  {
    selector: 'node.waypoint',
    style: {
      shape: 'hexagon',
      'background-color': '#e0665b',
    },
  },
  {
    selector: 'node.hovered',
    style: {
      'border-width': 3,
    },
  },
  {
    selector: 'node:selected',
    style: {
      'border-width': 3,
      'border-color': '#1a64a4',
    },
  },
  {
    selector: 'edge:selected',
    style: {
      'line-color': '#1a64a4',
      'target-arrow-color': '#1a64a4',
    },
  },
  {
    selector: 'edge',
    style: {
      label: 'data(label)',
      'curve-style': 'bezier',
      'target-arrow-shape': 'triangle',
    },
  },
  {
    selector: 'edge.hovered',
    style: {
      'width': '5px',

    },
  },



  {
    selector: 'edge.dotted',
    style: {
      'line-style': 'dotted',
    },
  },
  {
    selector: 'edge.self-loop',
    style: {
      'curve-style': 'bezier',
      'loop-direction': 90,
      'loop-sweep': '-25deg',
      'control-point-step-size': 'data(controlPointStepSize)',
    },
  },
  {
    selector: 'node.highlighted, edge.highlighted',
    style: {
      'overlay-opacity': 0.18,
      'overlay-padding': 10,
      'line-color': 'black',
    },
  },
  {
    selector: ':parent',
    style: {
      'shape': 'rectangle',
      'background-color': '#f0f0f0',
      'border-color': '#517887',
      'border-width': 2,
      'text-valign': 'bottom',
      'text-halign': 'center',
      'text-margin-y': 5,
    },
  },
  {
    selector: '.group-node',
    style: {
      'shape': 'rectangle',
      'text-valign': 'bottom',
      'text-halign': 'center',
      'text-margin-y': 5,
    },
  },
  {
    selector: '.collapsed-group',
    style: {
      'shape': 'rectangle',
      'min-width': '260px',
      'min-height': '260px',
      'background-color': '#d9d9d9',
      'border-color': '#2b3a40',
      'border-width': 2,
      'border-style': 'dashed',
      'text-valign': 'center',
      'text-halign': 'center',
      'font-size': '2.5em',
    },
  },
  {
    selector: 'edge.meta-edge',
    style: {
      'line-color': '#7b8a90',
      'target-arrow-color': '#7b8a90',
    },
  }
];
