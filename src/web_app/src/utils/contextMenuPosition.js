const MENU_WIDTH = 220;
const MENU_HEIGHT = 164;
const OFFSET = 12;
const MARGIN = 8;

export function computeMenuPosition(containerRect, renderedPosition) {
  let x = containerRect.left + renderedPosition.x + OFFSET;
  let y = containerRect.top + renderedPosition.y + OFFSET;

  if (x + MENU_WIDTH > window.innerWidth) {
    x = Math.max(MARGIN, window.innerWidth - MENU_WIDTH - MARGIN);
  }

  if (y + MENU_HEIGHT > window.innerHeight) {
    y = Math.max(MARGIN, window.innerHeight - MENU_HEIGHT - MARGIN);
  }

  return {x, y};
}
