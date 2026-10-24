#pragma once
#include <chrono>

using clk = std::chrono::steady_clock;

template <typename StateEnum>
class FSM {
private:
    StateEnum current;
    clk::time_point entry;

public:
    void init(StateEnum s) {
        current = s;
        entry = clk::now();
    }

    void set_state(StateEnum s) {
        if (s != current) {
            current = s;
            entry = clk::now();
        }
    }

    StateEnum state() const { return current; }

    clk::duration time_in_state() const {
        return clk::now() - entry;
    }
};
