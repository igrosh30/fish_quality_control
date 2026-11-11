#include "EventLog.hpp"

#include <nlohmann/json.hpp>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <cstdio>   // rename(), perror()
#include <ctime>

bool EventLog::open(const std::string& path, size_t max_bytes)
{
    this->path = path;
    this->max_bytes = max_bytes;

    fd = ::open(path.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd < 0)
    {
        perror("EventLog open");
        return false;
    }

    struct stat st;
    bytes = (fstat(fd, &st) == 0) ? static_cast<size_t>(st.st_size) : 0;
    return true;
}

void EventLog::transition(const char* fsm, const char* from, const char* to, const char* reason)
{
    if (fd < 0) return;

    // json does the quoting/escaping, so a reason string can never break a line
    nlohmann::json j;
    j["ts"]     = std::time(nullptr);
    j["fsm"]    = fsm;
    j["from"]   = from;
    j["to"]     = to;
    j["reason"] = reason;
    append_line(j.dump() + "\n");
}

void EventLog::child_exit(const char* fsm, pid_t pid, const char* status, int code)
{
    if (fd < 0) return;

    nlohmann::json j;
    j["ts"]     = std::time(nullptr);
    j["fsm"]    = fsm;
    j["event"]  = "child_exit";
    j["pid"]    = pid;
    j["status"] = status;
    j["code"]   = code;
    append_line(j.dump() + "\n");
}

void EventLog::append_line(const std::string& line)
{
    if (bytes + line.size() > max_bytes)
    {
        // rotate: current file becomes .1 (replacing the old .1), start fresh
        close(fd);
        rename(path.c_str(), (path + ".1").c_str());
        fd = ::open(path.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0644);
        bytes = 0;
        if (fd < 0) { perror("EventLog rotate"); return; }
    }

    if (write(fd, line.data(), line.size()) < 0)
        perror("EventLog write");
    else
        bytes += line.size();
}
