"""Filesystem behavior checks using placeholders, never game assets."""

import importlib.util
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
from unittest import mock


CHECKER_PATH = Path(__file__).resolve().parents[1] / "check_gamedata.py"
SPEC = importlib.util.spec_from_file_location("check_gamedata", CHECKER_PATH)
checker = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(checker)


class GamedataPathTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix="simpsons-path-test-")
        self.addCleanup(self.temp.cleanup)
        self.base = Path(self.temp.name)

    def make_game(self, name="gamedata", xex="default.xex", movies="movies"):
        root = self.base / name
        root.mkdir(parents=True)
        # A placeholder is sufficient: the checker must not require game bytes.
        (root / xex).write_bytes(b"path-test-placeholder")
        (root / movies).mkdir()
        return root

    def codes(self, result):
        return {issue["code"] for issue in result["errors"]}

    def test_existing_lowercase_paths_are_accepted_without_reading_assets(self):
        root = self.make_game()
        result = checker.check_gamedata(root)
        self.assertTrue(result["ok"])
        self.assertEqual(Path(result["default_xex"]), root / "default.xex")
        self.assertEqual(Path(result["movies"]), root / "movies")

    def test_uppercase_and_mixed_case_names_with_spaces_and_unicode(self):
        for name, xex, movies in (
            ("Test maiuscole", "DEFAULT.XEX", "MOVIES"),
            ("Giochi di Andrea/citta italiana è", "DeFaUlT.xEx", "MoViEs"),
        ):
            with self.subTest(name=name):
                root = self.make_game(name, xex, movies)
                result = checker.check_gamedata(str(root))
                self.assertTrue(result["ok"], result)
                self.assertEqual(Path(result["default_xex"]).name, xex)
                self.assertEqual(Path(result["movies"]).name, movies)

    def test_missing_root_is_not_created(self):
        root = self.base / "missing"
        result = checker.check_gamedata(root)
        self.assertFalse(result["ok"])
        self.assertIn("root_missing", self.codes(result))
        self.assertFalse(root.exists())

    def test_iso_file_is_rejected_as_root(self):
        path = self.base / "own-game.iso"
        path.write_bytes(b"placeholder")
        result = checker.check_gamedata(path)
        self.assertIn("root_not_directory", self.codes(result))

    def test_parent_folder_is_not_silently_replaced_with_nested_gamedata(self):
        self.make_game("install/gamedata")
        result = checker.check_gamedata(self.base / "install")
        self.assertFalse(result["ok"])
        self.assertEqual(self.codes(result), {"missing_default_xex", "missing_movies"})

    def test_missing_required_entry_reports_which_one(self):
        for missing in ("default.xex", "movies"):
            with self.subTest(missing=missing):
                root = self.make_game("missing-" + missing)
                entry = root / missing
                entry.rmdir() if entry.is_dir() else entry.unlink()
                result = checker.check_gamedata(root)
                self.assertFalse(result["ok"])
                self.assertEqual(len(result["errors"]), 1)
                self.assertIn(missing, result["errors"][0]["message"])

    def test_file_and_directory_types_cannot_be_swapped(self):
        root = self.base / "wrong-types"
        root.mkdir()
        (root / "default.xex").mkdir()
        (root / "movies").write_bytes(b"placeholder")
        result = checker.check_gamedata(root)
        self.assertFalse(result["ok"])
        self.assertEqual(self.codes(result), {"wrong_type_default_xex", "wrong_type_movies"})

    def test_case_collisions_are_rejected_on_case_sensitive_filesystems(self):
        root = self.make_game()
        (root / "DEFAULT.XEX").write_bytes(b"another-placeholder")
        if len([entry for entry in root.iterdir() if entry.name.casefold() == "default.xex"]) < 2:
            self.skipTest("Filesystem does not support separate case-colliding entries")
        result = checker.check_gamedata(root)
        self.assertFalse(result["ok"])
        self.assertIn("ambiguous_default_xex", self.codes(result))
        self.assertIsNone(result["default_xex"])

    def test_relative_paths_resolve_from_current_working_directory(self):
        root = self.make_game()
        previous = Path.cwd()
        try:
            os.chdir(self.base)
            result = checker.check_gamedata(Path("gamedata"))
        finally:
            os.chdir(previous)
        self.assertTrue(result["ok"])
        self.assertEqual(Path(result["game_data_root"]), root.resolve())

    def test_valid_root_symlink_is_accepted(self):
        root = self.make_game()
        link = self.base / "linked-root"
        try:
            link.symlink_to(root, target_is_directory=True)
        except OSError as exc:
            self.skipTest("Symlinks unavailable: " + str(exc))
        result = checker.check_gamedata(link)
        self.assertTrue(result["ok"])
        self.assertEqual(Path(result["game_data_root"]), root.resolve())

    def test_broken_required_symlink_is_reported(self):
        root = self.make_game()
        entry = root / "default.xex"
        entry.unlink()
        try:
            entry.symlink_to(root / "nonexistent.xex")
        except OSError as exc:
            self.skipTest("Symlinks unavailable: " + str(exc))
        result = checker.check_gamedata(root)
        self.assertFalse(result["ok"])
        self.assertIn("unavailable_default_xex", self.codes(result))

    def test_listing_error_becomes_a_diagnostic(self):
        root = self.make_game()
        with mock.patch.object(checker.os, "scandir", side_effect=PermissionError("access denied")):
            result = checker.check_gamedata(root)
        self.assertFalse(result["ok"])
        self.assertIn("root_unreadable", self.codes(result))

    def test_empty_path_does_not_implicitly_check_current_directory(self):
        result = checker.check_gamedata("")
        self.assertFalse(result["ok"])
        self.assertIn("empty_path", self.codes(result))

    def test_command_line_json_and_exit_codes(self):
        root = self.make_game("percorso con spazi")
        for path, expected_exit in ((root, 0), (self.base / "absent", 1)):
            with self.subTest(path=path):
                process = subprocess.run(
                    [sys.executable, str(CHECKER_PATH), str(path), "--json"],
                    capture_output=True, text=True, check=False,
                )
                self.assertEqual(process.returncode, expected_exit, process.stderr)
                result = json.loads(process.stdout)
                self.assertEqual(result["ok"], expected_exit == 0)
                self.assertEqual(process.stderr, "")

    def test_check_preserves_files_names_content_and_modification_times(self):
        root = self.make_game(xex="DEFAULT.XEX", movies="MoViEs")
        (root / "MoViEs" / "it").mkdir()
        (root / "MoViEs" / "it" / "placeholder.txt").write_text("preserve me", encoding="utf-8")

        def snapshot():
            return {
                str(path.relative_to(root)): (
                    path.stat().st_mode,
                    path.stat().st_mtime_ns,
                    path.read_bytes() if path.is_file() else None,
                )
                for path in [root, *root.rglob("*")]
            }

        before = snapshot()
        self.assertTrue(checker.check_gamedata(root)["ok"])
        self.assertEqual(snapshot(), before)


if __name__ == "__main__":
    unittest.main()
