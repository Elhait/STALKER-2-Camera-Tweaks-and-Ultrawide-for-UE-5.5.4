#include "../../src/overlay/renderer_wait_policy.hpp"

#include <iostream>

int main()
{
    const bool validTimeout = overlay::GpuFenceWaitTimeoutMs > 0 &&
        overlay::GpuFenceWaitTimeoutMs < INFINITE;
    const bool completionOnly =
        overlay::GpuFenceWaitCompleted(WAIT_OBJECT_0) &&
        !overlay::GpuFenceWaitCompleted(WAIT_TIMEOUT) &&
        !overlay::GpuFenceWaitCompleted(WAIT_FAILED) &&
        !overlay::GpuFenceWaitCompleted(WAIT_ABANDONED);
    std::cout << "bounded_wait=" << (validTimeout ? "PASS" : "FAIL")
        << " wait_result_policy=" << (completionOnly ? "PASS" : "FAIL") << '\n';
    return validTimeout && completionOnly ? 0 : 1;
}
