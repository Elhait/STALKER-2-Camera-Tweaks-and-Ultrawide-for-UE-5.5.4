#include "../../src/overlay/overlay_lifecycle.hpp"

#include <iostream>
#include <string>

namespace
{
    bool Check(bool condition, const char* name)
    {
        if (!condition) std::cerr << "FAIL: " << name << "\n";
        return condition;
    }
}

int main()
{
    overlay::Lifecycle lifecycle;
    bool pass = true;

    pass &= Check(lifecycle.state() == overlay::State::Unavailable,
        "initial_unavailable");
    pass &= Check(lifecycle.BeginDiscovery() &&
        lifecycle.state() == overlay::State::Discovering,
        "begin_discovery");
    pass &= Check(!lifecycle.CanRender(), "discovery_cannot_render");
    pass &= Check(lifecycle.MarkReady() && lifecycle.CanRender(),
        "discovery_ready");

    pass &= Check(lifecycle.BeginResize() &&
        lifecycle.state() == overlay::State::Resizing,
        "begin_resize");
    pass &= Check(!lifecycle.CanRender(), "resize_cannot_render");
    pass &= Check(lifecycle.CompleteResize(true) && lifecycle.CanRender(),
        "resize_recreation_ready");

    pass &= Check(lifecycle.BeginResize() && !lifecycle.CompleteResize(false) &&
        lifecycle.state() == overlay::State::Failed && !lifecycle.CanRender(),
        "resize_failure_fail_closed");
    pass &= Check(lifecycle.ResetForRecreation() &&
        lifecycle.state() == overlay::State::Discovering,
        "failed_recreation_restarts_discovery");
    pass &= Check(lifecycle.MarkReady() && lifecycle.Fail() &&
        lifecycle.state() == overlay::State::Failed,
        "discovery_failure");

    pass &= Check(lifecycle.Disable() && lifecycle.state() == overlay::State::Disabled,
        "disable");
    pass &= Check(!lifecycle.BeginDiscovery() && !lifecycle.ResetForRecreation() &&
        !lifecycle.Fail() && !lifecycle.CanRender(),
        "disabled_is_terminal");
    pass &= Check(!lifecycle.Disable(), "disable_is_idempotent");

    pass &= Check(std::string(overlay::StateName(overlay::State::Ready)) == "Ready",
        "state_name");
    std::cout << "Overlay lifecycle harness: " << (pass ? "PASS" : "FAIL") << "\n";
    return pass ? 0 : 1;
}
