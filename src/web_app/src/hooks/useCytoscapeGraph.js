import { useCallback, useEffect, useRef } from "react";
import cytoscape from "cytoscape";
import { graphStyle } from "../graph/style";
import { computeMenuPosition } from "../utils/contextMenuPosition";

export default function useCytoscapeGraph({
  elements,
  setMenu,
  hideMenu,
  setSelectedElementInfo,
}) {
  const containerRef = useRef(null);
  const cyRef = useRef(null);
  const hideWaypointsRef = useRef(false);

  const applyWaypointVisibility = useCallback((cy, withFit = true) => {
    if (!cy) return;

    cy.nodes().style("display", "element");
    cy.edges().style("display", "element");

    if (!hideWaypointsRef.current) {
      cy.edges().forEach((edge) => {
        edge.style("display", edge.data("nbWaypoints") > 0 ? "none" : "element");
      });
      if (withFit) cy.fit(undefined, 40);
      return;
    }

    const waypointNodes = cy.nodes(".waypoint");
    waypointNodes.style("display", "none");

    cy.edges().forEach((edge) => {
      const sourceHidden = edge.source().style("display") === "none";
      const targetHidden = edge.target().style("display") === "none";
      edge.style("display", sourceHidden || targetHidden ? "none" : "element");
    });



    if (withFit) {
      cy.fit(cy.nodes(":visible"), 60);
    }
  }, []);

  useEffect(() => {
    if (!containerRef.current) return undefined;

    const cyElements = Array.isArray(elements) ? elements : [];

    const cy = cytoscape({
      container: containerRef.current,
      elements: cyElements,
      style: graphStyle,
      layout: {
        name: "cose",
        animate: true,
        animationDuration: 450,
        padding: 30,
      },
    });

    cyRef.current = cy;
  // Start with waypoint filtering enabled, equivalent to a first click.
  hideWaypointsRef.current = true;
  applyWaypointVisibility(cy, false);

    const onRightClick = (event) => {
      event.originalEvent.preventDefault();

      const element = event.target;
      const label = element.data("label") || element.id();
      const type = element.isNode() ? "Noeud" : "Arete";
      const containerRect = containerRef.current.getBoundingClientRect();
      const { x, y } = computeMenuPosition(containerRect, event.renderedPosition);

      setMenu({
        visible: true,
        x,
        y,
        title: `${type}: ${label}`,
        elementId: element.id(),
        elementKind: element.isNode() ? "node" : "edge",
      });
    };


    const onLeftClick = (event) => {
      const element = event.target;
      const data = element.data();

      setSelectedElementInfo?.(data);
    };

    const onTapCanvas = (event) => {
      if (event.target === cy) {
        hideMenu();
        setSelectedElementInfo?.(null);
      }
    };

    const onMouseOverElement = (event) => {
      event.target.addClass("hovered");
    };

    const onMouseOutElement = (event) => {
      event.target.removeClass("hovered");
    };

    cy.on("tap", "node, edge", onLeftClick);
    cy.on("cxttap", "node, edge", onRightClick);
    cy.on("tap", onTapCanvas);
    cy.on("mouseover", "node, edge", onMouseOverElement);
    cy.on("mouseout", "node, edge", onMouseOutElement);

    return () => {
      cy.removeListener("tap", "node, edge", onLeftClick);
      cy.removeListener("cxttap", "node, edge", onRightClick);
      cy.removeListener("tap", onTapCanvas);
      cy.removeListener("mouseover", "node, edge", onMouseOverElement);
      cy.removeListener("mouseout", "node, edge", onMouseOutElement);
      cy.destroy();
      cyRef.current = null;
    };
  }, [elements, setMenu, hideMenu, applyWaypointVisibility, setSelectedElementInfo]);

  const runLayout = useCallback((name) => {
    if (!cyRef.current) return;
    cyRef.current.layout({
      name,
      animate: true,
      animationDuration: 400,
      padding: 30,
    }).run();
  }, []);

  const fitGraph = useCallback(() => {
    if (!cyRef.current) return;
    cyRef.current.fit(undefined, 40);
  }, []);


  const showWayPoints = useCallback(() => {
    if (!cyRef.current) return;

    const cy = cyRef.current;
    hideWaypointsRef.current = !hideWaypointsRef.current;
    applyWaypointVisibility(cy, true);
  }, [applyWaypointVisibility]);


  return {
    containerRef,
    cyRef,
    runLayout,
    fitGraph,
    showWayPoints
  };

}
