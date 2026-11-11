#pragma once

#include <string>
#include <sys/types.h>

// Append-only JSON-lines log of FSM transitions and worker exits.
// One write() per line, file opened once at startup (O_APPEND).
// Above max_bytes the file rotates: events.jsonl -> events.jsonl.1
// (the previous .1 is overwritten). Every method is safe to call when
// open() failed: it just does nothing.
class EventLog
{
public:
    // max_bytes is a parameter (with the agreed ~5 MB default) so a test
    // can trigger rotation without writing 5 MB first.
    bool open(const std::string& path, size_t max_bytes = 5 * 1024 * 1024);

    void transition(const char* fsm, const char* from, const char* to, const char* reason);
    void child_exit(const char* fsm, pid_t pid, const char* status, int code);

private:
    void append_line(const std::string& line); // rotation check + single write()

    std::string path;
    size_t max_bytes = 0;
    size_t bytes = 0;   // current file size, tracked to avoid an fstat per line
    int fd = -1;
};
