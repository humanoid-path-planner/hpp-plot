import { useEffect, useRef } from "react";

export const initialMenuState = {
  visible: false,
  x: 0,
  y: 0,
  title: "Element",
  elementId: null,
  elementKind: null,
};

export default function ContextMenu({ menu, onAction, onClose }) {
  const menuRef = useRef(null);

  useEffect(() => {
    const onDocumentClick = (event) => {
      if (!menuRef.current) return;
      if (!menuRef.current.contains(event.target)) {
        onClose();
      }
    };

    const onResize = () => onClose();

    document.addEventListener("click", onDocumentClick);
    window.addEventListener("resize", onResize);

    return () => {
      document.removeEventListener("click", onDocumentClick);
      window.removeEventListener("resize", onResize);
    };
  }, [onClose]);

  return (
    <div
      ref={menuRef}
      id="element-menu"
      className={`context-menu ${menu.visible ? "" : "hidden"}`}
      role="menu"
      aria-hidden={!menu.visible}
      style={{ left: `${menu.x}px`, top: `${menu.y}px` }}
    >
      <p id="element-menu-title" className="context-menu-title">{menu.title}</p>
      {menu.elementKind === "node" && (
        <>
          <button
            className="context-menu-item"
            type="button"
            onClick={() => onAction("generate_random_config")}
          >
            Generate from random
          </button>
          <button
            className="context-menu-item"
            type="button"
            onClick={() => onAction("generate_from_current_config")}
          >
            Generate from current
          </button>
          <button
            className="context-menu-item"
            type="button"
            onClick={() => onAction("set_target_state")}
          >
            Set as target
          </button>
        </>
      )}

      {menu.elementKind === "edge" && (
        <>
          <button
            className="context-menu-item"
            type="button"
            onClick={() => onAction("extend_current_to_current")}
          >
            Extend from current
          </button>
          <button
            className="context-menu-item"
            type="button"
            onClick={() => onAction("extend_current_to_random")}
          >
            Extend from random
          </button>
        </>
      )}
    </div>
  );
}
