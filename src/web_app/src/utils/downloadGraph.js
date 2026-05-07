import * as Type from './Type';
export function downloadGraphPng(cy, filename, background) {
  if (!cy) return;

  const pngUrl = cy.png({
    bg: background === 'transparent' ? 'transparent' :
        background === 'white'       ? '#ffffff' :
                                       '#000000',
    full: false,
    maxWidth: 15000,
    maxHeight: 11250,
  });

  const a = document.createElement('a');
  a.href = pngUrl;
  a.download = `${filename}.png`;
  document.body.appendChild(a);
  a.click();
  document.body.removeChild(a);
}



// const jsonConfig = {
//   elements:
//   [{
//     id : INT,
//     pos: [FLOAT, FLOAT],
//   }],

//   groups: [
//     {
//       name: STRING,
//       members: [INT],
//     }
//   ]

// };

export function downloadGraphJson(cy, filename) {
  if (!cy) return;

  const jsonConfig = {
    elements: [],
    groups: [],
  };


  jsonConfig.elements = cy.nodes().map((node) => {
    return {
      id: node.id(),
      pos: [node.position('x'), node.position('y')],
    };
  });

  jsonConfig.groups = cy.nodes('.group-node').map((node) => {
    const members = node.children().map((child) => child.id());
    return {
      name: node.data('name'),
      members: members,
      pos: [node.position('x'), node.position('y')],
    };
  });



  const jsonStr = JSON.stringify(jsonConfig, null, 2);
  const blob = new Blob([jsonStr], {type: 'application/json'});
  const url = URL.createObjectURL(blob);

  const a = document.createElement('a');
  a.href = url;
  a.download = `${filename}.json`;
  document.body.appendChild(a);
  a.click();
  document.body.removeChild(a);
  URL.revokeObjectURL(url);
}


export function loadConfigFromJson(cy, json) {
  const config = JSON.parse(json);

  for (const element of config.elements) {
    cy.getElementById(element.id).position({
      x: element.pos[0],
      y: element.pos[1],
    });
  }

  for (const group of config.groups) {
    createGroup(cy, group.name, group.members);
    cy.getElementById(`group-${group.name}`).position({
      x: group.pos[0],
      y: group.pos[1],
    });
  }
}

export function createGroup(cy, name, memberIds) {
  const newGroupId = `group-${Date.now()}`;
  cy.add({
    group: 'nodes',
    data: {
      id: newGroupId,
      label: name,
      type: Type.GROUP_TYPE,
      name: name,
      visible: true
    },
    classes: 'group-node',
  });

  memberIds.forEach((id) => {
    const element = cy.getElementById(id);
    if (element && element.isNode()) {
      element.move({parent: newGroupId});
    }
  });
}
