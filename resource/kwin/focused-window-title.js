function reportActiveWindowTitle(window) {
    callDBus(
        "io.github.filesfm.worktime",
        "/io/github/filesfm/worktime/FocusedWindow",
        "io.github.filesfm.worktime.FocusedWindow",
        "setActiveWindowTitle",
        window ? window.caption : ""
    );
}

workspace.windowActivated.connect(reportActiveWindowTitle);
reportActiveWindowTitle(workspace.activeWindow);