#pragma once

#include <cstdint>

namespace diagnostics
{
    enum class CinematicExitCandidateForm : std::uint8_t
    {
        Legacy,
        Indexed,
    };

    struct CinematicLifecycleGprSnapshot
    {
        std::uintptr_t rax{};
        std::uintptr_t rbx{};
        std::uintptr_t rcx{};
        std::uintptr_t rdx{};
        std::uintptr_t rsi{};
        std::uintptr_t rdi{};
        std::uintptr_t r8{};
        std::uintptr_t r9{};
        std::uintptr_t r10{};
        std::uintptr_t r11{};
        std::uintptr_t r12{};
        std::uintptr_t r13{};
        std::uintptr_t r14{};
        std::uintptr_t r15{};
    };

    struct CinematicLifecycleEnterContext
    {
        bool valid{};
        std::uint64_t generation{};
        float xmm0{};
        CinematicLifecycleGprSnapshot gprs{};
        std::uint32_t coordinator{};
        std::uint64_t selectionGeneration{};
        bool selectionValid{};
    };

    struct CinematicLifecycleExitContext
    {
        bool valid{};
        CinematicExitCandidateForm form{CinematicExitCandidateForm::Legacy};
        float xmm0{};
        CinematicLifecycleGprSnapshot gprs{};
        std::uint32_t coordinator{};
        std::uint64_t selectionGeneration{};
        bool selectionValid{};
        bool acceptedByF7A{};
        std::uint64_t associationGeneration{};
        std::uintptr_t aspectStoreTargetObject{};
        std::uintptr_t associatedWriterRsi{};
        bool associationValid{};
        bool associationConflict{};
    };

    inline bool CaptureCinematicLifecycleEnter(bool diagnosticMode,
        std::uint64_t generation, float xmm0,
        const CinematicLifecycleGprSnapshot& gprs, std::uint32_t coordinator,
        std::uint64_t selectionGeneration, bool selectionValid,
        CinematicLifecycleEnterContext& output) noexcept
    {
        if (!diagnosticMode) return false;
        output = { true, generation, xmm0, gprs, coordinator,
            selectionGeneration, selectionValid };
        return true;
    }

    inline bool CaptureCinematicLifecycleExit(bool diagnosticMode,
        CinematicExitCandidateForm form, float xmm0,
        const CinematicLifecycleGprSnapshot& gprs, std::uint32_t coordinator,
        std::uint64_t selectionGeneration, bool selectionValid,
        bool acceptedByF7A, std::uint64_t associationGeneration,
        std::uintptr_t aspectStoreTargetObject,
        std::uintptr_t associatedWriterRsi, bool associationValid,
        bool associationConflict, CinematicLifecycleExitContext& output) noexcept
    {
        if (!diagnosticMode) return false;
        output = { true, form, xmm0, gprs, coordinator, selectionGeneration,
            selectionValid, acceptedByF7A, associationGeneration,
            aspectStoreTargetObject, associatedWriterRsi, associationValid,
            associationConflict };
        return true;
    }

    // Diagnostic comparison only. A matching register is not lifecycle
    // identity and must never be used to authorize an EXIT.
    inline std::uint16_t CompareCinematicLifecycleGprs(
        const CinematicLifecycleGprSnapshot& enter,
        const CinematicLifecycleGprSnapshot& exit) noexcept
    {
        const std::uintptr_t enterValues[] = {
            enter.rax, enter.rbx, enter.rcx, enter.rdx, enter.rsi, enter.rdi,
            enter.r8, enter.r9, enter.r10, enter.r11, enter.r12, enter.r13,
            enter.r14, enter.r15,
        };
        const std::uintptr_t exitValues[] = {
            exit.rax, exit.rbx, exit.rcx, exit.rdx, exit.rsi, exit.rdi,
            exit.r8, exit.r9, exit.r10, exit.r11, exit.r12, exit.r13,
            exit.r14, exit.r15,
        };
        std::uint16_t matches = 0;
        for (std::uint16_t index = 0; index < 14; ++index) {
            if (enterValues[index] == exitValues[index])
                matches |= static_cast<std::uint16_t>(1u << index);
        }
        return matches;
    }

    inline const char* CinematicExitCandidateFormName(
        CinematicExitCandidateForm form) noexcept
    {
        return form == CinematicExitCandidateForm::Indexed ? "indexed" : "legacy";
    }
}
