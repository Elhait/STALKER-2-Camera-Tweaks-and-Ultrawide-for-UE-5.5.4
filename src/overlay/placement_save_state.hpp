#pragma once

namespace overlay
{
    enum class PlacementSaveDisposition
    {
        Clean,
        PendingAfterMovement,
        FailedUntilNextMovement,
    };

    // A failed write is remembered without polling. A later actual position
    // change creates the next ordinary save opportunity for the latest value.
    class PlacementSaveState
    {
    public:
        void PositionChanged() noexcept
        {
            disposition_ = PlacementSaveDisposition::PendingAfterMovement;
        }

        bool ShouldAttemptSave(bool leftMouseButtonDown) const noexcept
        {
            return disposition_ == PlacementSaveDisposition::PendingAfterMovement &&
                !leftMouseButtonDown;
        }

        void CompleteAttempt(bool succeeded) noexcept
        {
            if (disposition_ != PlacementSaveDisposition::PendingAfterMovement)
                return;
            disposition_ = succeeded ? PlacementSaveDisposition::Clean
                : PlacementSaveDisposition::FailedUntilNextMovement;
        }

        PlacementSaveDisposition disposition() const noexcept { return disposition_; }

    private:
        PlacementSaveDisposition disposition_{PlacementSaveDisposition::Clean};
    };
}
