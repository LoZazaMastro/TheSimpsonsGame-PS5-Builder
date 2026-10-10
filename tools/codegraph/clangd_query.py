#!/usr/bin/env python3
"""
clangd_query.py -- lightweight LSP client for querying the SDK's clangd index.

Launches clangd once per invocation, talks raw LSP over stdio, and prints
results as plain text. Meant to be run from Bash by an agent, not a human.

The background index clangd builds is cached under build/.cache/clangd/index
(relative to --root), so repeated invocations across sessions get faster --
the first run after a source change re-indexes only the changed files.

Commands:
  symbol <query>         workspace/symbol fuzzy search -> file:line per hit
  outline <file>         textDocument/documentSymbol -> every symbol in a file
  def <file> <line> <col>       go-to-definition (1-indexed line/col)
  refs <file> <line> <col>      find all references
  callers <file> <line> <col>   incoming call hierarchy (who calls this)
  callees <file> <line> <col>   outgoing call hierarchy (what this calls)

Examples:
  python3 clangd_query.py --root /path/to/sdk symbol VulkanCommandProcessor::IssueDraw
  python3 clangd_query.py --root /path/to/sdk outline src/graphics/shared_memory.cpp
  python3 clangd_query.py --root /path/to/sdk callers src/graphics/shared_memory.cpp 313 6
"""
import argparse
import json
import os
import subprocess
import sys
import threading


class ClangdClient:
    def __init__(self, clangd_path, root, compile_commands_dir):
        self.proc = subprocess.Popen(
            [clangd_path, f"--compile-commands-dir={compile_commands_dir}",
             "--background-index", "--log=error"],
            stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.DEVNULL,
            cwd=root,
        )
        self.root = root
        self._id = 0
        self._buf = b""

    def _send(self, obj):
        data = json.dumps(obj).encode()
        header = f"Content-Length: {len(data)}\r\n\r\n".encode()
        self.proc.stdin.write(header + data)
        self.proc.stdin.flush()

    def _read_message(self):
        # Read headers.
        headers = {}
        while True:
            line = self.proc.stdout.readline()
            if not line:
                raise EOFError("clangd closed stdout")
            line = line.strip()
            if not line:
                break
            k, _, v = line.decode().partition(":")
            headers[k.strip().lower()] = v.strip()
        length = int(headers["content-length"])
        body = self.proc.stdout.read(length)
        return json.loads(body)

    def request(self, method, params, timeout_messages=2000):
        self._id += 1
        my_id = self._id
        self._send({"jsonrpc": "2.0", "id": my_id, "method": method, "params": params})
        for _ in range(timeout_messages):
            msg = self._read_message()
            if msg.get("id") == my_id:
                return msg.get("result")
            self._maybe_track_progress(msg)
        raise TimeoutError(f"no response to {method} after {timeout_messages} messages")

    def notify(self, method, params):
        self._send({"jsonrpc": "2.0", "method": method, "params": params})

    def _maybe_track_progress(self, msg):
        if msg.get("method") != "$/progress":
            return
        p = msg.get("params", {})
        value = p.get("value", {})
        if value.get("kind") == "begin":
            self._indexing_tokens.add(p.get("token"))
        elif value.get("kind") == "end":
            self._indexing_tokens.discard(p.get("token"))

    def prime_workspace_symbol(self, compile_commands_dir):
        """workspace/symbol returns nothing at all until at least one file has
        been opened via didOpen -- it doesn't matter which file. Open the
        first entry from compile_commands.json as a cheap anchor, then give
        clangd a few seconds to fold it into the project model."""
        import time
        cc_path = os.path.join(compile_commands_dir, "compile_commands.json")
        entries = json.load(open(cc_path))
        # Prefer a real, non-thirdparty C++ source as the anchor -- a random
        # vendored .c file (wrong language, unusual flags) is flaky.
        def rank(e):
            f = e["file"]
            return (
                0 if "thirdparty" not in f and "third_party" not in f else 1,
                0 if f.endswith((".cpp", ".cc", ".cxx")) else 1,
            )
        anchor = sorted(entries, key=rank)[0]["file"]
        self.open_file(anchor)
        time.sleep(10)

    def initialize(self):
        self._indexing_tokens = set()
        self.request("initialize", {
            "processId": os.getpid(),
            "rootUri": "file://" + self.root,
            "capabilities": {"window": {"workDoneProgress": True}},
        })
        self.notify("initialized", {})

    def open_file(self, path):
        abspath = path if os.path.isabs(path) else os.path.join(self.root, path)
        text = open(abspath, errors="replace").read()
        self.notify("textDocument/didOpen", {
            "textDocument": {
                "uri": "file://" + abspath, "languageId": "cpp",
                "version": 1, "text": text,
            }
        })
        return "file://" + abspath

    def shutdown(self):
        try:
            self.request("shutdown", {})
            self.notify("exit", {})
        except Exception:
            pass
        self.proc.terminate()


