import { useCallback, useEffect, useMemo, useRef, useState } from "react";
import ContextMenu, { initialMenuState } from "./components/ContextMenu";
import GraphCanvas from "./components/GraphCanvas";
import Toolbar from "./components/Toolbar";
import { elementsFromGraphSnapshot } from "./graph/normalizeSnapshot";
import webSocket from "./hooks/webSocket";
import cytoscapeGraph from "./graph/cytoscapeGraph";
import { downloadGraphPng } from "./utils/downloadGraph";

export default function App() {
  const [menu, setMenu] = useState(initialMenuState);
  const [selectedElementInfo, setSelectedElementInfo] = useState(null);
  const [viewerSnapshot, setViewerSnapshot] = useState({ graph: null });
  const viewerSectionRef = useRef(null);
  const { status, lastMessage, sendMessage } = webSocket();
  const cyElements = useMemo(
    () => elementsFromGraphSnapshot(viewerSnapshot.graph),
    [viewerSnapshot.graph],
  );



  useEffect(() => {
    if (!lastMessage) return;

    if (lastMessage.type === "viewer_snapshot") {
      setViewerSnapshot({
        graph: lastMessage.graph ?? null,
      });
    }
  }, [lastMessage]);



  const hideMenu = useCallback(() => {
    setMenu((prev) => ({ ...prev, visible: false, elementId: null, elementKind: null }));
  }, []);



  const refreshGraph = useCallback(() => {
    sendMessage({ type: "request_snapshot" });
  }, [sendMessage]);

  const { containerRef, cyRef, runLayout, fitGraph, showWayPoints } = cytoscapeGraph({
    elements: cyElements,
    setMenu,
    hideMenu,
    setSelectedElementInfo,
  });

  const downloadGraph = useCallback(() => {
    downloadGraphPng(cyRef.current);
  }, [cyRef]);



  const onMenuAction = (action) => {
    if (!cyRef.current || !menu.elementId) return;
    const element = cyRef.current.getElementById(menu.elementId);
    if (!element) return;

    sendMessage({
      type: "menu_action",
      action,
      elementId: element.id(),
      elementKind: menu.elementKind || (element.isNode() ? "node" : "edge"),
    });

    hideMenu();
  };


  const toggleFullScreen = useCallback(async () => {
    const viewerSection = viewerSectionRef.current;
    if (!viewerSection) return;

    if (document.fullscreenElement) {
      await document.exitFullscreen();
      return;
    }

    await viewerSection.requestFullscreen();
  }, []);


  return (
    <>
      <main>
        <section ref={viewerSectionRef} className="viewer-section">
          <GraphCanvas
            ref={containerRef}
            info={selectedElementInfo}
            graphSnapshot={viewerSnapshot.graph}
            toolbar={(
              <Toolbar
                showWayPoints={showWayPoints}
                onLayout={runLayout}
                onFit={fitGraph}
                onDownload={downloadGraph}
                onRefresh={refreshGraph}
                status={status}
              />
            )}
          />
          <button type="button" className="fullScreenGraph" onClick={toggleFullScreen}>Full Screen</button>
          <ContextMenu menu={menu} onAction={onMenuAction} onClose={hideMenu} />
        </section>
      </main>
    </>
  );
}
