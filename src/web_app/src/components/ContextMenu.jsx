import { useEffect, useRef } from "react";
import { applyWaypointVisibility } from "../graph/cytoscapeGraph";
import * as Type from '../utils/Type';


export const initialMenuState = {
  visible: false,
  x: 0,
  y: 0,
  title: "Element",
  selectedIds: [],
  elementKind: null,
};

export default function ContextMenu({ menu, onAction, onClose, cyRef, hideWaypointsRef }) {
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


  const createGroupAction = () => {
    const cy = cyRef?.current;
    if (!cy) return;

    menu.selectedIds.forEach(id => {
      const element = cy.getElementById(id);
    });

    const newLabel = prompt("Enter new group name:", "");
    if (!newLabel) {
      onClose();
      return;
    }
    const newGroupId = `group-${Date.now()}`;
    cy.add({
      group: 'nodes',
      data: { id: newGroupId, label: newLabel, type: Type.GROUP_TYPE, name: newLabel, visible: true },
      classes: 'group-node',
    });

    menu.selectedIds.forEach((id) => {
      const element = cy.getElementById(id);
      if (element && element.isNode()) {
        element.move({ parent: newGroupId });
      }
    });

    onClose();
  };


  const renameGroupAction = () => {
    const cy = cyRef?.current;
    if (!cy) return;

    const groupId = menu.selectedIds[0];
    const groupElement = cy.getElementById(groupId);
    if (groupElement && groupElement.isNode() && groupElement.hasClass('group-node')) {
      const newLabel = prompt("Enter new group name:", groupElement.data('label'));
      if (newLabel) {
        groupElement.data('label', newLabel);
        groupElement.data('name', newLabel);
      }
    }
    onClose();
  };

  const toggleVisibilityAction = () => {
    const cy = cyRef?.current;
    if (!cy) return;

    const groupId = menu.selectedIds[0];
    const groupElement = cy.getElementById(groupId);
    if (groupElement && groupElement.isNode() && groupElement.hasClass('group-node')) {
      toggleVisibility(groupElement, cy);
    }

    applyWaypointVisibility(cy, hideWaypointsRef.current, true);
    onClose();
  };


  function toggleVisibility(groupElement, cy) {
      const isVisible = groupElement.data('visible') !== false;
      groupElement.data('visible', !isVisible);

      if (isVisible) {
        const children = groupElement.children();
        const metaEdges = [];

        children.connectedEdges().forEach(edge => {
          const source = edge.source();
          const target = edge.target();
          const isSourceChild = children.contains(source);
          const isTargetChild = children.contains(target);

          const classNames = [];
          classNames.push('meta-edge');
          if (Number(edge.data('weight')) <= 0)
              classNames.push('dotted');


          if (isSourceChild && !isTargetChild) {
            metaEdges.push({
              group: 'edges',
              data: {
                type : Type.META_EDGE_TYPE,
                id: `meta-${edge.id()}`,
                source: groupElement.id(),
                target: target.id(),
                weight: edge.data('weight'),
                label: edge.data('label'),
                nbWaypoints: edge.data('nbWaypoints'),
                isWaypointEdge: (source).data('type') === Type.WAYPOINT_STATE_TYPE || (target).data('type') === Type.WAYPOINT_STATE_TYPE
              },
              classes: classNames.join(' '),
            });
          } else if (!isSourceChild && isTargetChild) {
            metaEdges.push({
              group: 'edges',
              data: {
                type : Type.META_EDGE_TYPE,
                id: `meta-${edge.id()}`,
                source: source.id(),
                target: groupElement.id(),
                weight: edge.data('weight'),
                label: edge.data('label'),
                nbWaypoints: edge.data('nbWaypoints'),
                isWaypointEdge: (source).data('type') === Type.WAYPOINT_STATE_TYPE || (target).data('type') === Type.WAYPOINT_STATE_TYPE
              },
              classes: classNames.join(' ')
            });
          }
        });

        if (metaEdges.length > 0)
          cy.add(metaEdges);

        const childrenIds = children.map(c => c.id());
        groupElement.data('collapsedChildren', childrenIds);

        children.move({ parent: null });
        children.style('display', 'none');
        groupElement.addClass('collapsed-group');
      } else {

        cy.edges('.meta-edge').filter(e => e.source().id() === groupElement.id() || e.target().id() === groupElement.id()).remove();
        const childrenIds = groupElement.data('collapsedChildren') || [];

        const children = cy.collection(
          childrenIds.map(id => cy.getElementById(id)).filter(e => e.length > 0)
        );

        children.style('display', 'element');
        children.move({ parent: groupElement.id() });
        groupElement.removeClass('collapsed-group');
      }
  }


  const deleteGroupAction = () => {
    const cy = cyRef?.current;
    if (!cy) return;

    const groupId = menu.selectedIds[0];
    const groupElement = cy.getElementById(groupId);

    if (groupElement && groupElement.isNode() && groupElement.hasClass('group-node')) {
      const isVisible = groupElement.data('visible');

      if (!isVisible) {
        cy.edges('.meta-edge').filter(e => e.source().id() === groupElement.id() || e.target().id() === groupElement.id()).remove();

        const childrenIds = groupElement.data('collapsedChildren') || [];
        const children = cy.collection(
          childrenIds.map(id => cy.getElementById(id)).filter(e => e.length > 0)
        );
        children.move({ parent: null });
        children.style('display', 'element');

      } else {
        const children = groupElement.children();
        children.move({ parent: null });
      }
      groupElement.remove();
    }
    applyWaypointVisibility(cy, hideWaypointsRef.current, true);
    onClose();
  };


  const addToGroupAction = (groupId) => {
    const cy = cyRef?.current;
    if (!cy) return;

    if (!groupId) {
      onClose();
      return;
    }
    const groupElement = cy.getElementById(groupId);


    if (groupElement && groupElement.isNode() && groupElement.hasClass('group-node')) {
      const isVisible = groupElement.data('visible') !== false;

      menu.selectedIds.forEach(id => {
        const element = cy.getElementById(id);
        if (element && element.isNode()) {
          element.move({ parent: groupId });

          if (!isVisible) {
            const childrenIds = groupElement.data('collapsedChildren') || [];
            if (!childrenIds.includes(element.id())) {
              childrenIds.push(element.id());
              groupElement.data('collapsedChildren', childrenIds);
            }
            element.move({ parent: null });
            element.style('display', 'none');
          }
        }
      });
    } else {
      alert(`Group with ID ${groupId} not found.`);
    }

    onClose();
  };


  const removeFromGroupAction = () => {
    const cy = cyRef?.current;
    if (!cy) return;


    for (const id of menu.selectedIds) {
      const nodeElement = cy.getElementById(id);

      if (nodeElement && nodeElement.isNode() && nodeElement.data('parent')) {
        const parentId = nodeElement.data('parent');
        const parentElement = cy.getElementById(parentId);

        if (parentElement && parentElement.isNode() && parentElement.hasClass('group-node')) {
          const isVisible = parentElement.data('visible') !== false;

          nodeElement.move({ parent: null });

          if (!isVisible) {
            const childrenIds = parentElement.data('collapsedChildren') || [];
            const index = childrenIds.indexOf(id);
            if (index !== -1) {
              childrenIds.splice(index, 1);
              parentElement.data('collapsedChildren', childrenIds);
            }
            nodeElement.style('display', 'element');
          }
        }
      }
    }

    onClose();
  };



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



      {menu.selectedIds.length > 0 && menu.selectedIds.some(id => {
          const el = cyRef?.current?.getElementById(id);
          return el && el.isNode() && el.data('parent') === undefined;
        }) &&  menu.elementKind !== "group" && (
         <>
          <button
            className="context-menu-item"
            type="button"
            onClick={() => createGroupAction()}
          >
            Create Group
          </button>
        </>
      )}

      { menu.selectedIds.length > 0 && menu.selectedIds.some(id => {
          const el = cyRef?.current?.getElementById(id);
          return el && el.isNode() && el.data('parent') !== undefined;
        }) &&  menu.elementKind !== "group" && (
          <button
            className="context-menu-item"
            type="button"
            onClick={() => removeFromGroupAction()}
          >
            Remove from group
          </button>
      )}

      { menu.selectedIds.length > 0 && menu.selectedIds.some(id => {
          const el = cyRef?.current?.getElementById(id);
          return el && el.isNode() && el.data('parent') === undefined;
        }) &&  cyRef?.current.nodes('.group-node').length > 0 &&   menu.elementKind !== "group" && (

            <select
              className="context-menu-item"
              defaultValue=""
              onChange={(e) => addToGroupAction(e.target.value)}
              onClick={(e) => e.stopPropagation()}
            >
              <option value="" disabled selected>Add to group</option>
              {cyRef.current.nodes('.group-node').toArray().map(g => (
                <option key={g.id()} value={g.id()}>{g.data('label') || g.id()}</option>
              ))}
            </select>
      )}

      { menu.selectedIds.length > 0 && (menu.elementKind === "group" || menu.elementKind === "node" || menu.elementKind === "edge") &&   menu.elementKind !== "group" && (
        <hr style={{ margin: '4px 0', border: 'none', borderTop: '1px solid #ccc' }} />
      )}


      {
        menu.elementKind === "group" && (

          <>
            <button
              className="context-menu-item"
              type="button"
              onClick={() => renameGroupAction()}
            >
              rename
            </button>
              <button
              className="context-menu-item"
              type="button"
              onClick={() => toggleVisibilityAction()}
            >
              Toggle Visibility
            </button>
            <button
              className="context-menu-item"
              type="button"
              onClick={() => deleteGroupAction()}
            >
              delete group
            </button>
        </>
        )

      }


      {menu.elementKind === "Selection"}

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
