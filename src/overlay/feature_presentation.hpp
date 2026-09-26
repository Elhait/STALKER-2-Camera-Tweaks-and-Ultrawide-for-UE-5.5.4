#pragma once

#include "../plugin/runtime_settings.hpp"
#include "localization_keys.hpp"

namespace overlay
{
    enum class FeatureStatusKind
    {
        CannotAssess,
        Unavailable,
        Disabled,
        Active,
        EnabledStateful,
        Ready,
        Applying,
        Starting,
        Ending,
        Waiting,
    };

    enum class FeatureStatusTone
    {
        Neutral,
        Positive,
        Warning,
        Error,
    };

    struct FeaturePresentation
    {
        FeatureStatusKind kind{FeatureStatusKind::CannotAssess};
        FeatureStatusTone tone{FeatureStatusTone::Neutral};
        loc::Key label{loc::common::CannotAssess};
        loc::Key detail{loc::status::RuntimeUnavailable};
    };

    FeaturePresentation ProjectGameplayPresentation(
        const plugin::OverlaySemanticSnapshot& snapshot) noexcept;
    FeaturePresentation ProjectCinematicPresentation(
        const plugin::OverlaySemanticSnapshot& snapshot) noexcept;
    FeaturePresentation ProjectDialoguePresentation(
        const plugin::OverlaySemanticSnapshot& snapshot) noexcept;
}
