import cytoscape from 'cytoscape';
import {useCallback, useEffect, useRef} from 'react';

import {computeMenuPosition} from '../utils/contextMenuPosition';
import * as Type from '../utils/Type';

import {graphStyle} from './style';

export const applyWaypointVisibility = (cy, isHidden, withFit = true) => {
  if (!cy) return;

  cy.nodes().style('display', 'element');
  cy.edges().style('display', 'element');

  const hiddenNodes = []

  cy.elements('.collapsed-group').forEach((group) => {
    group.data('collapsedChildren')?.forEach((childId) => {
      hiddenNodes.push(childId);
    });
  })

  console.log('Hidden nodes due to collapsed groups:', hiddenNodes);

  cy.nodes().forEach((node) => {
    node.style('display', hiddenNodes.includes(node.id()) ? 'none' : 'element');
  });


  if (!isHidden) {
    cy.edges().forEach((edge) => {
      edge.style('display', edge.data('nbWaypoints') > 0 ? 'none' : 'element');
    });
    if (withFit) cy.fit(undefined, 40);
    return;
  } else {
    cy.edges().forEach((edge) => {
      edge.style('display', edge.data('isWaypointEdge') ? 'none' : 'element');
    });
  }

  const waypointNodes = cy.nodes('.waypoint');
  waypointNodes.style('display', 'none');


  if (withFit) {
    cy.fit(cy.nodes(':visible'), 60);
  }
};

export default function useCytoscapeGraph({
  elements,
  setMenu,
  hideMenu,
  setSelectedElementInfo,
}) {
  const containerRef = useRef(null);
  const cyRef = useRef(null);
  const hideWaypointsRef = useRef(false);

  useEffect(() => {
    if (!containerRef.current) return undefined;

    const cyElements = Array.isArray(elements) ? elements : [];

    const cy = cytoscape({
      container: containerRef.current,
      elements: cyElements,
      style: graphStyle,
      layout: {
        name: 'cose',
        animate: true,
        animationDuration: 450,
        padding: 30,
      },
    });

    cyRef.current = cy;
    // Start with waypoint filtering enabled, equivalent to a first click.
    hideWaypointsRef.current = true;
    applyWaypointVisibility(cy, hideWaypointsRef.current, false);

    const onRightClick = (event) => {
      event.originalEvent.preventDefault();

      const element = event.target;
      const cy = event.cy;

      const selectedElements = cy.$(':selected');

      const targetElements = (element && element.selected()) ?
          selectedElements :
          cy.collection([element]);


      const label = element.data('label') || element.id();
      const type = element.isNode() ? 'Noeud' : 'Arete';
      const containerRect = containerRef.current.getBoundingClientRect();
      const {x, y} = computeMenuPosition(containerRect, event.renderedPosition);
      const isMultiple = targetElements.length > 1;
      const title = isMultiple ?
          `Selection of (${targetElements.length}) elements` :
          element.isNode() ? `Node: ${label}` :
                             `Edge: ${label}`;

      let elementKind = isMultiple ? 'Selection' :
          element.isNode()         ? 'node' :
                                     'edge';
      if (element.isNode() && element.hasClass('group-node')) {
        elementKind = 'group';
      }

      setMenu({
        visible: true,
        x,
        y,
        title: title,
        selectedIds: targetElements.map(e => e.id()),
        elementKind: elementKind,
      });
    };


    const onLeftClick = (event) => {
      const element = event.target;
      const data = element.data();
      event.cy.$(':selected').unselect();
      element.select();

      setSelectedElementInfo?.(data);
    };

    const onTapCanvas = (event) => {
      if (event.target === cy) {
        hideMenu();
        setSelectedElementInfo?.(null);
      }
    };

    const onMouseOverElement = (event) => {
      event.target.addClass('hovered');
    };

    const onMouseOutElement = (event) => {
      event.target.removeClass('hovered');
    };

    cy.on('tap', 'node, edge', onLeftClick);
    cy.on('cxttap', 'node, edge', onRightClick);
    cy.on('tap', onTapCanvas);
    cy.on('mouseover', 'node, edge', onMouseOverElement);
    cy.on('mouseout', 'node, edge', onMouseOutElement);

    return () => {
      cy.removeListener('tap', 'node, edge', onLeftClick);
      cy.removeListener('cxttap', 'node, edge', onRightClick);
      cy.removeListener('tap', onTapCanvas);
      cy.removeListener('mouseover', 'node, edge', onMouseOverElement);
      cy.removeListener('mouseout', 'node, edge', onMouseOutElement);
      cy.destroy();
      cyRef.current = null;
    };
  }, [elements, setMenu, hideMenu, setSelectedElementInfo]);

  const runLayout = useCallback((name) => {
    if (!cyRef.current) return;
    cyRef.current
        .layout({
          name,
          animate: true,
          animationDuration: 400,
          padding: 30,
        })
        .run();
  }, []);

  const fitGraph = useCallback(() => {
    if (!cyRef.current) return;
    cyRef.current.fit(undefined, 40);
  }, []);


  const showWayPoints = useCallback(() => {
    if (!cyRef.current) return;

    const cy = cyRef.current;
    hideWaypointsRef.current = !hideWaypointsRef.current;
    applyWaypointVisibility(cy, hideWaypointsRef.current, true);
  }, []);


  return {
    containerRef,
    cyRef,
    runLayout,
    fitGraph,
    showWayPoints,
    hideWaypointsRef
  };
}
