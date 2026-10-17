// Included only into the ignored experimental copy of Renderer.
        HRESULT ProbeSetRoot(IDCompositionVisual* root) noexcept;
        HRESULT ProbeCommit() noexcept;
        bool ReconcileProbeAttachment() noexcept;
        bool probePendingRoot_{};
        bool probePendingAttached_{};
        bool probeRootKnown_{};
        bool probeRootAttached_{};
        bool probeIdleLogged_{};
        std::uint64_t probeRenderCalls_{};
        std::uint64_t probeDrawCalls_{};
        std::uint64_t probeCommitAttempts_{};
        std::uint64_t probeCommitAccepted_{};
