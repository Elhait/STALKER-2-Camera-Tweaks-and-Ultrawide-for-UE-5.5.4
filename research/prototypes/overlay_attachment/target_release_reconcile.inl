    bool Renderer::ReconcileCompositionAttachment() noexcept
    {
        const bool required = visible() || !notifications_.empty() ||
            startupNotification_.pending();
        // Experiment only: retain the visual/surface/device, not the HWND target.
        if (required && !compositionTarget_) {
            bool acquired = false;
            try {
                acquired = CreateCompositionTarget(window_);
            } catch (...) {
                // Remain inside the optional Renderer exception boundary.
            }
            if (!acquired) {
                Disable("experiment_target_reacquire_failed");
                return false;
            }
        }
        if (visualAttached_ != required) {
            bool committed = false;
            try {
                if (compositionTarget_ && compositionDevice_ && visual_) {
                    committed = CommitCompositionAttachment(visualAttached_, required,
                        [this](bool attach) {
                            return SUCCEEDED(compositionTarget_->SetRoot(
                                attach ? visual_.Get() : nullptr));
                        },
                        [this]() { return SUCCEEDED(compositionDevice_->Commit()); });
                }
            } catch (...) {
                // No reuse after an uncertain attachment transaction.
            }
            if (!committed) {
                Disable("composition_attachment_commit_failed");
                return false;
            }
            LogCompositionAttachment();
        }
        if (!required && compositionTarget_) {
            // Only after the root-clear transaction was accepted above.
            compositionTarget_.Reset();
            try { Log("OVERLAY_TARGET_EXPERIMENT released=1"); } catch (...) {}
        }
        return true;
    }
