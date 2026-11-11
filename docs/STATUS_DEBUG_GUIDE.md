# Status & Debug system — owner's guide

This explains **where every message goes, where it is stored, and the exact steps
to add a new one**. Written for the system as of Phase 2.

---

## 1. The two channels — know which one you need

The supervisor emits two very different kinds of information. Before adding
anything, decide which kind yours is:

| | **Snapshot** (live status) | **Event log** (history) |
|---|---|---|
| Question it answers | "what is the state **right now**?" | "**what happened**, and why?" |
| File | `/dev/shm/fish_status.json` | `/home/ciimar/fish_quality_control/data/events.jsonl` |
| Storage | RAM (tmpfs) — gone on reboot, zero flash wear | SD/flash — survives reboot |
| Written | the whole file is **replaced** once per second | one line is **appended** per event |
| Growth | never grows (always ~1 kB) | grows; auto-rotates to `events.jsonl.1` above 5 MB |
| Written by | `StatusRegistry::publish()` (called in `main`'s loop) | `EventLog` (called by the FSM sink + the managers) |
| Read with | `watch -n1 cat /dev/shm/fish_status.json` | `tail -f .../data/events.jsonl` |

**Rule of thumb:** a *value* (counter, state, countdown, pid) → snapshot.
A *moment* (transition, worker died, something fired) → event log.

### Nothing is sent anywhere yet

Today both channels are **files on the Jetson**. No network is involved.
Phase 3 adds `display_debug.py`, a separate process that will *read these two
files* and POST them to a server. The supervisor itself will never talk to the
network — that separation is deliberate: the supervisor must never block, and
a network call to a dead server is the definition of blocking.

```
                    ┌── supervisor (C++) ────────────────────────┐
                    │                                            │
  every set_state() ──► FSM sink ──► EventLog ──► data/events.jsonl   (append, disk)
  every child exit  ──► elog->child_exit() ──┘
                    │                                            │
  once per second   ──► StatusRegistry.publish() ──► /dev/shm/fish_status.json  (replace, RAM)
                    └────────────────────────────────────────────┘
                                                      │
                               (Phase 3, future)      ▼
                               display_debug.py reads both files ──POST──► dashboard server
```

---

## 2. Recipe A — add a new value to the snapshot

**Cost: one line.** That was the whole design goal.

### Example: publish the tank's `ping_tries`

The owner of the variable is `WaterTank_Manager`, so the line goes in
**its** `register_status()` (`supervisor/src/WaterTank_Manager.cpp`):

```cpp
r.add("tank.ping_tries", [this] { return ping_tries; });
```

Done. Rebuild, run, and `tank.ping_tries` appears in the JSON within a second.

### Why this works with a private member

`register_status()` is a member function of `WaterTank_Manager`, so code
written inside it — including the lambda — may touch private members. The
lambda captures `this` (a pointer to the manager), and every second, when
`publish()` runs it, it reads `this->ping_tries` **fresh at that moment**.
You registered a *getter*, not a value.

### The three shapes a getter can take

```cpp
// 1. plain member — just return it
r.add("tank.ping_tries", [this] { return ping_tries; });

// 2. computed value — do the math inside
r.add("tank.time_in_state_s", [this] {
    return std::chrono::duration_cast<std::chrono::seconds>(managerFSM.time_in_state()).count();
});

// 3. sometimes-null — needs the explicit return type
r.add("tank.next_cycle_in_s", [this]() -> nlohmann::json {
    if (managerFSM.state() != tank_manager_state::IDLE)
        return nullptr;              // JSON null = "not applicable right now"
    ...
    return seconds_left;
});
```

Shape 3's `-> nlohmann::json` is **mandatory** when one branch returns
`nullptr` and another returns a number — without it the compiler cannot deduce
a single return type and refuses to compile.

### Naming convention

Flat key, prefix = owner: `cam.*`, `tank.*`, `sup.*`. Keys come out of the
file alphabetically sorted, so a shared prefix keeps related fields together.

### If the value lives in a NEW class

1. Give the class `void register_status(StatusRegistry& r);` (declare in its
   header; `class StatusRegistry;` forward declaration at the top is enough).
2. In the `.cpp`, `#include "StatusRegistry.hpp"` and write the `r.add(...)`
   lines with a new prefix.
3. In `main.cpp`, after `setup()`: `my_thing.register_status(status);`

That is the entire integration. `StatusRegistry` never needs to know your
class exists — it only stores name + getter pairs.

---

## 3. Recipe B — a new transition (or reason) in the event log

Transitions log **automatically**. There is one choke point:

```cpp
camFSM.set_state(cam_manager_state::CAPTURE, "timeout 60min, daytime");
//                        │                        │
//              where you're going      WHY — which condition fired
```

On a real change (same-state calls are ignored), the FSM calls the sink that
`main` installed, and the sink appends one line:

```json
{"ts":1794423148,"fsm":"cam","from":"IDLE","to":"CAPTURE","reason":"timeout 60min, daytime"}
```

So:

- **Change a reason** → edit the string at that `set_state` call. Nothing else.
- **Add a new transition** → just call `set_state(target, "reason")`. Logged.
- **Add a new state** → add the enumerator to the enum **and** a case to its
  `to_string()` (in the same header as the enum). If you forget the case, the
  log shows `"UNKNOWN"` — that's your reminder, not a crash.
- **A reason can contain anything** — quotes, backslashes — because the line
  is built by nlohmann::json, which escapes it. You cannot break the file
  format with a weird string.

### Writing a good reason

The reason answers *"which condition in the `if` fired?"* — short, specific,
present tense: `"reply window timed out"`, `"tank READY, start cycle sent"`.
Bad: `"state change"`, `"error"` (those say nothing the from/to don't).

### Wiring a NEW FSM into the log

If you create another `FSM<my_enum>` somewhere:

```cpp
// at init (the init does NOT log — entering the first state is not a transition):
myFSM.init(my_enum::START, "myname", to_string);

// in main, reuse the one existing sink:
my_manager.myFSM.set_sink(sink);
```

Note: `linkFSM` deliberately has **no sink** — it flip-flops every 2 s by
design and would write ~43k lines/day. If you ever want it:
`tank_manager.linkFSM.set_sink(sink);` in `main.cpp`, one line.

---

## 4. Recipe C — a new *kind* of event line (like `child_exit`)

`EventLog` currently knows two line shapes:

```cpp
void transition(const char* fsm, const char* from, const char* to, const char* reason);
void child_exit(const char* fsm, pid_t pid, const char* status, int code);
```

To add a third shape — say, a one-off note — copy the pattern in
`supervisor/src/EventLog.cpp`:

```cpp
// EventLog.hpp: declare it
void note(const char* fsm, const char* msg);

// EventLog.cpp: build the json, hand it to append_line
void EventLog::note(const char* fsm, const char* msg)
{
    if (fd < 0) return;              // open() failed -> every method is a no-op
    nlohmann::json j;
    j["ts"]    = std::time(nullptr);
    j["fsm"]   = fsm;
    j["event"] = "note";             // give each shape an "event" tag
    j["msg"]   = msg;
    append_line(j.dump() + "\n");
}
```

`append_line` gives you rotation and the single-`write()` guarantee for free.
Call it from a manager through the `elog` pointer, always null-checked:

```cpp
if (elog) elog->note("cam", "something noteworthy happened");
```

The `if (elog)` matters: managers receive the log via `set_event_log()` from
`main`; if nobody wired it, `elog` is `nullptr` and logging silently does
nothing — the manager must work without it.

---

## 5. Storage details you should know

- **Why `/dev/shm`:** it's tmpfs = RAM. The snapshot rewrites every second
  forever; on SD that's ~86k writes/day of pure wear for data that's only
  interesting *now*.
- **Why the snapshot never tears:** `publish()` writes `fish_status.json.tmp`
  (same directory!) then `rename()`s it over the real name. A rename atomically
  swaps the directory entry, so a reader sees the complete old file or the
  complete new file, never a half-written one. This is also why the `.tmp`
  must live next to the target: `rename()` only works within one filesystem.
- **Event log rotation:** when `events.jsonl` would pass 5 MB, it is renamed
  to `events.jsonl.1` (replacing the previous `.1`) and a fresh file starts.
  Total disk use is therefore bounded at ~10 MB, forever. The newest events
  are always in `events.jsonl`; the generation before, in `.1`; older is gone.
- **One `write()` per event line + `O_APPEND`:** the kernel appends each line
  as a single unit, so lines never interleave or split. Readers like
  `tail -f` always see whole lines.
- **Both paths are constants in `common/include/Config.h`:**
  `status_json_path` and `events_jsonl_path`. Change them there, nowhere else.

---

## 6. Quick verification (Jetson)

```bash
# snapshot alive? seq must climb by 1 per second
watch -n1 cat /dev/shm/fish_status.json

# snapshot always valid JSON, even mid-rewrite?
while :; do python3 -m json.tool /dev/shm/fish_status.json >/dev/null || echo TORN; done

# events flowing, with reasons?
tail -f /home/ciimar/fish_quality_control/data/events.jsonl

# a single event line, pretty-printed
tail -1 /home/ciimar/fish_quality_control/data/events.jsonl | python3 -m json.tool
```

After adding a field: `make clean && make` (the Makefile doesn't track header
dependencies, so `clean` first when a `.hpp` changed), restart the supervisor,
and your key appears in the snapshot within one second.

---

## 7. Mental model in one paragraph

Every manager *owns* its variables and *publishes* them itself: status fields
by registering `[this]`-capturing getters once at startup, transitions by
giving a reason at the single `set_state` choke point. The supervisor's loop
only does two mechanical things with all of that — run every getter and
atomically replace one RAM file (1 Hz), and append sink output to one disk
file as events happen. Everything downstream (Phase 3's pump, Phase 4's web
page) just *reads those two files*; nothing downstream can ever slow the
supervisor down, because the supervisor never waits for any reader.
