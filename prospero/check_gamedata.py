#!/usr/bin/env python3
"""Read-only structural check of an existing, already compatible game dump.

Requires Python 3.9+. Only directory entries and file metadata are inspected.
No game content is read, copied, extracted, hashed, patched or uploaded.
"""

import argparse
import json
import os
from pathlib import Path
import stat
from typing import Any, Dict, Optional, Sequence, Union


def check_gamedata(path: Union[str, os.PathLike[str]]) -> Dict[str, Any]:
    """Check required root paths without inferring region or runtime support.

    Names are matched case-insensitively. Case collisions are rejected even
    when one entry matches the spelling exactly, since their behavior would
    differ across the source and destination filesystems. Valid symlinks are
    followed only for resolving the supplied root and checking entry types.
    """
    result: Dict[str, Any] = {
        "ok": False,
        "scope": "required_paths_only",
        "game_data_root": os.fspath(path),
        "default_xex": None,
        "movies": None,
        "errors": [],
    }

    def error(code: str, message: str) -> None:
        result["errors"].append({"code": code, "message": message})

    if not os.fspath(path).strip():
        error("empty_path", "Indicare il percorso della cartella gamedata.")
        return result

    try:
        root = Path(path).expanduser().resolve(strict=True)
        result["game_data_root"] = str(root)
        root_mode = root.stat().st_mode
    except FileNotFoundError:
        error("root_missing", "La cartella indicata non esiste.")
        return result
    except (OSError, RuntimeError, ValueError) as exc:
        error("root_unavailable", "Impossibile accedere al percorso: " + str(exc))
        return result

    if not stat.S_ISDIR(root_mode):
        error("root_not_directory", "Indicare la cartella gamedata, non un file o un'ISO.")
        return result

    try:
        # Inspect just the root. Do not recurse into or alter the user's files.
        with os.scandir(root) as entries:
            root_entries = [Path(entry.path) for entry in entries]
    except OSError as exc:
        error("root_unreadable", "Impossibile elencare la cartella gamedata: " + str(exc))
        return result

    for expected, field, is_required_kind, kind_name in (
        ("default.xex", "default_xex", stat.S_ISREG, "un file"),
        ("movies", "movies", stat.S_ISDIR, "una cartella"),
    ):
        matches = sorted(
            (entry for entry in root_entries if entry.name.casefold() == expected),
            key=lambda entry: entry.name,
        )
        if not matches:
            error("missing_" + field, "Manca " + expected + " nella radice di gamedata.")
            continue
        if len(matches) > 1:
            names = ", ".join(entry.name for entry in matches)
            error("ambiguous_" + field, "Nomi ambigui per " + expected + ": " + names)
            continue

        entry = matches[0]
        try:
            mode = entry.stat().st_mode
        except (OSError, ValueError) as exc:
            error("unavailable_" + field, "Percorso non disponibile " + str(entry) + ": " + str(exc))
            continue
        if not is_required_kind(mode):
            error("wrong_type_" + field, str(entry) + " deve essere " + kind_name + ".")
            continue
        result[field] = str(entry)

    result["ok"] = not result["errors"]
    return result


def main(argv: Optional[Sequence[str]] = None) -> int:
    parser = argparse.ArgumentParser(
        description=("Controlla in sola lettura i percorsi di un gamedata già funzionante "
                     "nel recomp PC; non verifica l'avvio su PS5.")
    )
    parser.add_argument("gamedata", help="Cartella con default.xex e movies direttamente al suo interno")
    parser.add_argument("--json", action="store_true", help="Emette un risultato JSON su stdout")
    args = parser.parse_args(argv)
    result = check_gamedata(args.gamedata)

    if args.json:
        print(json.dumps(result, ensure_ascii=True, indent=2))
    elif result["ok"]:
        print("OK: percorsi richiesti presenti in " + result["game_data_root"])
        print("default.xex: " + result["default_xex"])
        print("movies:      " + result["movies"])
        print("Controllo strutturale completato; asset e avvio su PS5 non verificati.")
    else:
        print("Controllo fallito: " + result["game_data_root"])
        for issue in result["errors"]:
            print("- " + issue["message"])

    return 0 if result["ok"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
