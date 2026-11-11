#include "StatusRegistry.hpp"

#include <fcntl.h>
#include <unistd.h>
#include <cstdio>   // rename()

void StatusRegistry::add(const std::string& name, std::function<nlohmann::json()> getter)
{
    items.emplace_back(name, std::move(getter));
}

bool StatusRegistry::publish(const std::string& path)
{
    try
    {
        nlohmann::json j = nlohmann::json::object();
        for (const auto& item : items)
            j[item.first] = item.second();   // run the getter now, at publish time

        const std::string text = j.dump() + "\n";

        // .tmp lives in the same directory as `path`: rename() is only atomic
        // within one filesystem.
        const std::string tmp = path + ".tmp";
        int fd = open(tmp.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd < 0)
            return false;

        ssize_t written = write(fd, text.data(), text.size());
        close(fd);
        if (written != static_cast<ssize_t>(text.size()))
            return false;

        return rename(tmp.c_str(), path.c_str()) == 0;
    }
    catch (...)   // a throwing getter or an allocation failure must not kill the loop
    {
        return false;
    }
}
