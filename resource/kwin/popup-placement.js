const APP_ID = "io.github.filesfm.worktime";
const POPUP_CAPTION = "WorkTime";
const MARGIN = 8;

// Top-left corner for a popup of the given size next to the pointer. The popup
// opens above the pointer when the pointer is in the lower half of the work
// area (a bottom panel), and below it otherwise (a top panel). It is kept
// inside the work area, so it never covers the panel.
function popupPosition(pointerX, pointerY, width, height, area) {
    const above = pointerY > area.y + area.height / 2;
    const wantedX = pointerX - width / 2;
    const wantedY = above ? pointerY - height - MARGIN : pointerY + MARGIN;

    return {
        x: Math.max(area.x + MARGIN, Math.min(wantedX, area.x + area.width - width - MARGIN)),
        y: Math.max(area.y + MARGIN, Math.min(wantedY, area.y + area.height - height - MARGIN)),
    };
}

function placePopup(window) {
    const pointer = workspace.cursorPos;
    // KWin 6 has no point overload of clientArea, only output and window ones.
    const area = workspace.clientArea(KWin.WorkArea, workspace.screenAt(pointer), workspace.currentDesktop);
    const frame = window.frameGeometry;
    const position = popupPosition(pointer.x, pointer.y, frame.width, frame.height, area);
    // KWin scripts run without the QML `Qt` global, so no Qt.rect here.
    window.frameGeometry = {x: position.x, y: position.y, width: frame.width, height: frame.height};
}

// Only loaded while a tray icon is in use, so the popup is the only window
// with this caption that this script ever sees.
workspace.windowAdded.connect(window => {
    if (window.resourceClass === APP_ID && window.caption === POPUP_CAPTION)
        placePopup(window);
});
