#pragma once

namespace overlay
{
    // Factory exports are supplemental discovery only. Readiness requires an
    // independently installed, lifetime-validated shared factory table.
    constexpr bool HasFactoryObservationCoverage(bool sharedFactoryTableInstalled) noexcept
    {
        return sharedFactoryTableInstalled;
    }
}
