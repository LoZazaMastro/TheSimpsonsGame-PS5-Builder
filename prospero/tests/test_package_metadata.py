#!/usr/bin/env python3
"""Run package.sh's real metadata stage without converting/signing an executable.

The temporary stages use the repository's actual param.json and controlled file
sizes. No game, collector or console process is built or launched by this test.
"""
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[2]
TITLE_ID = "PPSA74207"
PARAM = ROOT / "prospero/sce_sys/param.json"


def production_metadata_stage():
    # Execute the exact packaging block rather than a second implementation of
    # its checks. A moved/removed block fails the test and needs an explicit
    # update; the test must never silently check only a detached helper copy.
    source = (ROOT / "prospero/package.sh").read_text()
    marker = 'python3 - "$stage" "$PROSPERO_TITLE_ID" <<\'PY\'\n'
    if source.count(marker) != 1:
        raise RuntimeError("cannot identify package.sh's production metadata stage")
    return source.split(marker, 1)[1].split("\nPY\n", 1)[0] + "\n"


class PackageMetadataTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory(prefix="simpsons-metadata-")
        self.addCleanup(self.temporary.cleanup)
        self.stage = Path(self.temporary.name)
        for relative, mode in ((".", 0o755), ("sce_sys", 0o755), ("sce_module", 0o755),
                               ("logs", 0o777), ("cache", 0o777), ("userdata", 0o777)):
            directory = self.stage / relative
            directory.mkdir(exist_ok=True)
            directory.chmod(mode)
        for relative in ("eboot.bin", "sce_module/libc.prx"):
            path = self.stage / relative
            path.write_bytes(b"metadata-test fixture; not an executable\n")
            path.chmod(0o755)
        self.metadata_path = self.stage / "sce_sys/param.json"
        self.original = PARAM.read_bytes()
        self.metadata_path.write_bytes(self.original)

    def run_stage(self):
        return subprocess.run([sys.executable, "-", str(self.stage), TITLE_ID],
                              input=production_metadata_stage(), text=True,
                              capture_output=True, timeout=10)

    def test_real_template_preserved_as_file_sizes_change(self):
        metadata = json.loads(self.original)
        self.assertEqual(type(metadata["downloadDataSize"]), int)
        self.assertEqual(metadata["downloadDataSize"], 256)
        previous_total = None
        asset = self.stage / "sce_sys/variable-size-asset.bin"
        for size in (0, 131364743, 268435456):
            with self.subTest(file_size=size):
                # Sparse fixtures keep the test small while stat() sees the
                # exact sizes, including the value rejected on the console.
                with asset.open("wb") as stream:
                    stream.truncate(size)
                before = self.metadata_path.stat()
                result = self.run_stage()
                self.assertEqual(result.returncode, 0, result.stderr)
                self.assertEqual(self.metadata_path.read_bytes(), self.original)
                self.assertEqual(self.metadata_path.stat().st_mtime_ns, before.st_mtime_ns)
                statistics = json.loads(result.stdout)
                expected_total = sum(p.stat().st_size for p in self.stage.rglob("*")
                                     if p.is_file())
                self.assertEqual(statistics["title_total_bytes"], expected_total)
                self.assertEqual(statistics["downloadDataSize"], 256)
                self.assertEqual(statistics["download_data_size_source"], "explicit_metadata")
                if previous_total is not None:
                    self.assertGreater(expected_total, previous_total)
                previous_total = expected_total

    def test_invalid_or_byte_count_metadata_is_rejected_without_repair(self):
        for value in (131364743, 0, -1, "256", 256.0, True, None):
            with self.subTest(downloadDataSize=value):
                metadata = json.loads(self.original)
                if value is None:
                    metadata.pop("downloadDataSize")
                else:
                    metadata["downloadDataSize"] = value
                contents = (json.dumps(metadata, indent=4) + "\n").encode()
                self.metadata_path.write_bytes(contents)
                result = self.run_stage()
                self.assertNotEqual(result.returncode, 0)
                self.assertIn("downloadDataSize must be the explicit project value 256", result.stderr)
                self.assertEqual(self.metadata_path.read_bytes(), contents)


if __name__ == "__main__":
    unittest.main(verbosity=2)
