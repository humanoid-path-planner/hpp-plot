import { useCallback, useEffect, useMemo, useRef, useState } from "react";
import ContextMenu, { initialMenuState } from "./components/ContextMenu";
import GraphCanvas from "./components/GraphCanvas";
import DownloadForm from "./components/DownloadForm";
import Toolbar from "./components/Toolbar";
import { elementsFromGraphSnapshot } from "./graph/normalizeSnapshot";
import useGraphWebSocket from "./hooks/webSocket";
import useCytoscapeGraph from "./graph/cytoscapeGraph";
import { loadConfigFromJson } from "./utils/downloadGraph";

export default function App() {
  const [menu, setMenu] = useState(initialMenuState);
  const [selectedElementInfo, setSelectedElementInfo] = useState(null);
  const [viewerSnapshot, setViewerSnapshot] = useState({ graph: null });
  const viewerSectionRef = useRef(null);
  const mainRef = useRef(null);
  const { status, lastMessage, sendMessage } = useGraphWebSocket();
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
    setMenu((prev) => ({ ...prev, visible: false, selectedIds: [], elementKind: null }));
  }, []);


  const refreshGraph = useCallback(() => {
    sendMessage({ type: "request_snapshot" });
  }, [sendMessage]);

  const { containerRef, cyRef, runLayout, fitGraph, showWayPoints, hideWaypointsRef } = useCytoscapeGraph({
    elements: cyElements,
    setMenu,
    hideMenu,
    setSelectedElementInfo,
  });

  const toggleOpenDownloadForm = useCallback(() => {
    const formDl = document.getElementById("downloadForm");
    if (formDl.style.display === "flex") {
      formDl.style.display = "none";
      const main = mainRef.current;
      main.style.filter = "none";
      main.style.pointerEvents = "auto";
      return;
    }
    formDl.style.display = "flex";
    const main = mainRef.current;
    main.style.filter = "blur(2px)";
    main.style.pointerEvents = "none";
  }, [cyRef]);





  const onMenuAction = (action) => {
    if (!cyRef.current || !menu.selectedIds || menu.selectedIds.length != 1) return;
    const element = cyRef.current.getElementById(menu.selectedIds[0]);
    if (!element) return;

    console.log(`Menu action triggered: ${action} on element ${element.id()}`);
    sendMessage({
      type: "menu_action",
      action,
      elementId: element.id(),
      elementKind: menu.elementKind || (element.isNode() ? "node" : "edge"),
    });

    hideMenu();
  };


  const loadJsonConfig = useCallback(() => {
    const input = document.createElement('input');
    input.type = 'file';
    input.accept = 'application/json';

    input.onchange = (event) => {
      const file = event.target.files[0];
      if (!file) return;

      const reader = new FileReader();
      reader.onload = (e) => {
        try {
          const json = e.target.result;
          loadConfigFromJson(cyRef.current, json);
        } catch (error) {
          console.error('Error loading JSON config:', error);
        }
      };
      reader.readAsText(file);
    };

    input.click();
  }, [cyRef]);



  return (
    <>
      <main ref={mainRef}>
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
                onDownload={toggleOpenDownloadForm}
                onRefresh={refreshGraph}
                status={status}
                loadJsonConfig={loadJsonConfig}
              />
            )}
          />
          <ContextMenu menu={menu} onAction={onMenuAction} onClose={hideMenu} cyRef={cyRef} hideWaypointsRef={hideWaypointsRef} />
        </section>
      </main>
      <DownloadForm close={toggleOpenDownloadForm} cyRef={cyRef} />

    </>
  );
}
