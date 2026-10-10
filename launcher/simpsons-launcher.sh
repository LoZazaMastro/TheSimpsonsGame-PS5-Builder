#!/bin/bash
# The Simpsons Game -- Recompiled : launcher entry point
cd "$(dirname "$(readlink -f "$0")")"

# HAND PATCH: when Steam runs this shortcut via a compatibility tool (Steam
# Linux Runtime), it executes inside a sandboxed container whose own bundled
# python3 can be much older than the host's -- old enough that the vendored
# PySide6's `list[str]` annotation syntax (PEP 585, needs 3.9+; PySide6
# itself needs 3.10+) throws `TypeError: 'type' object is not subscriptable`
# on import. That silently drops the launcher into Steam's built-in browser
# instead of its own window, and the game window spawned afterward loses
# gamescope's window tracking as a result (black screen when switching to
# it). Pressure-vessel bind-mounts the real host filesystem at /run/host, so
# reach through that for a modern interpreter when present, falling back to
# whatever's on PATH otherwise (e.g. plain Desktop Mode, no container).
version_ok() {
    "$1" -c 'import sys; sys.exit(0 if (3, 10) <= sys.version_info[:2] < (3, 15) else 1)' 2>/dev/null
}

pick_python() {
    for cand in /run/host/usr/bin/python3 python3; do
        exe="$(command -v "$cand" 2>/dev/null)" || continue
        if version_ok "$exe"; then
            echo "$exe"
            return 0
        fi
    done
    command -v python3
}

PYEXE="$(pick_python)"
exec "$PYEXE" launcher.py "$@"
