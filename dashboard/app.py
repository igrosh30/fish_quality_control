#!/usr/bin/env python3
"""Dashboard server for the fish supervisor.

Receives what display_debug.py pumps from the Jetson and serves one page
that shows it live.

    POST /ingest          {"status": {...}|null, "events": [...]}  (from the pump)
    GET  /status          latest snapshot + how stale it is
    GET  /events?since=N  events newer than id N (page polls with its last id)
    GET  /                the dashboard page (static/index.html)

Runs locally for now (the VPS does not exist yet):

    python3 app.py                      # 127.0.0.1:5000
    python3 app.py --host 0.0.0.0 --port 8080
"""

import argparse
import json
import os
import time
import threading
from collections import deque

from flask import Flask, request, jsonify, send_from_directory

app = Flask(__name__, static_folder="static")

# ---- shared state (the dev server runs threaded: one lock guards it all) ----
lock = threading.Lock()
latest = {"status": None, "received_at": None}
events = deque()          # entries: {"id": n, "data": <event as sent>}
next_id = 1
keep = 500                # events held in memory for GET /events
store_path = None         # every received event is also appended here (disk)


@app.post("/ingest")
def ingest():
    global next_id
    payload = request.get_json(force=True, silent=True)
    if payload is None or not isinstance(payload, dict):
        return jsonify(error="body is not a JSON object"), 400

    new_events = payload.get("events") or []
    with lock:
        latest["status"] = payload.get("status")
        latest["received_at"] = time.time()
        for ev in new_events:
            events.append({"id": next_id, "data": ev})
            next_id += 1
        while len(events) > keep:
            events.popleft()
        if store_path and new_events:      # disk copy survives server restarts
            with open(store_path, "a") as f:
                for ev in new_events:
                    f.write(json.dumps(ev) + "\n")
    return jsonify(ok=True)


@app.get("/status")
def status():
    now = time.time()
    with lock:
        st = latest["status"]
        received_at = latest["received_at"]

    # Two different staleness signals, because there are two ways to be dead:
    #  - pump_age_s:     time since the last POST (pump or network down)
    #  - snapshot_age_s: now - sup.ts_unix INSIDE the snapshot. The pump keeps
    #    re-sending the frozen file when the supervisor dies, so received time
    #    alone would never notice; the snapshot's own timestamp does.
    pump_age = (now - received_at) if received_at is not None else None
    snap_age = None
    if isinstance(st, dict) and isinstance(st.get("sup.ts_unix"), (int, float)):
        snap_age = max(0.0, now - st["sup.ts_unix"])   # clamp tiny clock skew

    return jsonify(status=st,
                   received_at=received_at,
                   pump_age_s=pump_age,
                   snapshot_age_s=snap_age)


@app.get("/events")
def get_events():
    try:
        since = int(request.args.get("since", 0))
    except ValueError:
        since = 0
    with lock:
        newer = [e for e in events if e["id"] > since]
        last = next_id - 1
    return jsonify(events=newer, last_id=last)


@app.get("/")
def index():
    return send_from_directory(app.static_folder, "index.html")


def main():
    global keep, store_path
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--host", default="127.0.0.1")
    ap.add_argument("--port", type=int, default=5000)
    ap.add_argument("--keep", type=int, default=500,
                    help="events kept in memory for the page")
    ap.add_argument("--store", default=os.path.join(os.path.dirname(__file__),
                                                    "received_events.jsonl"),
                    help="disk file where every received event is appended"
                         " ('' disables)")
    args = ap.parse_args()
    keep = args.keep
    store_path = args.store or None

    app.run(host=args.host, port=args.port, threaded=True)


if __name__ == "__main__":
    main()
