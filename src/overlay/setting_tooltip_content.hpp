#pragma once

#include "../config/feature_config.hpp"
#include "localization_keys.hpp"

#include <span>
#include <string_view>

namespace overlay::setting_tooltip_content
{
    struct Option
    {
        std::string_view canonicalName;
        std::span<const loc::Key> descriptions;
        loc::Key examplesContext;
        std::span<const loc::Key> examples;
    };

    template <typename Value>
    struct SelectorOptionBinding
    {
        Value value;
        const Option* documentation;
    };

    namespace detail
    {
        inline constexpr loc::Key gameplayHorPlusDescription[]{
            loc::tooltip::gameplay::HorPlus};
        inline constexpr loc::Key gameplayHorPlusExamples[]{
            loc::tooltip::gameplay::HorPlusExample90,
            loc::tooltip::gameplay::HorPlusExample100,
            loc::tooltip::gameplay::HorPlusExample110};
        inline constexpr loc::Key gameplayAspectRecalculationDescription[]{
            loc::tooltip::gameplay::AspectRecalculation};

        inline constexpr loc::Key cinematicAspectAutoDescription[]{
            loc::tooltip::cinematics::AspectAuto};
        inline constexpr loc::Key cinematicAspectNativeDescription[]{
            loc::tooltip::cinematics::AspectNative};
        inline constexpr loc::Key cinematicAspect169Description[]{
            loc::tooltip::cinematics::Aspect169};
        inline constexpr loc::Key cinematicAspect219Description[]{
            loc::tooltip::cinematics::Aspect219};
        inline constexpr loc::Key cinematicAspect329Description[]{
            loc::tooltip::cinematics::Aspect329};
        inline constexpr loc::Key cinematicAspect329Examples[]{
            loc::tooltip::cinematics::AspectForcedExample};

        inline constexpr loc::Key cinematicGameplayFovDescription[]{
            loc::tooltip::cinematics::FovGameplay};
        inline constexpr loc::Key cinematicGameplayFovExamples[]{
            loc::tooltip::cinematics::FovGameplayExample90,
            loc::tooltip::cinematics::FovGameplayExample112_6};
        inline constexpr loc::Key cinematicNativeFovDescription[]{
            loc::tooltip::cinematics::FovNative};
        inline constexpr loc::Key cinematicNativeFovExamples[]{
            loc::tooltip::cinematics::FovNativeExample90};

        inline constexpr loc::Key dialogueNativeDescription[]{
            loc::tooltip::dialogue::ZoomNative};
        inline constexpr loc::Key dialogueNativeExamples[]{
            loc::tooltip::dialogue::ZoomNativeExample90,
            loc::tooltip::dialogue::ZoomNativeExample110};
        inline constexpr loc::Key dialogueAdaptiveDescription[]{
            loc::tooltip::dialogue::ZoomAdaptive};
        inline constexpr loc::Key dialogueAdaptiveExamples[]{
            loc::tooltip::dialogue::ZoomAdaptiveExample90,
            loc::tooltip::dialogue::ZoomAdaptiveExample110};
        inline constexpr loc::Key dialogueReducedDescription[]{
            loc::tooltip::dialogue::ZoomReduced};
        inline constexpr loc::Key dialogueReducedExamples[]{
            loc::tooltip::dialogue::ZoomReducedExample90,
            loc::tooltip::dialogue::ZoomReducedExample110};
        inline constexpr loc::Key dialogueDisabledDescription[]{
            loc::tooltip::dialogue::ZoomDisabled};
        inline constexpr loc::Key dialogueDisabledExamples[]{
            loc::tooltip::dialogue::ZoomDisabledExample90,
            loc::tooltip::dialogue::ZoomDisabledExample110};

