#pragma once
#include <chrono>
#include <functional>

using clk = std::chrono::steady_clock;

template <typename StateEnum>
class FSM {
public:
    // Called on every real transition: (fsm name, from, to, reason).
    // Not set -> transitions are silent, exactly as before.
    using Sink = std::function<void(const char* fsm, const char* from,
                                    const char* to, const char* reason)>;

    // to_str is a plain function pointer to the enum's to_string overload.
    // init() does not fire the sink: entering the first state is not a transition.
    void init(StateEnum s, const char* name, const char* (*to_str)(StateEnum)) {
        current = s;
        entry = clk::now();
        this->fsm_name = name;
        this->to_str = to_str;
    }

    void set_sink(Sink s) { sink = std::move(s); }

    // Single choke point for transitions. The reason says WHICH condition
    // fired, so the event log reads like a story, not a state dump.
    void set_state(StateEnum s, const char* reason) {
        if (s == current)
            return;                       // only a real change resets the timer
        if (sink && to_str)
            sink(fsm_name, to_str(current), to_str(s), reason);
        current = s;
        entry = clk::now();
    }

    StateEnum state() const { return current; }

    clk::duration time_in_state() const {
        return clk::now() - entry;
    }

private:
    StateEnum current{};
    clk::time_point entry;
    const char* fsm_name = "?";
    const char* (*to_str)(StateEnum) = nullptr;
    Sink sink;
};
