#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace overlay::presentation
{
    struct Rect
    {
        int left{};
        int top{};
        int right{};
        int bottom{};

        bool empty() const noexcept { return right <= left || bottom <= top; }
        friend bool operator==(const Rect&, const Rect&) = default;
    };

    inline Rect Intersect(Rect left, Rect right) noexcept
    {
        return { (std::max)(left.left, right.left),
            (std::max)(left.top, right.top),
            (std::min)(left.right, right.right),
            (std::min)(left.bottom, right.bottom) };
    }

    inline Rect Union(Rect left, Rect right) noexcept
    {
        if (left.empty()) return right;
        if (right.empty()) return left;
        return { (std::min)(left.left, right.left),
            (std::min)(left.top, right.top),
            (std::max)(left.right, right.right),
            (std::max)(left.bottom, right.bottom) };
    }

    class DamageTracker
    {
    public:
        Rect Plan(Rect current, Rect extent, bool forceFull) const noexcept
        {
            if (forceFull || !valid_ || !IsInside(current, extent) ||
                !IsInside(previous_, extent))
                return extent;
            return Intersect(Union(previous_, current), extent);
        }

        void Publish(Rect current, Rect extent) noexcept
        {
            if (IsInside(current, extent)) {
                previous_ = current;
                valid_ = true;
            } else {
                Invalidate();
            }
        }

        void Invalidate() noexcept
        {
            previous_ = {};
            valid_ = false;
        }

        Rect previous() const noexcept { return previous_; }
        bool valid() const noexcept { return valid_; }

    private:
        static bool IsInside(Rect value, Rect extent) noexcept
        {
            if (extent.empty()) return false;
            return value.empty() || (value.left >= extent.left &&
                value.top >= extent.top && value.right <= extent.right &&
                value.bottom <= extent.bottom);
        }

        Rect previous_{};
        bool valid_{};
    };

    enum class DpiCoordinateAwareness : std::uint8_t
    {
        Unaware,
        SystemAware,
        PerMonitorAware,
        Unknown,
    };

    inline float HostCoordinateToPhysicalScale(DpiCoordinateAwareness awareness,
        unsigned windowDpi, unsigned systemDpi) noexcept
    {
        const float windowScale = static_cast<float>(windowDpi ? windowDpi : 96u) / 96.0f;
        switch (awareness) {
        case DpiCoordinateAwareness::Unaware:
            return windowScale;
        case DpiCoordinateAwareness::SystemAware:
            return static_cast<float>(windowDpi ? windowDpi : 96u) /
                static_cast<float>(systemDpi ? systemDpi : 96u);
        case DpiCoordinateAwareness::PerMonitorAware:
        case DpiCoordinateAwareness::Unknown:
        default:
            return 1.0f;
        }
    }

    inline int EffectiveFontPixels(int configured96DpiPixels,
        unsigned windowDpi) noexcept
    {
        const float scale = static_cast<float>(windowDpi ? windowDpi : 96u) / 96.0f;
        return (std::max)(1, static_cast<int>(std::lround(
            static_cast<float>(configured96DpiPixels) * scale)));
    }
}