        inline constexpr Option gameplayOptions[]{
            {"AspectRecalculation", gameplayAspectRecalculationDescription, {}, {}},
            {"HorPlus", gameplayHorPlusDescription,
                loc::tooltip::gameplay::HorPlusExamplesContext, gameplayHorPlusExamples},
        };
        inline constexpr Option cinematicAspectOptions[]{
            {"Auto", cinematicAspectAutoDescription, {}, {}},
            {"Native", cinematicAspectNativeDescription, {}, {}},
            {"16:9", cinematicAspect169Description, {}, {}},
            {"21:9", cinematicAspect219Description, {}, {}},
            {"32:9", cinematicAspect329Description, {}, cinematicAspect329Examples},
        };
        inline constexpr Option cinematicFovOptions[]{
            {"NativeHorPlus", cinematicNativeFovDescription,
                loc::tooltip::cinematics::FovNativeExamplesContext,
                cinematicNativeFovExamples},
            {"GameplayHorPlus", cinematicGameplayFovDescription,
                loc::tooltip::cinematics::FovGameplayExamplesContext,
                cinematicGameplayFovExamples},
        };
        inline constexpr Option dialogueOptions[]{
            {"Native", dialogueNativeDescription,
                loc::tooltip::dialogue::ZoomExamplesContext, dialogueNativeExamples},
            {"Adaptive", dialogueAdaptiveDescription,
                loc::tooltip::dialogue::ZoomExamplesContext, dialogueAdaptiveExamples},
            {"Reduced", dialogueReducedDescription,
                loc::tooltip::dialogue::ZoomExamplesContext, dialogueReducedExamples},
            {"Disabled", dialogueDisabledDescription,
                loc::tooltip::dialogue::ZoomExamplesContext, dialogueDisabledExamples},
        };
    }

    inline constexpr SelectorOptionBinding<config::GameplayMode>
        gameplaySelectorOptions[]{
            {config::GameplayMode::AspectRecalculation, &detail::gameplayOptions[0]},
            {config::GameplayMode::HorPlus, &detail::gameplayOptions[1]},
        };
    inline constexpr SelectorOptionBinding<config::CinematicAspectPolicy>
        cinematicAspectSelectorOptions[]{
            {config::CinematicAspectPolicy::Auto, &detail::cinematicAspectOptions[0]},
            {config::CinematicAspectPolicy::Native, &detail::cinematicAspectOptions[1]},
            {config::CinematicAspectPolicy::Forced16x9, &detail::cinematicAspectOptions[2]},
            {config::CinematicAspectPolicy::Forced21x9, &detail::cinematicAspectOptions[3]},
            {config::CinematicAspectPolicy::Forced32x9, &detail::cinematicAspectOptions[4]},
        };
    inline constexpr SelectorOptionBinding<config::CinematicFovMode>
        cinematicFovSelectorOptions[]{
            {config::CinematicFovMode::NativeHorPlus, &detail::cinematicFovOptions[0]},
            {config::CinematicFovMode::GameplayHorPlus, &detail::cinematicFovOptions[1]},
        };
    inline constexpr SelectorOptionBinding<config::DialogueZoomPolicy>
        dialogueSelectorOptions[]{
            {config::DialogueZoomPolicy::Native, &detail::dialogueOptions[0]},
            {config::DialogueZoomPolicy::Adaptive, &detail::dialogueOptions[1]},
            {config::DialogueZoomPolicy::Reduced, &detail::dialogueOptions[2]},
            {config::DialogueZoomPolicy::Disabled, &detail::dialogueOptions[3]},
        };

    inline std::span<const SelectorOptionBinding<config::GameplayMode>>
    GameplaySelectorOptions() noexcept { return gameplaySelectorOptions; }
    inline std::span<const SelectorOptionBinding<config::CinematicAspectPolicy>>
    CinematicAspectSelectorOptions() noexcept { return cinematicAspectSelectorOptions; }
    inline std::span<const SelectorOptionBinding<config::CinematicFovMode>>
    CinematicFovSelectorOptions() noexcept { return cinematicFovSelectorOptions; }
    inline std::span<const SelectorOptionBinding<config::DialogueZoomPolicy>>
    DialogueSelectorOptions() noexcept { return dialogueSelectorOptions; }

    inline std::span<const Option> Gameplay() noexcept
    {
        return detail::gameplayOptions;
    }

    inline std::span<const Option> CinematicAspect() noexcept
    {
        return detail::cinematicAspectOptions;
    }

    inline std::span<const Option> CinematicFov() noexcept
    {
        return detail::cinematicFovOptions;
    }

    inline std::span<const Option> Dialogue() noexcept
    {
        return detail::dialogueOptions;
    }
}
