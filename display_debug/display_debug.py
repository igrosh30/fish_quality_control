#!/usr/bin/env python3
"""display_debug.py - Jetson-side pump for the status/debug system.

Once per second:
  1. read the status snapshot   (/dev/shm/fish_status.json)
  2. read NEW lines from the event log (data/events.jsonl), surviving rotation
  3. POST {"status": {...}|null, "events": [...]} to the dashboard server

It must never crash, whatever happens to the files or the server:
  - snapshot missing / invalid JSON  -> "status": null, keep going
  - half-written or corrupt event line -> skipped
  - server unreachable -> events buffered (bounded), retried next tick

Usage:
    display_debug.py http://SERVER:5000/ingest
    display_debug.py http://127.0.0.1:5000/ingest --interval 1 --buffer 1000
"""

import argparse
import json
import os
import sys
import time
import urllib.request
import urllib.error
from collections import deque

STATUS_PATH = "/dev/shm/fish_status.json"
EVENTS_PATH = "/home/ciimar/fish_quality_control/data/events.jsonl"


class EventsReader:
    """Tail-follows a .jsonl file like `tail -f`, remembering how far it got.

    Rotation (file renamed to <path>.1, fresh file created) is detected by the
    inode changing; the tail of the old generation is drained from <path>.1
    before reading the new file, so no event is lost across a rotation.
    """

    def __init__(self, path):
        self.path = path
        self.ino = None     # inode of the generation we are currently reading
        self.offset = 0     # bytes of that generation already consumed

    def start_at_end(self):
        """Begin at the current end of file: old history is not re-sent when
        the pump (re)starts, exactly like `tail -f`."""
        try:
            st = os.stat(self.path)
            self.ino, self.offset = st.st_ino, st.st_size
        except OSError:
            self.ino, self.offset = None, 0

    def poll(self):
        """Return the list of parsed events appended since the last call."""
        raw = []
        try:
            st = os.stat(self.path)
        except OSError:
            return []          # no log file (yet): nothing to do

        if self.ino is not None and st.st_ino != self.ino:
            # rotation happened: our old generation now lives at <path>.1
            raw += self._read_from(self.path + ".1", self.offset)
            self.ino, self.offset = st.st_ino, 0

        if self.ino is None:   # the file appeared after start_at_end() failed
            self.ino, self.offset = st.st_ino, 0

        if st.st_size < self.offset:
            self.offset = 0    # file shrank under us: start over, don't crash

        raw += self._read_from(self.path, self.offset, remember=True)
        return self._parse(raw)

    def _read_from(self, path, start, remember=False):
        """Read complete lines from `start` to EOF. A trailing piece with no
        newline is a line still being written: leave it for the next poll."""
        try:
            with open(path, "rb") as f:
                f.seek(start)
                data = f.read()
        except OSError:
            return []
        end = data.rfind(b"\n")
        if end < 0:
            return []
        if remember:
            self.offset = start + end + 1
        return data[:end].split(b"\n")

    @staticmethod
    def _parse(raw_lines):
        out = []
        for line in raw_lines:
            line = line.strip()
            if not line:
                continue
            try:
                out.append(json.loads(line))
            except json.JSONDecodeError:
                pass           # corrupt line: skip it, never die for it
        return out


def read_status(path):
    """The snapshot, or None if it is missing or unparsable right now."""
    try:
        with open(path) as f:
            return json.load(f)
    except (OSError, ValueError):
        return None


def post(url, payload, timeout_s):
    """True if the server accepted the payload."""
    body = json.dumps(payload).encode()
    req = urllib.request.Request(
        url, data=body, headers={"Content-Type": "application/json"})
    try:
        with urllib.request.urlopen(req, timeout=timeout_s) as resp:
            return 200 <= resp.status < 300
    except (urllib.error.URLError, OSError, ValueError):
        return False


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("url", help="ingest endpoint, e.g. http://SERVER:5000/ingest")
    ap.add_argument("--status-file", default=STATUS_PATH)
    ap.add_argument("--events-file", default=EVENTS_PATH)
    ap.add_argument("--interval", type=float, default=1.0, help="seconds between ticks")
    ap.add_argument("--buffer", type=int, default=1000,
                    help="max events held while the server is unreachable (oldest dropped)")
    ap.add_argument("--timeout", type=float, default=3.0, help="POST timeout seconds")
    args = ap.parse_args()

    reader = EventsReader(args.events_file)
    reader.start_at_end()
    pending = deque(maxlen=args.buffer)
    server_ok = None   # tri-state so we only log when reachability CHANGES

    print("pumping", args.status_file, "+", args.events_file, "->", args.url, flush=True)

    while True:
        tick_start = time.monotonic()
        try:
            status = read_status(args.status_file)
            for ev in reader.poll():
                pending.append(ev)

            batch = list(pending)
            ok = post(args.url, {"status": status, "events": batch}, args.timeout)
            if ok:
                # drop exactly what was sent (single-threaded: nothing was
                # appended between building the batch and here)
                for _ in range(len(batch)):
                    pending.popleft()

            if ok != server_ok:   # log state changes only, not every failure
                print("server reachable" if ok
                      else "server unreachable, buffering events", flush=True)
                server_ok = ok
        except Exception as e:
            # belt and braces: one broken tick must never kill the pump
            print("tick failed:", repr(e), file=sys.stderr, flush=True)

        elapsed = time.monotonic() - tick_start
        time.sleep(max(0.0, args.interval - elapsed))


if __name__ == "__main__":
    main()
