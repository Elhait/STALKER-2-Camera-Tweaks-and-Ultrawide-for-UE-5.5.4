// Research-only methods. Existing renderer serialization owns every field here.
#ifndef OVERLAY_ATTACHMENT_EXPERIMENT_DETACH
#error The attachment experiment must select B=0 or C=1 explicitly.
#endif

namespace overlay {
    void Renderer::LogAttachmentExperimentSnapshot(const char* reason) const noexcept
    {
        try {
            char buffer[512]{};
            LARGE_INTEGER qpc{};
            QueryPerformanceCounter(&qpc);
            std::snprintf(buffer, sizeof(buffer),
                "OVERLAY_ATTACHMENT_PROBE reason=%s variant=%s tick_ms=%llu qpc=%lld "
                "root_commit_accepted=%d visible=%d notifications=%zu startup_pending=%d "
                "render_calls=%llu draw_calls=%llu commit_attempts=%llu commits_accepted=%llu",
                reason, OVERLAY_ATTACHMENT_EXPERIMENT_DETACH ? "C" : "B",
                static_cast<unsigned long long>(GetTickCount64()), qpc.QuadPart,
                probeRootKnown_ ? static_cast<int>(probeRootAttached_) : -1,
                static_cast<int>(visible()), notifications_.size(),
                static_cast<int>(startupNotification_.pending()),
                static_cast<unsigned long long>(probeRenderCalls_),
                static_cast<unsigned long long>(probeDrawCalls_),
                static_cast<unsigned long long>(probeCommitAttempts_),
                static_cast<unsigned long long>(probeCommitAccepted_));
            Log(buffer);
        } catch (...) {
            // Observation must not affect presentation.
        }
    }

    HRESULT Renderer::ProbeSetRoot(IDCompositionVisual* root) noexcept
    {
        const HRESULT result = compositionTarget_->SetRoot(root);
        if (SUCCEEDED(result)) {
            probePendingRoot_ = true;
            probePendingAttached_ = root != nullptr;
        }
        return result;
    }

    HRESULT Renderer::ProbeCommit() noexcept
    {
        ++probeCommitAttempts_;
        const HRESULT result = compositionDevice_->Commit();
        if (SUCCEEDED(result)) {
            ++probeCommitAccepted_;
            if (probePendingRoot_) {
                const bool changed = !probeRootKnown_ ||
                    probeRootAttached_ != probePendingAttached_;
                probeRootAttached_ = probePendingAttached_;
                probeRootKnown_ = true;
                probePendingRoot_ = false;
                if (changed) LogAttachmentExperimentSnapshot("root_commit_accepted");
            }
        } else {
            LogAttachmentExperimentSnapshot("commit_failed");
        }
        return result;
    }

    bool Renderer::ReconcileProbeAttachment() noexcept
    {
        const bool contentNeeded = visible() || !notifications_.empty() ||
            startupNotification_.pending();
        if constexpr (OVERLAY_ATTACHMENT_EXPERIMENT_DETACH != 0) {
            if (!probeRootKnown_ || contentNeeded != probeRootAttached_) {
                if (!compositionTarget_ || !compositionDevice_ || !visual_ ||
                    FAILED(ProbeSetRoot(contentNeeded ? visual_.Get() : nullptr)) ||
                    FAILED(ProbeCommit())) {
                    Disable("attachment_experiment_transition_failed");
                    return false;
                }
            }
        }
        const bool idle = !contentNeeded && !surfaceDirty_;
        if (idle != probeIdleLogged_) {
            probeIdleLogged_ = idle;
            LogAttachmentExperimentSnapshot(idle ? "idle_begin" : "idle_end");
        }
        return true;
    }
}
