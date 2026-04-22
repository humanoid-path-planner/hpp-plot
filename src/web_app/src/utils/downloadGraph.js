export function downloadGraphPng(cy) {
  if (!cy) return;

  const pngUrl = cy.png({
    bg: 'black',
    full: false,
    maxWidth: 15000,
    maxHeight: 11250,
  });

  const a = document.createElement('a');
  a.href = pngUrl;
  a.download = 'graph.png';
  document.body.appendChild(a);
  a.click();
  document.body.removeChild(a);
}
