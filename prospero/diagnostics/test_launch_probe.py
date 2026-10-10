#!/usr/bin/env python3
"""Host integration checks for the actual collector; never runs PS5 APIs.

The C source is compiled with a temporary Linux-only output directory and
mock native launch/user functions. The HTTP peer implements the real flat
ShadowMountPlus kernel-log response, including its boolean truncated field.
"""
import contextlib
import http.server
import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import threading
import unittest


class ProbeTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temporary = tempfile.TemporaryDirectory(prefix="simpsons-launch-probe-")
        cls.work = Path(cls.temporary.name)
        cls.target = cls.work / "PPSA74207"
        cls.executable = cls.work / "probe-host"
        mock = cls.work / "mock_native.c"
        mock.write_text(r'''
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef struct {
    uint32_t size; int32_t user_id; uint32_t app_opt;
    uint64_t crash_report; uint32_t check_flag;
} LaunchAppParam;
int sceUserServiceInitialize(void *arg) {
    assert(arg == NULL);
    const char *value = getenv("PROBE_MOCK_INIT");
    return value ? (int32_t)strtoul(value, NULL, 0) : 0;
}
int sceUserServiceGetForegroundUser(int32_t *user) {
    if (getenv("PROBE_MOCK_NO_USER")) return -25;
    *user = 12345;
    return 0;
}
int sceSystemServiceLaunchApp(const char *title, const char **args, LaunchAppParam *p) {
    assert(strcmp(title, "PPSA74207") == 0);
    assert(args && args[0] == NULL);
    assert(p->size == sizeof(*p) && p->user_id == 12345);
    assert(!p->app_opt && !p->crash_report && !p->check_flag);
    puts("MOCK_NATIVE_LAUNCH_PPSA74207");
    const char *value = getenv("PROBE_MOCK_RETURN");
    return value ? (int32_t)strtoul(value, NULL, 0) : 0;
}
''')
        source = Path(__file__).with_name("launch_probe.c")
        subprocess.run([
            os.environ.get("CC", "cc"), "-D_GNU_SOURCE", "-std=c11", "-O1",
            "-Wall", "-Wextra", "-Werror",
            f'-DLAUNCH_PROBE_HOST_TEST_DIR="{cls.target}"',
            str(source), str(mock), "-o", str(cls.executable),
        ], check=True)

    @classmethod
    def tearDownClass(cls):
        cls.temporary.cleanup()

    def setUp(self):
        shutil.rmtree(self.target, ignore_errors=True)

    @contextlib.contextmanager
    def kernel_api(self, status=200, payload=None, raw_body=None):
        if payload is None:
            content = '[PPSA74207] CRT: _start entered\nUTF-8: città\n'
            payload = {"status": 0, "content": content,
                       "total_bytes": 500000, "returned_bytes": len(content.encode()),
                       "truncated": True}
        body = raw_body if raw_body is not None else json.dumps(payload).encode()
        requests = []

        class Handler(http.server.BaseHTTPRequestHandler):
            protocol_version = "HTTP/1.1"

            def log_message(self, *args):
                pass

            def do_POST(self):
                received = self.rfile.read(int(self.headers.get("Content-Length", 0)))
                requests.append((self.path, received))
                self.send_response(status)
                self.send_header("Content-Type", "application/json")
                self.send_header("Content-Length", str(len(body)))
                self.send_header("Connection", "close")
                self.end_headers()
                # Exercise partial socket reads, without changing the contract.
                for start in range(0, len(body), 17):
                    self.wfile.write(body[start:start + 17])
                    self.wfile.flush()
                self.close_connection = True

        class Server(http.server.HTTPServer):
            allow_reuse_address = True

        with Server(("127.0.0.1", 10101), Handler) as server:
            worker = threading.Thread(target=server.serve_forever, daemon=True)
            worker.start()
            try:
                yield requests
            finally:
                server.shutdown()
                worker.join()
        for path, request_body in requests:
            self.assertEqual(path, "/api/v1/kernel-log")
            self.assertEqual(json.loads(request_body), {"max_bytes": 131072})

    def run_probe(self, *, umask=-1, **environment):
        env = {key: value for key, value in os.environ.items()
               if not key.startswith("PROBE_MOCK_")}
        env.update(environment)
        result = subprocess.run([str(self.executable)], env=env, text=True,
                                capture_output=True, timeout=18, umask=umask)
        path = self.target / "logs/launch-report.log"
        text = path.read_text() if path.is_file() else ""
        return result, text

    def test_actual_success_response_append_and_exact_single_title(self):
        path = self.target / "logs/launch-report.log"
        path.parent.mkdir(parents=True)
        self.target.chmod(0o750)
        path.parent.chmod(0o700)
        path.write_text("PREVIOUS REPORT MUST REMAIN\n")
        with self.kernel_api() as requests:
            result, text = self.run_probe(PROBE_MOCK_INIT="0x80960001")
        self.assertEqual(result.returncode, 0, result.stderr + result.stdout)
        self.assertEqual(len(requests), 2)
        self.assertEqual(result.stdout.count("MOCK_NATIVE_LAUNCH_PPSA74207"), 1)
        self.assertTrue(text.startswith("PREVIOUS REPORT MUST REMAIN\n"))
        self.assertIn("LAUNCH_REQUEST=ACCEPTED", text)
        self.assertIn("KERNEL_CAPTURE=COMPLETE_TAILS", text)
        self.assertIn('"truncated": true', text)
        self.assertIn("API acceptance does not prove _start", text)
        self.assertEqual(self.target.stat().st_mode & 0o777, 0o750)
        self.assertEqual(path.parent.stat().st_mode & 0o777, 0o700)

    def test_only_new_directories_receive_shared_permissions_despite_umask(self):
        with self.kernel_api():
            result, _ = self.run_probe(umask=0o077, PROBE_MOCK_NO_USER="1")
        self.assertEqual(result.returncode, 4, result.stderr + result.stdout)
        self.assertEqual(self.target.stat().st_mode & 0o777, 0o777)
        self.assertEqual((self.target / "logs").stat().st_mode & 0o777, 0o777)

    def test_negative_native_result_preserved(self):
        value = 0x80940001
        with self.kernel_api():
            result, text = self.run_probe(PROBE_MOCK_RETURN=hex(value))
        self.assertEqual(result.returncode, 1)
        self.assertIn(f"hex=0x{value:08x} signed={value - 2**32}", text)
        self.assertIn("LAUNCH_REQUEST=REJECTED", text)
        self.assertEqual(result.stdout.count("MOCK_NATIVE_LAUNCH_PPSA74207"), 1)

    def test_kernel_permission_error_is_incomplete_but_launch_code_remains(self):
        with self.kernel_api(status=403, payload={"status": 13, "error": "Permission denied"}):
            result, text = self.run_probe()
        self.assertEqual(result.returncode, 3)
        self.assertIn("HTTP status=403; API status=13; JSON valid=yes", text)
        self.assertIn("KERNEL_CAPTURE=INCOMPLETE", text)
        self.assertIn("LAUNCH_REQUEST=ACCEPTED", text)

    def test_partial_json_never_claims_success(self):
        with self.kernel_api(raw_body=b'{"status":0,"content":"partial'):
            result, text = self.run_probe()
        self.assertEqual(result.returncode, 3)
        self.assertIn("JSON valid=no", text)
        self.assertIn("KERNEL_CAPTURE=INCOMPLETE", text)

    def test_no_foreground_user_prevents_launch(self):
        with self.kernel_api() as requests:
            result, text = self.run_probe(PROBE_MOCK_NO_USER="1")
        self.assertEqual(result.returncode, 4)
        self.assertEqual(len(requests), 1)
        self.assertNotIn("MOCK_NATIVE_LAUNCH", result.stdout)
        self.assertIn("NO_LAUNCH: foreground user unavailable", text)

    def test_report_symlink_is_rejected_before_any_launch(self):
        path = self.target / "logs/launch-report.log"
        path.parent.mkdir(parents=True)
        protected = self.work / "protected-user-file"
        protected.write_text("UNCHANGED")
        path.symlink_to(protected)
        result, _ = self.run_probe()
        self.assertEqual(result.returncode, 2)
        self.assertNotIn("MOCK_NATIVE_LAUNCH", result.stdout)
        self.assertEqual(protected.read_text(), "UNCHANGED")


if __name__ == "__main__":
    unittest.main(verbosity=2)