def uri_to_path(uri):
    return uri[len("file://"):] if uri.startswith("file://") else uri


def print_locations(locs, root):
    if locs is None:
        print("(no results)")
        return
    if isinstance(locs, dict):
        locs = [locs]
    if not locs:
        print("(no results)")
        return
    for loc in locs:
        target = loc.get("location", loc)
        uri = target.get("uri") or target.get("targetUri")
        rng = target.get("range") or target.get("targetRange") or target.get("selectionRange")
        path = uri_to_path(uri)
        try:
            path = os.path.relpath(path, root)
        except Exception:
            pass
        line = rng["start"]["line"] + 1
        col = rng["start"]["character"] + 1
        name = loc.get("name", "")
        prefix = f"{name}: " if name else ""
        print(f"{prefix}{path}:{line}:{col}")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--root", required=True, help="project root (compile_commands.json parent or subdir)")
    ap.add_argument("--compile-commands-dir", default=None, help="dir containing compile_commands.json (default: --root)")
    ap.add_argument("--clangd", default=None, help="path to clangd binary")
    ap.add_argument("command", choices=["symbol", "outline", "def", "refs", "callers", "callees"])
    ap.add_argument("args", nargs="+")
    a = ap.parse_args()

    root = os.path.abspath(a.root)
    ccdir = os.path.abspath(a.compile_commands_dir) if a.compile_commands_dir else root
    clangd_path = a.clangd or os.path.join(root, "..", "clang20", "bin", "clangd")
    clangd_path = os.path.abspath(clangd_path)

    client = ClangdClient(clangd_path, root, ccdir)
    try:
        client.initialize()

        if a.command == "symbol":
            query = " ".join(a.args)
            client.prime_workspace_symbol(ccdir)
            result = client.request("workspace/symbol", {"query": query})
            print_locations(result, root)

        elif a.command == "outline":
            uri = client.open_file(a.args[0])
            result = client.request("textDocument/documentSymbol", {"textDocument": {"uri": uri}})
            if not result:
                print("(no symbols)")

            def walk(sym, depth=0):
                name = sym.get("name")
                kind = sym.get("kind")
                rng = (sym.get("range") or sym.get("location", {}).get("range") or {}).get("start", {})
                print(f"{rng.get('line', 0)+1}: {'  ' * depth}{name} (kind={kind})")
                for child in sym.get("children") or []:
                    walk(child, depth + 1)

            for sym in result or []:
                walk(sym)

        else:
            import time
            file_, line, col = a.args[0], int(a.args[1]), int(a.args[2])
            uri = client.open_file(file_)
            time.sleep(5)
            pos = {"line": line - 1, "character": col - 1}
            if a.command == "def":
                result = client.request("textDocument/definition",
                                         {"textDocument": {"uri": uri}, "position": pos})
                print_locations(result, root)
            elif a.command == "refs":
                result = client.request("textDocument/references",
                                         {"textDocument": {"uri": uri}, "position": pos,
                                          "context": {"includeDeclaration": True}})
                print_locations(result, root)
            elif a.command in ("callers", "callees"):
                prep = client.request("textDocument/prepareCallHierarchy",
                                       {"textDocument": {"uri": uri}, "position": pos})
                if not prep:
                    print("(no call hierarchy item at this position)")
                    return
                item = prep[0]
                method = ("callHierarchy/incomingCalls" if a.command == "callers"
                          else "callHierarchy/outgoingCalls")
                result = client.request(method, {"item": item})
                if not result:
                    print("(none)")
                for entry in result or []:
                    node = entry.get("from") or entry.get("to")
                    name = node.get("name")
                    uri2 = node.get("uri")
                    rng = node.get("range", {}).get("start", {})
                    path = uri_to_path(uri2)
                    try:
                        path = os.path.relpath(path, root)
                    except Exception:
                        pass
                    print(f"{name}: {path}:{rng.get('line', 0)+1}")
    finally:
        client.shutdown()


if __name__ == "__main__":
    main()
