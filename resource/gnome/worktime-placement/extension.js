import Gio from 'gi://Gio';
import {Extension} from 'resource:///org/gnome/shell/extensions/extension.js';
import * as Main from 'resource:///org/gnome/shell/ui/main.js';

const APP_ID = 'io.github.filesfm.worktime';
const POPUP_TITLE = 'WorkTime';
const MARGIN = 8;

const DBUS_SCHEMA = `
<node>
    <interface name="io.github.filesfm.worktime.Placement">
        <method name="PlaceNextPopup" />
    </interface>
</node>`;

// Top-left corner for a popup of the given size next to the pointer. The popup
// opens above the pointer when the pointer is in the lower half of the work
// area (a bottom panel), and below it otherwise (a top bar). It is kept inside
// the work area, so it never covers the panel or top bar.
function popupPosition(pointerX, pointerY, width, height, area) {
    const above = pointerY > area.y + area.height / 2;
    const wantedX = pointerX - width / 2;
    const wantedY = above ? pointerY - height - MARGIN : pointerY + MARGIN;

    return {
        x: Math.max(area.x + MARGIN, Math.min(wantedX, area.x + area.width - width - MARGIN)),
        y: Math.max(area.y + MARGIN, Math.min(wantedY, area.y + area.height - height - MARGIN)),
    };
}

function monitorIndexAt(x, y) {
    return Main.layoutManager.monitors.findIndex(
        monitor =>
            x >= monitor.x && x < monitor.x + monitor.width &&
            y >= monitor.y && y < monitor.y + monitor.height
    );
}

export default class WorkTimePlacement extends Extension {
    #dbus = null;
    #mapId = 0;
    // Set by PlaceNextPopup() just before the app shows the popup. It is
    // cleared once the popup is placed, so other windows are never moved.
    #pending = false;

    enable() {
        this.#dbus = Gio.DBusExportedObject.wrapJSObject(DBUS_SCHEMA, this);
        this.#dbus.export(Gio.DBus.session, '/io/github/filesfm/worktime/Placement');
        this.#mapId = global.window_manager.connect('map', (_wm, actor) => this.#onMap(actor));
    }

    disable() {
        global.window_manager.disconnect(this.#mapId);
        this.#mapId = 0;
        this.#pending = false;
        this.#dbus.unexport();
        this.#dbus = null;
    }

    PlaceNextPopup() {
        this.#pending = true;
    }

    #onMap(actor) {
        const window = actor.meta_window;
        if (!this.#pending || window.get_wm_class() !== APP_ID || window.get_title() !== POPUP_TITLE)
            return;

        this.#pending = false;

        // Mapping happens after Mutter has run its own placement, so this move is the final one.
        const [pointerX, pointerY] = global.get_pointer();
        let monitor = monitorIndexAt(pointerX, pointerY);
        if (monitor < 0)
            monitor = window.get_monitor();

        const frame = window.get_frame_rect();
        const area = Main.layoutManager.getWorkAreaForMonitor(monitor);
        const position = popupPosition(pointerX, pointerY, frame.width, frame.height, area);
        window.move_frame(true, position.x, position.y);
    }
}
