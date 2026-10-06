#!/usr/bin/env python3
"""Write shields.io endpoint JSON: each package's latest published version and last smoke-test status.

Usage: make_badges.py <status-dir> <out-dir> < versions.json
The versions JSON comes from package_versions.py; status files come from smoke-package.sh.
"""

import json
import os
import re
import sys

NAMES = {
    "macos": "macOS",
    "windows": "Windows",
    "fedora": "Fedora",
    "ubuntu": "Ubuntu",
    "arch": "Arch",
    "linux": "Linux",
}
STATUS_COLORS = {"passing": "brightgreen", "failing": "red", "skipped": "yellow"}


def short_version(version):
    # "0.5.22~noble1" and "0.5.22-1" are shown as "0.5.22".
    match = re.match(r"\d+(?:\.\d+)*", version)
    return match.group(0) if match else version


def write_badge(out_dir, name, label, message, color):
    path = os.path.join(out_dir, f"{name}.json")
    with open(path, "w", encoding="utf-8") as handle:
        json.dump({"schemaVersion": 1, "label": label, "message": message, "color": color}, handle)
        handle.write("\n")


def main():
    status_dir, out_dir = sys.argv[1], sys.argv[2]
    versions = json.load(sys.stdin)
    for key, label in NAMES.items():
        info = versions.get(key, {})
        if info.get("error"):
            version = ("unknown", "lightgrey")
        elif info.get("latest"):
            version = (short_version(info["latest"]), "blue")
        else:
            version = ("not published", "lightgrey")
        write_badge(out_dir, f"{key}-version", label, *version)

        status = "unknown"
        status_file = os.path.join(status_dir, f"{key}.json")
        if os.path.exists(status_file):
            with open(status_file, encoding="utf-8") as handle:
                status = json.load(handle)["status"]
        color = STATUS_COLORS.get(status, "lightgrey")
        write_badge(out_dir, f"{key}-status", f"{label} test", status, color)


if __name__ == "__main__":
    main()
