#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""POCKET METAL's installer on this computer only, at http://localhost:8765/ (Web MIDI works on localhost), with the
packages a build made (build/apps/metal/ is served as firmware/). Chrome or Edge. Ctrl+C stops it.
  python3 tools/serve.py"""
import functools, http.server, os, sys, webbrowser

here = os.path.dirname(os.path.abspath(__file__))
root = os.path.abspath(os.path.join(here, "..", "docs"))
fwdir = os.path.abspath(os.path.join(here, "..", "build", "apps", "metal"))

class Handler(http.server.SimpleHTTPRequestHandler):
    extensions_map = {**http.server.SimpleHTTPRequestHandler.extensions_map,
                      ".js": "text/javascript", ".mjs": "text/javascript", ".fwsc": "application/octet-stream"}
    def translate_path(self, path):
        p = super().translate_path(path)
        prefix = os.path.join(root, "firmware")
        if p == prefix or p.startswith(prefix + os.sep):
            return os.path.join(fwdir, os.path.relpath(p, prefix))
        return p
    def end_headers(self):
        self.send_header("Cache-Control", "no-store")
        super().end_headers()

if not os.path.exists(os.path.join(fwdir, "manifest.json")):
    sys.exit("No package yet in build/apps/metal/: build first (BUILD.bat, or node tools/op.mjs build).")
for port in range(8765, 8785):
    try:
        srv = http.server.ThreadingHTTPServer(("127.0.0.1", port), functools.partial(Handler, directory=root))
        break
    except OSError:
        continue
else:
    sys.exit("Could not open a local port (8765 to 8784).")
url = f"http://localhost:{port}/"
print(f"\n  POCKET METAL installer, offline: {url}\n  (Chrome or Edge; Ctrl+C to stop)\n")
webbrowser.open(url)
try:
    srv.serve_forever()
except KeyboardInterrupt:
    pass
