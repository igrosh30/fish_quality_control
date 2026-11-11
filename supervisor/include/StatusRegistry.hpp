#pragma once

#include <string>
#include <vector>
#include <utility>
#include <functional>
#include <nlohmann/json.hpp>

// Collects named getters ("cam.state" -> lambda) and dumps them all as one
// flat JSON object. Stores getters, not pointers, because some values are
// computed on the spot (time_in_state, countdowns).
class StatusRegistry
{
public:
    void add(const std::string& name, std::function<nlohmann::json()> getter);

    // Serialize every registered field and atomically replace the file at
    // `path` (write <path>.tmp, then rename() over <path>). Readers always
    // see a complete JSON document. Returns false on any failure, never
    // throws, never blocks long (the target lives on tmpfs).
    bool publish(const std::string& path);

private:
    std::vector<std::pair<std::string, std::function<nlohmann::json()>>> items;
};
